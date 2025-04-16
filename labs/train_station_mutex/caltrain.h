#include <pthread.h>

struct station {
    pthread_mutex_t mutex;                     // Main lock for synchronizing all operations
    pthread_cond_t train_arrived;              // Signaled when train arrives/opens doors
    pthread_cond_t all_passengers_seated;      // Signaled when last passenger boards
    int numnerOfEmptySeats;                    // Available seats in current train
    int numnerOfWaitingPassengers;             // Passengers waiting in station
    int numnerOfPassengersWalkingOnTheTrain;   // Passengers who boarded but not yet seated
};

// Function prototypes
void station_init(struct station *station);
void station_load_train(struct station *station, int count);
void station_wait_for_train(struct station *station);
void station_on_board(struct station *station);