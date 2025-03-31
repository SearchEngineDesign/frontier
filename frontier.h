#pragma once

// #include <queue>
#include "BloomFilter.h"

#include "ReaderWriterLock.h"
#include <pthread.h>
#include "UrlQueue.h"


class ThreadSafeFrontier {

    private:
        // std::queue<string> frontier_queue;
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

        bool empty() {
            {
                WithReadLock wl(rw_lock);
                return frontier_queue.empty();
            }
        }

        void insert( const string &s ) {
            {
                WithWriteLock wl(rw_lock); 
                
                if ( !bloom_filter.contains(s) ) {
                    frontier_queue.addUrl(s);
                    bloom_filter.insert(s);
                    pthread_cond_signal(&cv);
                }
            }

        }

        string getNextURLorWait() {
            {
                WithWriteLock wl(rw_lock);

                // check that thread is still alive
                while ( frontier_queue.empty()) {
                    pthread_cond_wait(&cv, &rw_lock.write_lock); // Wait for a URL to be available
                }

                // string url = frontier_queue.front();
                // frontier_queue.pop();
                string url = frontier_queue.getNextUrl();
                return url;
            }
        }

        ~ThreadSafeFrontier() {
            // pthread_mutex_destroy(&lock);
        }

};

