/// ============================================================================
/// Copyright (C) 2022 Pavol Federl (pfederl@ucalgary.ca)
/// All Rights Reserved. Do not distribute this file.
/// ============================================================================
///
/// You must modify this file and then submit it for grading to D2L.
///
/// You can delete all contents of this file and start from scratch if
/// you wish, as long as you implement the detect_primes() function as
/// defined in "detectPrimes.h".

// Lots of the code below is based off of code provided by my TA, Stephane Dorotich, specifically files barrier.cpp and thread_reuse.cpp
// However, code has been heavily modified by myself.

#include "detectPrimes.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <condition_variable>
#include <atomic>
#include <pthread.h>
#include <iostream>


//global variables
std::atomic<bool> primeFlag(false);                                                               // Flag for if number is prime or not
std::atomic<bool> cancelFlag(false);                                                              // Flag for when no numbers left in the provided vector
std::atomic<bool> done(false);                                                                    // Flag for if execution should continue or stop prime algo calculations
int64_t targetNum;
int64_t max = 0;
int totalThreads;
std::vector<int64_t> result;
std::vector<int64_t> numVector;
pthread_barrier_t barrier;


// Structure for threads
struct threadStruct {
  int offset;                                                                                       // Value used to offset different thread loop iteration starting points
};


// Function that the threads run
// Is responsible for finding out which numbers in the given vector are prime numbers
void * is_prime(void * ptr) {
  struct threadStruct * ts = (struct threadStruct *) ptr;
  int barrier_ret;
  // int offset = ts->offset;                                             // Removed unnecessary new variable, makes it slower?? 
  int64_t divisor;

  // While global variable cancelFlag is false, all threads run the while loop
  while(!cancelFlag) {                   

    // Barrier for serial task section                 
    barrier_ret = pthread_barrier_wait(&barrier);

    // SERIAL TASK -- ONLY LETS ONE THREAD IN
    if (barrier_ret != 0) {                                                               
      if (primeFlag) result.push_back(targetNum);                         // If primeFlag is true, push the value of targetNum (from last iteration) into result

      if (numVector.size() == 0) cancelFlag = true;                       // If size of numVector is 0, set cancelFlag to false

      else {                                                              // Else, set/reset global variables and get new number from vector and decrease size of vector
        done = false;                                                     // done is flag for early exit on calculating prime 
        primeFlag = true;                                                 // primeFlag is global flag for if the current number is prime or not
        targetNum = numVector.back();                           
        numVector.pop_back();
        max = sqrt(targetNum);                                            

        if (targetNum < 2 || targetNum % 2 == 0 || targetNum % 3 == 0)  {                   // If targetNum is less than 2, divisible by 2, or 3, then not prime and skip algo below
          primeFlag = false;
          done = true;
        }

        if (targetNum == 2 || targetNum == 3) {                                     // If targetNum is 2 or 3, set primeFlag and done to true to skip algo below...
          primeFlag = true;
          done = true;
        }
      }
    }

    divisor = ts->offset;                                                             // Sets/resets the divisor for each thread to respective starting point using offset

    // Barrier for parallel task below...
    pthread_barrier_wait(&barrier);        

    // PARALLEL TASK BELOW                                                      
    // Code below will be skipped if cancelFlag is set to true or done...
    // Algorithm from original detectPrimes but modified 
    // Checks if divisor is less than or equal to max and then checks if it or it +2 can divide targetNum
    // If so, then primeFlag to false, and done to true. --- if done is true, other threads should stop checking it...
    while(!done && !cancelFlag) {                                             
      if (divisor <= max) {
        // printf("Thread is: %i   Divisor Value is: %li\n", offset, divisor);                    // debug test print
        if (targetNum % divisor == 0 || targetNum % (divisor + 2) == 0) {
          primeFlag = false;
          done = true;
          break;
        }
        divisor += totalThreads * 6;                                   // Increments divisor by number of threads times 6.
      }
      else break;                                 // If divisor for thread is greater than max, then break. This only occurs if thread never sets primeFlag to false.
    }
  }
  return NULL;
}



std::vector<int64_t>
detect_primes(const std::vector<int64_t> & nums, int n_threads)
{
  
  // Allocate memory for pthreads and n_threads
  pthread_t * th = (pthread_t *) malloc (n_threads * sizeof(pthread_t));
  struct threadStruct * targs = (struct threadStruct *) malloc(n_threads * sizeof(struct threadStruct));
  numVector = nums;
  totalThreads = n_threads;

  // Initialize barrier
  pthread_barrier_init(&barrier,NULL, n_threads);

  // Thread creation 
  for (int i = 0; i < n_threads; i++) {
    targs[i].offset = i * 6 + 5;                                                              // Initializing with precalculated offset is faster...?
    if (pthread_create(&th[i], NULL, &is_prime, &targs[i]) != 0) std::cout << "Failed to Create Threads\n";
  }

  // Thread join
  for (int i = 0; i < n_threads; i++) {
    if (pthread_join(th[i], NULL) != 0) std::cout << "Failed to Join Threads\n";
  }

  // Destroy barrier
  pthread_barrier_destroy(&barrier);

  // Free allocated memory
  free(th);
  free(targs);
  
  return result;
}
