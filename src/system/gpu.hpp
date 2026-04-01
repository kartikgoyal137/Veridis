#ifndef GPU_HPP
#define GPU_HPP

#include <string>
#include <vector>
#include <map>

struct GpuStats {
    double power_w;
    int utilization;
    double default_limit;
    double current_limit;
};

class GpuMonitor {
private:
    bool has_nvidia;
    double last_power;
    double default_limit;
    
    std::string exec(const char* cmd);

public:
    GpuMonitor();
    GpuStats get_stats();
    void set_power_limit(double watts);
    void reset_power_limit();
    std::map<int, double> get_process_utilization();
};

#endif
