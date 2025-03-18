#pragma once

// #include <queue>
#include "BloomFilter.h"

#include "ReaderWriterLock.h"
#include <pthread.h>
#include "frontier/UrlQueue.h"


class ThreadSafeFrontier {

    private:
        // std::queue<std::string> frontier_queue;
        UrlQueue frontier_queue; 
        Bloomfilter bloom_filter;

        // pthread_mutex_t lock;
        ReaderWriterLock rw_lock;
        pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

    public:
        ThreadSafeFrontier( int num_objects, double false_positive_rate ) : 
        frontier_queue(), bloom_filter(num_objects, false_positive_rate), rw_lock() { 
            // pthread_mutex_init(&lock, nullptr); 
            // rw_lock = ReaderWriterLock();
        }

        void insert( const std::string &s ) {
            {
                WithWriteLock wl(rw_lock); 
                
                if ( !bloom_filter.contains(s) ) {
                    // frontier_queue.push(s);
                    frontier_queue.addUrl(s);
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

                // std::string url = frontier_queue.front();
                // frontier_queue.pop();
                std::string url = frontier_queue.getNextUrl();
                return url;
            }
        }

        ~ThreadSafeFrontier() {
            // pthread_mutex_destroy(&lock);
        }

};

