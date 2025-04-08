#pragma once

// #include <queue>
#include "BloomFilter.h"

#include "ReaderWriterLock.h"
#include <pthread.h>
#include "UrlQueue.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


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

        int writeFrontier(bool truncate, int factor) {
            FILE *file = fopen("./log/frontier/list", "w+");

            WithWriteLock wl(rw_lock); 
            if (file == NULL) {
                perror("Error opening file");
                return 1;
            }

            string endl("\n");
            frontier_queue.clearList();
            if (!truncate) {
                while (!frontier_queue.empty()) {
                    string s = frontier_queue.getUrlAndPop() + endl;
                    fputs(s.c_str(), file);
                }
                
            } else {
                for (auto &i : *frontier_queue.getUrls()) {
                    string s = i + endl;
                    if (rand() % factor == 0)
                        fputs(s.c_str(), file);
                }     
            }

            fclose(file);

            bloom_filter.writeBloomFilter();
            
        }

        int buildBloomFilter( const char * path ) {
            return bloom_filter.buildBloomFilter( path );
        }

        int buildFrontier( const char * path ) {
            int MAX_HOST = 300;
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
                if (i->value < MAX_HOST) {
                    insert(s); 
                    ++i->value;
                }   
            }

            free(line);
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

