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
#include "../distrib/URLForwarder.h"
#include "../distrib/URLReceiver.h"


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

        unsigned int numNodes;
        unsigned int id;

        UrlForwarder urlForwarder;
        std::unique_ptr<UrlReceiver[]> urlReceivers;  


    public:

        ThreadSafeFrontier() : bloom_filter(false), returnEmpty(false)
        {
            // pthread_mutex_init(&lock, NULL);
        }

        ThreadSafeFrontier(const unsigned int numNodes, const unsigned int id) : 
            bloom_filter(true), 
            returnEmpty(false),
            numNodes(numNodes),
            id(id),
            urlForwarder(numNodes, id), 
            urlReceivers(new UrlReceiver[numNodes]) 
        {
            for (size_t i = 0; i < numNodes; i++) {
                if(i == id) {
                    new (&urlReceivers[i]) UrlReceiver(i, 8080, this);
                } else {
                    // urlReceivers.push_back(nullptr);
                }
            }
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
            while (!frontier_queue.vecempty() && count < MAX_DOC) {
                if (rand() % factor == 0)  {
                    string s = frontier_queue.getUrlAndPop() + endl;
                    fputs(s.c_str(), file);
                    count++;
                }
            }

            fclose(file);

            return bloom_filter.writeBloomFilter();
            
        }

        int buildBloomFilter( const char * path ) {
            return bloom_filter.buildBloomFilter( path );
        }

        int buildFrontier( const char * fpath, const char * bfpath ) {
            HashTable<string, int> weights;
            FILE *file = fopen(fpath, "r");
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

            return buildBloomFilter(bfpath);
        }

        bool contains( const string &s ) 
            {
                WithWriteLock wl(rw_lock); 
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

                const int urlOwner = urlForwarder.addUrl(s);
                
                if (urlOwner == id) {
                    insertWithutForwarding(s);
                }

            }
        }

        inline void insertWithutForwarding(const string &s) {
            {

                // !WARNING: THIS IS NOT THREAD SAFE, DO NOT CALL OUTSIDE OF INSERT

                if ( !bloom_filter.contains(s) ) {
                    frontier_queue.addUrl(s);
                    bloom_filter.insert(s);
                    pthread_cond_signal(&cv);
                }
            }

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

