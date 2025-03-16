#pragma once

#include <queue>
#include "BloomFilter.h"

#include "ReaderWriterLock.h"
#include <pthread.h>



class ThreadSafeFrontier {

    private:
        std::queue<std::string> frontier_queue; 
        Bloomfilter bloom_filter;

        // pthread_mutex_t lock;
        ReaderWriterLock rw_lock;
        pthread_cond_t cv;

    public:
        ThreadSafeFrontier( int num_objects, double false_positive_rate ) : bloom_filter(num_objects, false_positive_rate) { 
            // pthread_mutex_init(&lock, nullptr); 
            rw_lock = ReaderWriterLock();
        }

        void insert( const std::string &s ) {
            {
                WithWriteLock wl(rw_lock); 
                
                if ( !bloom_filter.contains(s) ) {
                    frontier_queue.push(s);
                    bloom_filter.insert(s);
                }
            }

        }

        std::string getNextURLorWait() {
            {
                WithWriteLock wl(rw_lock);

                // check that thread is still alive
                while ( frontier_queue.empty()) {
                    pthread_cond_wait(&cv, &rw_lock.write_lock); // Wait for a URL to be available
                }

                std::string url = frontier_queue.front();
                frontier_queue.pop();
                return url;
            }
        }

        ~ThreadSafeFrontier() {
            // pthread_mutex_destroy(&lock);
        }

};

