#include <string>

struct CarbonData {
    int intensity;
    std::string profile; 
};

class CarbonMonitor {
private:
    std::string api_key;
    std::string zone;
    int mod_thresh;
    int dirty_thresh;

public:
    CarbonMonitor(std::string key, std::string z, int _mod_thresh, int _dirty_thresh);
    CarbonData fetch_live_intensity();
};
