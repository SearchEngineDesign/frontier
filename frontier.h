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
#include "../parser/HtmlParser.h"
#include <atomic>
#include <fstream>
#include <unordered_map>

const static int MAX_HOST = 300;
//const static int WRITE_TURNOVER = 500000;
const static int MAX_WRITE = 1000000;

class ThreadSafeFrontier {
    
    private:
        UrlQueue frontier_queue; 
        Bloomfilter bloom_filter;

        // pthread_mutex_t lock;
        ReaderWriterLock rw_lock;
        pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

        std::atomic<bool> returnEmpty;
        
        string fpath;

        unsigned int numNodes;
        unsigned int id;

        std::atomic<size_t> runningCount = 0;

        UrlForwarder urlForwarder;

        // MODIFY THIS TO SELECT FOR CERTAIN SITES
        const char * filter[10] = {
            "en.wikipedia", 
            "stackoverflow", 
            "reddit.com", 
            "Britannica",
            "stanford",
            "archive",
            "jstor",
            "amazon",
            "ebay",
            "gutenberg"};
        bool ignorefilter = true;

        inline bool frontierfilter(const string &s) {
            for (auto &i : filter) {
                if (s.contains(i))
                    return true;
            }
            return false;
        }


        inline void insertNoLock(const string &s) {
            if (ignorefilter || frontierfilter(s)) {
                const auto [urlOwner, alreadySeen] = urlForwarder.addUrl(s);
        
                if (urlOwner == id && !bloom_filter.contains(s)) {
                    bloom_filter.insert(s);
                    frontier_queue.addUrl(s);
                }
            } 
        }


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
            urlForwarder(numNodes, id)
        {
            
        }

        int writeFrontier() {
            std::cout << "writing frontier" << std::endl;
            std::unordered_map<std::string, int> weights;

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
            size_t limit = frontier_queue.size(); //std::min(frontier_queue.size(), MAX_WRITE);

            for (int i = 0; i < limit; i++) {
                string s = frontier_queue.at(i);
                std::string s2 = s.c_str();
                int& weight = weights[s2];  
                s += endl;
                if (weight < MAX_HOST) {
                    write(fd, s.c_str(), s.length());
                    ++weight;
                } else {
                    frontier_queue.erase(i);
                    i--;
                }         
            }

            close(fd);

            return bloom_filter.writeBloomFilter();
            
        }

        int buildBloomFilter( const char * path ) {
            return bloom_filter.buildBloomFilter( path );
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

        bool contains( const string &s ) 
            {
                return bloom_filter.contains(s);
            }

        void blacklist( const string &s ) 
            {
                bloom_filter.insert(s);
            }

        inline void insert( const string &s ) {
            {
                WithWriteLock wl(rw_lock); 
                insertNoLock(s);
                pthread_cond_signal(&cv);
            }
        }

        inline void insertWithoutForward( const string &s ) {
            {
                WithWriteLock wl(rw_lock); 
                if (!bloom_filter.contains(s) == false) {
                    bloom_filter.insert(s);
                    frontier_queue.addUrl(s);
                }
                pthread_cond_signal(&cv);
            }
        }

        void insert( const vector<string> &links ) {
            {
                WithWriteLock wl(rw_lock); 
                for (const auto &link : links) {
                    insertNoLock(link);
                }
                pthread_cond_broadcast(&cv); // Notify all waiting threads
            }
        }

        void insert( const vector<Link> &links ) {
            {
                WithWriteLock wl(rw_lock); 
                for (const auto &link : links) {
                    insertNoLock(link.URL);
                }
                pthread_cond_broadcast(&cv); // Notify all waiting threads
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

        uint32_t size() {
            return frontier_queue.size();
        }

};

