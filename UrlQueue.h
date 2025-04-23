#pragma once

#include <queue>

#include <vector>
#include "../utils/ParsedUrl.h"
#include "../utils/searchstring.h"
#include "../utils/HashTable.h"
#include "../ranker/StaticRanker.h"


// Data Structure that abstracts random K access to a queue of URLs
class UrlQueue {
    
    private:
        std::vector<string> urls;

        std::priority_queue<string, std::vector<string>, StaticRanker> urlPool;

        
        static constexpr size_t MAX_POOL_CANDIDATES = 20000;
        static constexpr size_t MAX_POOL_SIZE = 5000;

        void fillUrlPool() {
            // select random K urls from urls and add them to urlPool
            

            const size_t k = std::min(urls.size(), MAX_POOL_SIZE);
            const size_t N = std::min(urls.size(), MAX_POOL_CANDIDATES);


            unsigned int count = 0;

            while (count < N) {
                const unsigned int randomIndex = rand() % urls.size();
                const string& selectedUrl = urls[randomIndex];
                string curr = ParsedUrl(selectedUrl).Host;
               

                count++;


                urlPool.push(selectedUrl);

                 // swap the selected url with the last url in the vector to efficiently remote it
                std::swap(urls[randomIndex], urls[urls.size() - 1]);
                urls.pop_back(); 
            }

            for (int i = 0; i < (N - k); i++) {
                string top = urlPool.top();
                urls.push_back(top);
                urlPool.pop();
            }
                
            // remaining 5000 urls are sorted in reverse
        }

    public:

        std::vector<string> *getUrls() {
            return &urls;
        }

        UrlQueue() = default;

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

            string nextUrl;

            nextUrl = urlPool.top();
            urlPool.pop();

            return nextUrl;
        }

        inline bool empty() const {
            return urls.empty() and urlPool.empty();
        }

        inline bool vecempty() const {
            return urls.empty();
        }

        inline size_t size() const {
            return (urls.size() + urlPool.size());
        }

        string &at(int i) {
            return urls[i];
        }

        inline void erase(int i) {
            std::swap(urls[urls.size() - 1], urls[i]);
            urls.pop_back();
        }

};

