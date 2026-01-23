#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Config {
    std::string cgroup_root;
    double power_limit_soft;
    double power_limit_hard;
    uint64_t cpu_threshold_ms;
    std::set<std::string> users;
    std::set<std::string> whitelist;
    int probation_cycles;

    static Config load(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Could not open config file");
        
        json data = json::parse(f);
        Config cfg;
        
        cfg.cgroup_root = data.value("cgroup_root", "/sys/fs/cgroup/");
        cfg.power_limit_soft = data.value("power_limit_soft", 15.0);
        cfg.power_limit_hard = data.value("power_limit_hard", 30.0);
        cfg.cpu_threshold_ms = data.value("cpu_threshold_ms", 500);
        cfg.probation_cycles = data.value("probation_cycles", 5);
        
        for(const auto& u : data["monitored_users"]) cfg.users.insert(u.get<std::string>());
        for(const auto& w : data["whitelist"]) cfg.whitelist.insert(w.get<std::string>());
        
        return cfg;
    }
};
