#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct CarbonThresholds {
    int moderate_gco2;
    int dirty_gco2;
};

struct Config {
    std::string cgroup_root;
    
    std::string grid_zone;
    std::string api_key;
    CarbonThresholds carbon_thresholds;
    std::map<std::string, std::map<std::string, double>> power_profiles;

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
        
        cfg.grid_zone = data.value("grid_zone", "IN-UT");
        cfg.api_key = data.value("carbon_api_key", "");
        
        if (data.contains("carbon_thresholds")) {
            cfg.carbon_thresholds.moderate_gco2 = data["carbon_thresholds"].value("moderate_gco2", 250);
            cfg.carbon_thresholds.dirty_gco2 = data["carbon_thresholds"].value("dirty_gco2", 450);
        } else {
            cfg.carbon_thresholds = {250, 450};
        }

        if (data.contains("power_profiles")) {
            for (auto& [profile, limits] : data["power_profiles"].items()) {
                cfg.power_profiles[profile]["soft"] = limits.value("soft", 15.0);
                cfg.power_profiles[profile]["hard"] = limits.value("hard", 30.0);
            }
        }

        cfg.cpu_threshold_ms = data.value("cpu_threshold_ms", 500);
        cfg.probation_cycles = data.value("probation_cycles", 5);
        
        if (data.contains("monitored_users")) {
            for(const auto& u : data["monitored_users"]) cfg.users.insert(u.get<std::string>());
        }
        if (data.contains("whitelist")) {
            for(const auto& w : data["whitelist"]) cfg.whitelist.insert(w.get<std::string>());
        }
        
        return cfg;
    }
};
