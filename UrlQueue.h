#pragma once

#include <queue>

#include "../utils/vector.h"
#include "../utils/ParsedUrl.h"
#include "../utils/string.h"
#include "../utils/HashTable.h"
#include "../ranker/StaticRanker.h"

#include "DosProtector.h"


// Data Structure that abstracts random K access to a queue of URLs
class UrlQueue {
    
    private:
        vector<string> urls;

        //? CAN THIS BE A VECTOR ?
        //* YUH BECAUSE ORDERING DOESN:T MATTER IF ITS ALREADY IIN THE POOL
        std::priority_queue<string, std::vector<string>, StaticRanker> urlPool;

        DosProtector dosProtector;
        
        static constexpr size_t MAX_POOL_CANDIDATES = 500;
        static constexpr size_t MAX_POOL_SIZE = 100;

        void fillUrlPool() {
            // select random K urls from urls and add them to urlPool
            
            // TODO: select N links and statically rank them to select the top K

            const int k = std::min(urls.size(), MAX_POOL_SIZE);
            const int N = std::min(urls.size(), MAX_POOL_CANDIDATES);


            unsigned int count = 0;

            while (count < N) {
                const unsigned int randomIndex = rand() % urls.size();
                const string& selectedUrl = urls[randomIndex];
                
                if (!dosProtector.isRequestAllowed(selectedUrl)) {
                    continue; // skip this URL if the request is not allowed
                }

                count++;


                if (urlPool.size() < MAX_POOL_SIZE) {
                    urlPool.push(selectedUrl);
                   
                } else if (StaticRanker()(selectedUrl, urlPool.top())) {

                    urls.push_back(urlPool.top()); // Add the worst-ranked URL back to the pool

                    urlPool.pop();         // Remove worst-ranked from the pool
                    urlPool.push(selectedUrl);     // Insert better one
                } else {
                    continue; // skip this URL if it is not better than the worst-ranked one
                }

                 // swap the selected url with the last url in the vector to efficiently remote it
                 std::swap(urls[randomIndex], urls[urls.size() - 1]);
                 urls.popBack(); 


            }

            
            

        }

    public:

        vector<string> *getUrls() {
            return &urls;
        }

        UrlQueue() = default;

        string getUrlAndPop() {
            string s = urls.back();
            urls.popBack();
            dosProtector.updateRequestTime(s);
            return s;
        }

        void addUrl(const string &url) {
            urls.push_back(url);
        }

        string getNextUrl() {

            if (urlPool.empty() and urls.empty()) {
                throw std::runtime_error("No URLs available");
            }

            if (urlPool.empty()) {
                fillUrlPool();
            }

            string nextUrl = urlPool.top();
            urlPool.pop();
            return nextUrl;
        }

        inline bool empty() const {
            return urls.empty() && urlPool.empty();
        }

        inline bool vecempty() const {
            return urls.empty();
        }

};

