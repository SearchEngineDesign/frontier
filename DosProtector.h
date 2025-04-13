#pragma once


#include <chrono>
#include <unordered_map>
#include "../utils/string.h"


class DosProtector {

    private:
        static constexpr int TIME_BETWEEN_REQUESTS = 250; // in milliseconds

        static constexpr int REQUESTS_BEFORE_RESET = 5000;

        std::unordered_map<const char *, uint64_t> lastRequestTime;

        inline void probabilistically_reset() {
            const int rng = rand() % REQUESTS_BEFORE_RESET;
            if (rng == 0) {
                lastRequestTime.clear(); // Reset the map
            }
        } 

    public:
        DosProtector() = default;

        inline bool isRequestAllowed(const string &url) {
            probabilistically_reset(); 
            
            const auto& lastRequest = lastRequestTime.find(url.c_str());;

            if (lastRequest == lastRequestTime.end()) {
                return true; // No previous request, allow the request
            }
        
            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
        
            const auto lastRequestTimeInMS = (*lastRequest).second;
        
            const auto timeSinceLastRequest = timeInMS - lastRequestTimeInMS;
        
            return timeSinceLastRequest >= TIME_BETWEEN_REQUESTS;  
        }

        inline void updateRequestTime(const string &url) {
            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();

            lastRequestTime[url.c_str()] = timeInMS; // Update the last request time
        }

};