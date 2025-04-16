#include <pthread.h>
#include "caltrain.h"

// Initialize station to default state
void station_init(struct station *station) {
    // Initialize mutex for thread-safe access to shared data
    pthread_mutex_init(&station->mutex, NULL);
    // Condition variable: passengers wait for train arrival
    pthread_cond_init(&station->train_arrived, NULL);
    // Condition variable: train waits for boarding completion
    pthread_cond_init(&station->all_passengers_seated, NULL);
    // No seats available initially
    station->numnerOfEmptySeats = 0;
    // No passengers waiting initially  
    station->numnerOfWaitingPassengers = 0;
    // No passengers in boarding process
    station->numnerOfPassengersWalkingOnTheTrain = 0;
}

// Called when train arrives at station
void station_load_train(struct station *station, int count) {
    // Enter critical section - lock shared data
    pthread_mutex_lock(&station->mutex);
    
    // Set available seats for this train
    station->numnerOfEmptySeats = count;
    
    // If train has seats AND there are waiting passengers, wake them up
    if (station->numnerOfEmptySeats > 0 && station->numnerOfWaitingPassengers > 0) {
        // Broadcast to all waiting passengers that train arrived
        pthread_cond_broadcast(&station->train_arrived);
    }
    
    // Wait until either:
    // 1. Train is full (no empty seats) OR no waiting passengers left
    // AND
    // 2. All passengers who started boarding have completed seating
    while ((station->numnerOfEmptySeats > 0 && station->numnerOfWaitingPassengers > 0) 
          || station->numnerOfPassengersWalkingOnTheTrain > 0) {
        // Releases lock and sleeps until all_passengers_seated is signaled
        // Automatically re-acquires lock when awakened
        pthread_cond_wait(&station->all_passengers_seated, &station->mutex);
    }
    
    // Reset seats before train departs
    station->numnerOfEmptySeats = 0;
    
    // Exit critical section - release lock
    pthread_mutex_unlock(&station->mutex);
}

// Called when passenger arrives at station
void station_wait_for_train(struct station *station) {
    // Enter critical section
    pthread_mutex_lock(&station->mutex);
    
    // Add passenger to waiting count
    station->numnerOfWaitingPassengers++;
    
    // Wait until train arrives with available seats
    // Loop protects against spurious wakeups
    while (station->numnerOfEmptySeats == 0) {
        // Releases lock and sleeps until train_arrived is signaled
        // Re-checks condition when awakened
        pthread_cond_wait(&station->train_arrived, &station->mutex);
    }
    
    // Claim a seat on the train
    station->numnerOfEmptySeats--;
    // Remove passenger from waiting count
    station->numnerOfWaitingPassengers--;
    // Add to count of boarding passengers
    station->numnerOfPassengersWalkingOnTheTrain++;
    
    // Exit critical section
    pthread_mutex_unlock(&station->mutex);
}

// Called when passenger is seated
void station_on_board(struct station *station) {
    // Enter critical section
    pthread_mutex_lock(&station->mutex);
    
    // Decrement count of boarding passengers
    station->numnerOfPassengersWalkingOnTheTrain--;
    
    // Check if these conditions are ALL true:
    // 1. No passengers are still boarding
    // AND 
    // 2. Either:
    //    a) Train is full (no seats left)
    //    OR
    //    b) No passengers left waiting
    if (station->numnerOfPassengersWalkingOnTheTrain == 0 &&
        (station->numnerOfEmptySeats == 0 ||  station->numnerOfWaitingPassengers == 0)) {
        // Signal train that loading is complete
        pthread_cond_signal(&station->all_passengers_seated);
    }
    
    // Exit critical section
    pthread_mutex_unlock(&station->mutex);
}