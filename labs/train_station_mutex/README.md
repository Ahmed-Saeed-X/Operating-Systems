# CalTrain (Synchronization and Mutual Exclusion)

## 1. Objectives

- Understand and implement concurrent programming concepts using threads.
- Learn how to handle race conditions, synchronization, mutexes, and condition variables.
- Gain experience debugging concurrent programs and minimizing critical sections.

## 2. Problem Statement

In this project, you are tasked with automating the loading of passengers (robots) onto a train. Each robot and train is controlled by a separate thread, and you must implement synchronization functions to ensure that the loading process is orderly and efficient.

You will define a structure `struct station` and implement several functions to manage the loading of passengers onto the train. The main goal is to guarantee that:
- A train can load passengers only if there are enough available seats.
- Passengers cannot board until the train is in the station and there are free seats.
- Passengers should board concurrently without any conflicts, and the train should leave as soon as it's ready.

---

## 3. Requirements

You are required to implement the following:

### 3.1. Functions

- **`station_init`**: Initializes the station object when CalTrain starts.
- **`station_load_train(struct station *station, int count)`**: This function is invoked when the train arrives and has opened its doors. It ensures that the train is satisfactorily loaded (either full or all waiting passengers are seated). It must block until the loading is complete or no passengers are waiting.
- **`station_wait_for_train(struct station *station)`**: This function is called by a passenger robot when it arrives at the station. The robot will wait until there is a train and free seats for it to board.
- **`station_on_board(struct station *station)`**: This function is called by a passenger robot when it has successfully boarded the train.

### 3.2. Synchronization Constraints
- You must implement synchronization using **Pthreads mutexes** and **condition variables**. **Do not** use semaphores or other synchronization primitives.
- You can use **only one lock** per `struct station`.
- Avoid global variables and rely only on the `struct station` and function arguments.
- You must minimize critical sections and avoid unnecessary locking.

### 3.3. Condition Variables
- **`pthread_cond_broadcast`** should be used to notify all passengers that they can board the train.
- **`pthread_cond_signal`** should be used to signal when the train is ready to leave, allowing it to depart once all passengers are onboard or there are no waiting passengers.

### 3.4. Notes
- Use Ubuntu or a Virtual Machine (VM) with more than one CPU core for the best performance.
- Your solution must pass multiple runs with different thread schedules, so ensure that no errors occur during concurrent execution.
  
---

## 4. Code Implementation

### 4.1. **`caltrain.h`** - Declaration of `struct station`
You will define the `struct station` in this header file. The structure will store all necessary data to manage the state of the station, such as the number of available seats, the number of waiting passengers, and the synchronization primitives needed for the functions.

### 4.2. **`caltrain.c`** - Implementation of Synchronization Functions
In this file, you will implement:
- **`station_init`**: Initializes the station and its synchronization objects (mutex and condition variables).
- **`station_load_train`**: Manages the train's arrival and ensures that passengers are loaded appropriately.
- **`station_wait_for_train`**: Handles the arrival of passenger robots and their waiting process until they can board the train.
- **`station_on_board`**: Updates the station once a passenger has boarded.

---

## 5. Helpful Resources

To help you understand and implement synchronization, here are some useful resources:
- [What are race conditions?](https://youtu.be/FY9livorrJI)
- [What is Mutex?](https://youtu.be/oq29KUy29iQ)
- [Condition variables](https://youtu.be/0sVGnxg6Z3k?feature=shared)
- [Signalling for condition variables](https://youtu.be/RtTlIvnBw10?feature=shared)

---

## 6. Conclusion

This project will help you grasp essential concepts in concurrent programming, focusing on synchronization, race conditions, mutexes, and condition variables. By implementing these concepts to control the loading of passengers onto a train, you’ll gain hands-on experience debugging and optimizing multi-threaded applications.

---
