#pragma once

// #include <queue>
#include "BloomFilter.h"

#include "ReaderWriterLock.h"
#include <pthread.h>
#include "UrlQueue.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atomic>

const static int MAX_HOST = 300;
const static int MAX_DOC = 300000;

class ThreadSafeFrontier {
    
    private:
        // std::queue<string> frontier_queue;
        UrlQueue frontier_queue; 
        Bloomfilter bloom_filter;

        // pthread_mutex_t lock;
        ReaderWriterLock rw_lock;
        pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

        std::atomic<bool> returnEmpty;
        
        
    // urlForwarder(numNodes, id, NUM_OBJECTS, ERROR_RATE),
        //UrlForwarder urlForwarder;


    public:

        ThreadSafeFrontier() : bloom_filter(1, 0.1), returnEmpty(false) {
        }

        ThreadSafeFrontier( int num_objects, double false_positive_rate ) : 
         bloom_filter(num_objects, false_positive_rate), returnEmpty(false) {
        }

        bool empty() {
            WithReadLock wl(rw_lock);
            return frontier_queue.empty();
        }

        int writeFrontier(int factor) {
            WithWriteLock wl(rw_lock); 
            FILE *file = fopen("./log/frontier/list", "w+");
            if (file == NULL) {
                perror("Error opening file");
                return 1;
            }

            string endl("\n");
            int count = 0;
            while (!frontier_queue.empty() && count < MAX_DOC) {
                if (rand() % factor == 0)  {
                    string s = frontier_queue.getUrlAndPop() + endl;
                    fputs(s.c_str(), file);
                    count++;
                }
            }

            fclose(file);

            bloom_filter.writeBloomFilter();
            
        }

        int buildBloomFilter( const char * path ) {
            return bloom_filter.buildBloomFilter( path );
        }

        int buildFrontier( const char * path ) {
            HashTable<string, int> weights;
            FILE *file = fopen(path, "r");
            if (file == NULL) {
                perror("Error opening file");
                return 1;
            }

            char *line = NULL;
            size_t len = 0;
            ssize_t read;

            while ((read = getline(&line, &len, file)) != -1) {
                string s(line);
                s = s.substr(0, s.size() - 1);
                auto *i = weights.Find(ParsedUrl(s).Host, 0);
                if (i->value < MAX_HOST && s.size() > 0) {
                    insert(s); 
                    ++i->value;
                }   
            }

            free(line); // ? this is never allocated
            fclose(file);
            return 0;
        }

        bool contains( const string &s ) 
            {
                WithReadLock rl(rw_lock); 
                return bloom_filter.contains(s);
            }

        void blacklist( const string &s ) 
            {
                WithWriteLock wl(rw_lock); 
                bloom_filter.insert(s);
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

        void reset() {
            WithWriteLock wl(rw_lock);
            frontier_queue.clearList(true);
        }

        void startReturningEmpty() {
            returnEmpty = true;
            pthread_cond_broadcast(&cv); // Notify all waiting threads
        }

        void stopReturningEmpty() {
            returnEmpty = false;
        }

        string getNextURLorWait() {
            {
                
                if (returnEmpty) {
                    return "";
                }

                WithWriteLock wl(rw_lock);

                // check that thread is still alive
                while ( frontier_queue.empty() and returnEmpty == false) {
                    if (!returnEmpty) {
                        std::cout << "waiting because frontier queue is empty and is not returning" << std::endl;
                    }
                    pthread_cond_wait(&cv, &rw_lock.write_lock); // Wait for a URL to be available
                }

                if (returnEmpty) {
                    return "";
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

