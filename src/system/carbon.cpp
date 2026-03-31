#include "carbon.hpp"
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

CarbonMonitor::CarbonMonitor(std::string key, std::string z, int _mod_thresh, int _dirty_thresh) {
    api_key = key;
    zone = z;
    mod_thresh = _mod_thresh;
    dirty_thresh = _dirty_thresh;
}

CarbonData CarbonMonitor::fetch_live_intensity() {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    int current_intensity = -1;
    CarbonData data;

    curl = curl_easy_init();
    if(curl) {
        std::string url = "https://api.electricitymap.org/v3/carbon-intensity/latest?zone=" + zone;
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("auth-token: " + api_key).c_str());
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); 
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "[CarbonMonitor] cURL Error: " << curl_easy_strerror(res) << std::endl;
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            if (http_code == 200) {
                try {
                    json response_json = json::parse(readBuffer);
                    if (response_json.contains("carbonIntensity") && !response_json["carbonIntensity"].is_null()) {
                        current_intensity = response_json["carbonIntensity"].get<int>();
                    } else {
                        std::cerr << "[CarbonMonitor] JSON does not contain 'carbonIntensity'" << std::endl;
                    }
                } catch (json::parse_error& e) {
                    std::cerr << "[CarbonMonitor] JSON Parse Error: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "[CarbonMonitor] HTTP Error Code: " << http_code << "\nResponse: " << readBuffer << std::endl;
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    data.intensity = current_intensity;
    
    if (current_intensity >= dirty_thresh) {
        data.profile = "dirty";
    } else if (current_intensity == -1 || current_intensity >= mod_thresh) {
        data.profile = "moderate";
    } else {
        data.profile = "green";
    }
    
    return data;
}
