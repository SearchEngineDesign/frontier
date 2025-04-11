#pragma once


#include <chrono>
#include "../utils/HashTable.h"
#include "../utils/string.h"

class DosProtector {

    private:
        static constexpr int TIME_BETWEEN_REQUESTS = 250; // in milliseconds

        HashTable<string, uint64_t> lastRequestTime;


    public:
        DosProtector() = default;

        inline bool isRequestAllowed(const string &url) const {
            auto lastRequest = lastRequestTime.Find(url);

            if (lastRequest == nullptr) {
                return true; // No previous request, allow the request
            }
        
            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
        
            const auto lastRequestTimeInMS = lastRequest->value;
        
            const auto timeSinceLastRequest = timeInMS - lastRequestTimeInMS;
        
            return timeSinceLastRequest >= TIME_BETWEEN_REQUESTS;  
        }

        inline void updateRequestTime(const string &url) {
            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();

            lastRequestTime.Find(url, timeInMS); // Update the last request time

        }

        inline void reset() {
            for (auto i : lastRequestTime) {
                if (i.value > 10000) {}
                    // clean up or something -- no delete function in hashtable yet
            }
        }
};