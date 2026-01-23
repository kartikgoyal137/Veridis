#include <iostream>
#include <vector>
#include <csignal>
#include <atomic>
#include "src/system/cgroup.hpp"
#include "src/core/monitor.hpp" 
#include "src/system/helper.h"
#include <set>
#include <unistd.h>
#include <string>
#include "src/system/rapl.hpp"
#include "src/core/config.hpp"

volatile std::sig_atomic_t running = 1;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
       running = 0;
    }
}

const uint64_t NS_CONVERSION = 1000000; 

int main() {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);
  
    try {
        std::vector<std::pair<uint32_t, uint64_t>> usage_data;
        Config cfg = Config::load("config.json");
        usage_data.reserve(1024); 

        std::map<pid_t, int> throttled;

        logger(LogLevel::INFO, "Veridis Scheduler starting...");
        logger(LogLevel::INFO, "Config loaded\nCPU Threshold: " + std::to_string(cfg.cpu_threshold_ms) + "ms\n");
        logger(LogLevel::INFO, "POWER_LIMIT_SOFT: " + std::to_string(cfg.power_limit_soft) + "\n");
        logger(LogLevel::INFO, "POWER_LIMIT_HARD: " + std::to_string(cfg.power_limit_hard) + "\n");
        logger(LogLevel::INFO, "PROBATION_CYCLES: " + std::to_string(cfg.probation_cycles) + "\n");


        Rapl rapl = init_rapl();
        logger(LogLevel::INFO, "RAPL Initialized.");

        double power = 0;

        EBPF obj;         
        Cgroup root( cfg.cgroup_root + "veridis");
        root.enable_controllers("+cpu +memory +pids");

        Cgroup job = root.create_child("bad_jobs");
        job.set_memory_limit("524288000");
        job.set_pids_limit("128");


        logger(LogLevel::INFO, "Cgroups configured. Scheduler Loop Active.\n\n");
        while (running) {
            power = rapl.get_power();
            obj.fill_map(usage_data);
            
            if(power > cfg.power_limit_hard) {
              job.set_cpu_limit("5000", "100000");
            }
            else if (power > cfg.power_limit_soft) {
              job.set_cpu_limit("30000", "100000");
            }
            else {
              job.set_cpu_limit("90000", "100000");
            }

            std::set<pid_t> curr_bad_jobs;

            int bad_count = 0;
            for (const auto& entry : usage_data) {
                pid_t pid = static_cast<pid_t>(entry.first);
                uint64_t time_ns = entry.second;

                if (time_ns > (cfg.cpu_threshold_ms * NS_CONVERSION)) {
                    std::string name = process_name(pid);
                    std::string user = process_user(pid);

                    if(cfg.whitelist.find(name) != cfg.whitelist.end()) continue;
                    if(cfg.users.find(user) == cfg.users.end()) continue;
                          
                    curr_bad_jobs.insert(pid);
                        
                    if(throttled.find(pid)==throttled.end()) {
                      logger(LogLevel::INFO, "THROTTLE: " + name + " (PID: " + std::to_string(pid) + ")");
                      job.add_process(pid);
                      throttled[pid] = 0;
                      bad_count++;
                    }
                                         
                }

            for (auto it = throttled.begin(); it != throttled.end(); ) {
                pid_t pid = it->first;
                if (curr_bad_jobs.count(pid)) {
                    it->second = 0; 
                    ++it;
                } 
                else {
                    it->second++; 

                    if (it->second >= cfg.probation_cycles) {
                        try {
                            logger(LogLevel::INFO, "RELEASE: PID " + std::to_string(pid) + "\n");
                            root.add_process(pid); 
                           
                         } catch (const std::exception& e) {
                        logger(LogLevel::ERROR, "Failed to release PID " + std::to_string(pid) + ": " + e.what());
                        }
                        it = throttled.erase(it); 
                    } else {
                        ++it;
                    }
                }
            }}

            std::cout << "CPU Power Used : " << power << " W" <<  std::endl;
            if (bad_count > 0) {
                 std::cout << "Throttled " << bad_count << " processes this cycle." << std::endl;
            }
            std::cout << "\n\n";
            
            sleep(2);
        }

    } catch (const std::exception& e) {
        logger(LogLevel::ERROR, std::string("Fatal Error: ") + e.what());
        return 1;
    }
    
    logger(LogLevel::INFO, "Exiting gracefully.");
    return 0;
}
