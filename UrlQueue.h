#pragma once

#include <queue>

#include "../utils/include/vector.h"
#include "../utils/ParsedUrl.h"
#include "../utils/include/string.h"

// Data Structure that abstracts random K access to a queue of URLs
class UrlQueue {

    private:
        vector<string> urls;
        // HashTable<int, int> index_map;

        //? CAN THIS BE A VECTOR ?
        std::queue<string> urlPool;

    
        void fillUrlPool() {
            // select random K urls from urls and add them to urlPool
            
            // TODO: select N links and statically rank them to select the top K

            const int k = std::min(urls.size(), DEFAULT_POOL_SIZE);
            
            for (int i = 0; i < k; ++i) {
                // select a random index
                const unsigned int randomIndex = rand() % urls.size();
                string selectedUrl = urls[randomIndex];
                urlPool.push(selectedUrl);

                // swap the selected url with the last url in the vector
                std::swap(urls[randomIndex], urls[urls.size() - 1]);
                urls.popBack(); 
            }
        }

    public:

        static const size_t DEFAULT_POOL_SIZE = 10;

        UrlQueue() : urls(), urlPool() {
            
         }

        void addUrl(const string &url) {
            urls.push_back(url);
        }

        string getNextUrl() {

            // while (urlPool.empty() and urls.empty()) {
                // wait
            // }

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
            return urls.empty() and urlPool.empty();
        }

};

