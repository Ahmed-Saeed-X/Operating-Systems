import redis
import time
import uuid
import multiprocessing

# Predefined wait times for each client process (in seconds)
client_processes_waiting = [0, 1, 1, 1, 4]

class Redlock:
    def __init__(self, redis_nodes):
        """
        Initialize the Redlock distributed lock manager.
        
        Args:
            redis_nodes (list): List of (host, port) tuples for Redis instances
        """
        # Create Redis client connections for each node
        self.redis_clients = [
            redis.StrictRedis(
                host=host, 
                port=port,
                decode_responses=True  # Automatically convert responses to strings
            ) for host, port in redis_nodes
        ]
        
        # Calculate quorum size using majority rule (N/2 + 1)
        self.quorum = len(redis_nodes) // 2 + 1


    def acquire_lock(self, resource, ttl):
        """
        Attempt to acquire a distributed lock using the Redlock algorithm.
        
        Args:
            resource (str): Name of the resource to lock
            ttl (int): Time-to-live for the lock in milliseconds
            
        Returns:
            tuple: (success status, lock_id) where:
                - success status (bool): True if lock acquired
                - lock_id (str): Unique identifier for the lock
        """

        # Generate unique lock identifier using UUID
        lock_id = str(uuid.uuid4())
        acquired = 0
        start_time = time.monotonic() * 1000  # High-precision timer in milliseconds

        # Attempt to acquire lock on all Redis nodes
        for client in self.redis_clients:
            try:
                # Use Redis SET command with NX (not exists) and PX (TTL in milliseconds)
                if client.set(resource, lock_id, nx=True, px=ttl):
                    acquired += 1
            except redis.RedisError:
                # Ignore connection errors to individual nodes
                continue

        # Calculate remaining valid lock time accounting for clock drift
        elapsed_time = time.monotonic() * 1000 - start_time
        validity = ttl - elapsed_time

        # Check if we meet quorum requirements and have positive validity time
        if acquired >= self.quorum and validity > 0:
            return True, lock_id
        
        # Cleanup: Release any partial locks using standard release mechanism
        self.release_lock(resource, lock_id)
        return False, None
        

    def release_lock(self, resource, lock_id):
        """
        Release the distributed lock safely using atomic Lua script.
        
        Args:
            resource (str): Name of the locked resource
            lock_id (str): Unique lock identifier to verify ownership
        """
        
        
        # Attempt release on all nodes, even if some fail
        for client in self.redis_clients:
            try:
                if client.get(resource) == lock_id:
                    client.delete(resource)
            except redis.RedisError:
                # Continue trying other nodes if current one fails
                continue

def client_process(redis_nodes, resource, ttl, client_id):
    """
    Function to simulate a single client process trying to acquire and release a lock.
    """
    time.sleep(client_processes_waiting[client_id])

    redlock = Redlock(redis_nodes)
    print(f"\nClient-{client_id}: Attempting to acquire lock...")
    lock_acquired, lock_id = redlock.acquire_lock(resource, ttl)

    if lock_acquired:
        print(f"\nClient-{client_id}: Lock acquired! Lock ID: {lock_id}")
        # Simulate critical section
        time.sleep(3)  # Simulate some work
        redlock.release_lock(resource, lock_id)
        print(f"\nClient-{client_id}: Lock released!")
    else:
        print(f"\nClient-{client_id}: Failed to acquire lock.")

if __name__ == "__main__":
    # Define Redis node addresses (host, port)
    redis_nodes = [
        ("localhost", 63791),
        ("localhost", 63792),
        ("localhost", 63793),
        ("localhost", 63794),
        ("localhost", 63795),
    ]

    resource = "shared_resource"
    ttl = 5000  # Lock TTL in milliseconds (5 seconds)

    # Number of client processes
    num_clients = 5

    # Start multiple client processes
    processes = []
    for i in range(num_clients):
        process = multiprocessing.Process(target=client_process, args=(redis_nodes, resource, ttl, i))
        processes.append(process)
        process.start()

    for process in processes:
        process.join()
  



    """
    docker compose up -d 
    python redlock_simulation.py
    """