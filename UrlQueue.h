#pragma once

#include <queue>

#include "../utils/vector.h"
#include "../utils/ParsedUrl.h"
#include "../utils/string.h"
#include "../utils/HashTable.h"

#include "DosProtector.h"


// Data Structure that abstracts random K access to a queue of URLs
class UrlQueue {
    
    private:
        vector<string> urls;

        //? CAN THIS BE A VECTOR ?
        //* YUH BECAUSE ORDERING DOESN:T MATTER IF ITS ALREADY IIN THE POOL
        std::queue<string> urlPool;

        DosProtector dosProtector;
        

    
        void fillUrlPool() {
            // select random K urls from urls and add them to urlPool
            
            // TODO: select N links and statically rank them to select the top K

            const int k = std::min(urls.size(), DEFAULT_POOL_SIZE);
            
            unsigned int count = 0;

            while (count < k) {
                const unsigned int randomIndex = rand() % urls.size();
                string selectedUrl = urls[randomIndex];
                
                if (!dosProtector.isRequestAllowed(selectedUrl)) {
                    continue; // skip this URL if the request is not allowed
                }

                count++;
                urlPool.push(selectedUrl);
                
                
                // !TODO ADJUSTMENT
                
                // swap the selected url with the last url in the vector
                std::swap(urls[randomIndex], urls[urls.size() - 1]);
                urls.popBack(); 
            }

        }

    public:

        vector<string> *getUrls() {
            return &urls;
        }

        const size_t DEFAULT_POOL_SIZE = 10;

        UrlQueue() = default;

        void clearList(bool clearall) {
            if (clearall)
                urls.clear();
            std::queue<string>().swap(urlPool);
        }

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

            string nextUrl = urlPool.front();
            urlPool.pop();
            return nextUrl;
        }

        inline bool empty() const {
            return urls.empty();
        }

};

