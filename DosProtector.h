#pragma once


#include <chrono>
#include <unordered_map>

class DosProtector {

    private:
        static constexpr int MIN_TIME_BETWEEN_REQUESTS = 500; // in milliseconds

        static constexpr int REQUESTS_BEFORE_RESET = 5000;

        std::unordered_map<std::string, uint64_t> lastRequestTime;

        inline void probabilistically_reset() {
            const int rng = rand() % REQUESTS_BEFORE_RESET;
            if (rng == 0) {
                lastRequestTime.clear(); // Reset the map
                lastRequestTime.rehash(8192);
            }
        } 

    public:
        DosProtector() = default;

        inline bool isRequestAllowed(const char * url) {
            std::string str(url);

            probabilistically_reset(); 
            
            const auto& lastRequest = lastRequestTime.find(str);;

            if (lastRequest == lastRequestTime.end()) {
                return true; // No previous request, allow the request
            }
        
            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
        
            const auto lastRequestTimeInMS = (*lastRequest).second;
        
            const auto timeSinceLastRequest = timeInMS - lastRequestTimeInMS;
        
            if (timeSinceLastRequest >= MIN_TIME_BETWEEN_REQUESTS) {
                return true;
            } else {
                std::cerr << "Request to " << url << " not allowed" << std::endl;
                return false;
            }
        }

        inline void updateRequestTime(const char * url) {
            std::string str(url);

            const auto currentTime = std::chrono::steady_clock::now().time_since_epoch();
            const auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();

            lastRequestTime[str] = timeInMS; // Update the last request time
        }

};