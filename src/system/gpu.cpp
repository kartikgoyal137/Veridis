#include "gpu.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <map>

std::string GpuMonitor::exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

GpuMonitor::GpuMonitor() {
    std::string res = exec("command -v nvidia-smi");
    has_nvidia = !res.empty();
    if (has_nvidia) {
        std::string def_lim = exec("nvidia-smi --query-gpu=power.default_limit --format=csv,noheader,nounits");
        try {
            default_limit = std::stod(def_lim);
        } catch (...) {
            default_limit = 0;
        }
    } else {
        default_limit = 0;
    }
}

GpuStats GpuMonitor::get_stats() {
    GpuStats stats = {0, 0, default_limit, default_limit};
    if (!has_nvidia) return stats;

    std::string res = exec("nvidia-smi --query-gpu=power.draw,utilization.gpu,power.limit --format=csv,noheader,nounits");
    std::stringstream ss(res);
    std::string line;
    if (std::getline(ss, line)) {
        size_t first = line.find(',');
        size_t second = line.find(',', first + 1);
        try {
            stats.power_w = std::stod(line.substr(0, first));
            stats.utilization = std::stoi(line.substr(first + 1, second - first - 1));
            stats.current_limit = std::stod(line.substr(second + 1));
        } catch (...) {}
    }
    return stats;
}

void GpuMonitor::set_power_limit(double watts) {
    if (!has_nvidia) return;
    std::string cmd = "nvidia-smi -pl " + std::to_string((int)watts);
    exec(cmd.c_str());
}

void GpuMonitor::reset_power_limit() {
    if (!has_nvidia || default_limit <= 0) return;
    set_power_limit(default_limit);
}

std::map<int, double> GpuMonitor::get_process_utilization() {
    std::map<int, double> procs;
    if (!has_nvidia) return procs;

    std::string res = exec("nvidia-smi --query-compute-apps=pid,used_memory --format=csv,noheader,nounits");
    std::stringstream ss(res);
    std::string line;
    while (std::getline(ss, line)) {
        size_t sep = line.find(',');
        if (sep != std::string::npos) {
            try {
                int pid = std::stoi(line.substr(0, sep));
                double mem = std::stod(line.substr(sep + 1));
                procs[pid] = mem; // Using memory as a proxy for "importance" 
            } catch (...) {}
        }
    }
    return procs;
}
