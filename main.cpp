#include <iostream>
#include <vector>
#include <csignal>
#include <atomic>
#include <set>
#include <unistd.h>
#include <string>
#include "src/system/cgroup.hpp"
#include "src/core/monitor.hpp" 
#include "src/system/helper.h"
#include "src/system/rapl.hpp"
#include "src/core/config.hpp"
#include "src/system/carbon.hpp"

volatile std::sig_atomic_t run = 1;

void sig_hndlr(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
       run = 0;
    }
}

const uint64_t NS_CONV = 1000000; 

int main() {
    std::signal(SIGINT, sig_hndlr);
    std::signal(SIGTERM, sig_hndlr);
  
    try {
        std::vector<std::pair<uint32_t, uint64_t>> udata;
        Config cfg = Config::load("config.json");
        udata.reserve(1024); 

        std::map<pid_t, int> thrtld;

        logger(LogLevel::INFO, "Veridis init");

        CarbonMonitor carb(cfg.api_key, cfg.grid_zone, 
                           cfg.carbon_thresholds.moderate_gco2, 
                           cfg.carbon_thresholds.dirty_gco2);

        double pwr_sft = cfg.power_profiles["moderate"]["soft"];
        double pwr_hrd = cfg.power_profiles["moderate"]["hard"];
    
        int iters = 0;
        const int upd_intvl = 150;

        Rapl rapl = init_rapl();
        logger(LogLevel::INFO, "RAPL ready");

        double pwr = 0;

        EBPF ebpf;         
        Cgroup root(cfg.cgroup_root + "veridis");
        root.enable_controllers("+cpu +memory +pids");

        Cgroup job = root.create_child("bad_jobs");
        job.set_memory_limit("524288000");
        job.set_pids_limit("128");

        logger(LogLevel::INFO, "Loop start");

        while (run) {
            if (iters % upd_intvl == 0) {
                CarbonData gstate = carb.fetch_live_intensity();
                pwr_sft = cfg.power_profiles[gstate.profile]["soft"];
                pwr_hrd = cfg.power_profiles[gstate.profile]["hard"];
                
                logger(LogLevel::INFO, "Grid: " + gstate.profile + " | " + std::to_string(gstate.intensity) + " gCO2");
                logger(LogLevel::INFO, "Lims: " + std::to_string(pwr_sft) + "W / " + std::to_string(pwr_hrd) + "W");
            }

            pwr = rapl.get_power();
            ebpf.fill_map(udata);
            
            if(pwr > pwr_hrd) {
                job.set_cpu_limit("5000", "100000");
            }
            else if (pwr > pwr_sft) {
                job.set_cpu_limit("30000", "100000");
            }
            else {
                job.set_cpu_limit("90000", "100000");
            }

            std::set<pid_t> bad_jobs;
            int bad_cnt = 0;

            for (const auto& ent : udata) {
                pid_t pid = static_cast<pid_t>(ent.first);
                uint64_t t_ns = ent.second;

                if (t_ns > (cfg.cpu_threshold_ms * NS_CONV)) {
                    std::string nm = process_name(pid);
                    std::string usr = process_user(pid);

                    if(cfg.whitelist.find(nm) != cfg.whitelist.end()) continue;
                    if(cfg.users.find(usr) == cfg.users.end()) continue;
                          
                    bad_jobs.insert(pid);
                        
                    if(thrtld.find(pid) == thrtld.end()) {
                        logger(LogLevel::INFO, "THROTTLE: " + nm + " (" + std::to_string(pid) + ")");
                        job.add_process(pid);
                        thrtld[pid] = 0;
                        bad_cnt++;
                    }
                }
            }

            for (auto it = thrtld.begin(); it != thrtld.end(); ) {
                pid_t pid = it->first;
                if (bad_jobs.count(pid)) {
                    it->second = 0; 
                    ++it;
                } 
                else {
                    it->second++; 

                    if (it->second >= cfg.probation_cycles) {
                        try {
                            logger(LogLevel::INFO, "REL: " + std::to_string(pid));
                            root.add_process(pid); 
                        } catch (const std::exception& e) {
                            logger(LogLevel::ERROR, "Fail REL " + std::to_string(pid) + ": " + e.what());
                        }
                        it = thrtld.erase(it); 
                    } else {
                        ++it;
                    }
                }
            }

            logger(LogLevel::INFO, "PWR: " + std::to_string(pwr) + " W");
            if (bad_cnt > 0) {
                logger(LogLevel::INFO, "Caught " + std::to_string(bad_cnt) + " procs");
            }
            
            iters++;
            sleep(2);
        }

    } catch (const std::exception& e) {
        logger(LogLevel::ERROR, std::string("Fatal: ") + e.what());
        return 1;
    }
    
    logger(LogLevel::INFO, "Exit");
    return 0;
}
