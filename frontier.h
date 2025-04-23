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
#include <memory>


#include "../threading/ThreadPool.h"

const static int MAX_HOST = 300;
//const static int WRITE_TURNOVER = 500000;
const static int MAX_WRITE = 1000000;

class ThreadSafeFrontier {
    
    private:
        UrlQueue frontier_queue; 

        ReaderWriterLock rw_lock;
        pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

        std::atomic<bool> returnEmpty;
        
        string fpath;

        unsigned int numNodes;
        unsigned int selfId;

        std::atomic<size_t> runningCount = 0;

        UrlForwarder urlForwarder;


    public:

        ThreadSafeFrontier() : 
        returnEmpty(false)
        {}

        ThreadSafeFrontier(const unsigned int numNodes, const unsigned int id) : 
            returnEmpty(false),
            numNodes(numNodes),
            selfId(id),
            urlForwarder(numNodes, id)
        {
            
        }

        int writeFrontier() {
            WithWriteLock wl(rw_lock); 
            std::cout << "writing frontier" << std::endl;

            int fd = open(fpath.c_str(), O_TRUNC | O_RDWR);
            if (fd == -1) {
               std::cerr << "Error opening file";
               return 1;
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
               perror("Error getting file size");
               close(fd);
               return 1;
            }
            int fsize = sb.st_size;

            string endl("\n");
            int count = 0;
            std::vector<string> *urls = frontier_queue.getUrls();

            for (int i = 0; i < urls->size(); i++) {
                string s = frontier_queue.at(i);
                s += endl;
                write(fd, s.c_str(), s.length());       
            }
            std::cout << "Finished writing to frontier!" << std::endl;

            close(fd);

            return urlForwarder.getBloomFilter(selfId).writeBloomFilter();
            
        }

        int buildBloomFilter( const char * path ) {
            return urlForwarder.getBloomFilter(selfId).buildBloomFilter( path );
        }

        int buildFrontier( const char * fpath_in, const char * bfpath ) {
            fpath = string(fpath_in);
            std::ifstream file(fpath.c_str());
            std::string line;
            if (file.is_open()) {
                while (std::getline(file, line)) {
                    if (line.size() > 8)
                        insert(string(line.c_str()));
                }
                file.close();
            } else {
                std::cerr << "Unable to open file" << std::endl;
                return 1;
            }
            return buildBloomFilter(bfpath);
        }

        inline bool contains( const string &s ) 
            {
                return urlForwarder.getBloomFilter(selfId).contains(s);
            }

        inline void blacklist( const string &s ) 
            {
                urlForwarder.getBloomFilter(selfId).insert(s);
            }

        inline void insert( const string &s ) {
            {
                WithWriteLock wl(rw_lock); 

                // std::cout << "inserting: " << s << std::endl;

                const auto [urlOwner, alreadySeen] = urlForwarder.addUrl(s);
                
                if (urlOwner == selfId && alreadySeen == false) {
                    frontier_queue.addUrl(s);
                    pthread_cond_signal(&cv);
                }

            }
        }

        // inline void insertWithutForwarding(const string &s) {
        //     {

        //         // !WARNING: THIS IS NOT THREAD SAFE, DO NOT CALL OUTSIDE OF INSERT


        //         if ( bloom_filter.contains(s) == false) {
        //             frontier_queue.addUrl(s);
        //             bloom_filter.insert(s);
        //             pthread_cond_signal(&cv);
        //         }
        //     }

        // }


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
                        // std::cout << "waiting because frontier queue is empty and is not returning" << std::endl;
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

        ~ThreadSafeFrontier() = default;

        uint32_t size() {
            return frontier_queue.size();
        }

};

