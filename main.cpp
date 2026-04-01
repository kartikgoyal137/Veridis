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
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include "src/system/gpu.hpp"
#include "src/system/database.hpp"

using json = nlohmann::json;

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
        
        double total_carbon_used_g = 0.0;
        double total_carbon_saved_g = 0.0;
        double last_intensity = 0.0;

        Rapl rapl = init_rapl();
        logger(LogLevel::INFO, "RAPL ready");

        GpuMonitor gpu_mon;
        CarbonDatabase db("/home/kartik/.veridis/history.db");
        logger(LogLevel::INFO, "GPU and Database ready");

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
                last_intensity = (gstate.intensity > 0) ? gstate.intensity : 450; // Default to 450 if fetch fails
                
                logger(LogLevel::INFO, "Grid: " + gstate.profile + " | " + std::to_string(gstate.intensity) + " gCO2");
                logger(LogLevel::INFO, "Lims: " + std::to_string(pwr_sft) + "W / " + std::to_string(pwr_hrd) + "W");
            }

            pwr = rapl.get_power();
            GpuStats gpu_stats = gpu_mon.get_stats();
            double total_pwr = pwr + gpu_stats.power_w;
            
            ebpf.fill_map(udata);
            
            if(total_pwr > pwr_hrd) {
                job.set_cpu_limit("5000", "100000");
                gpu_mon.set_power_limit(gpu_stats.default_limit * 0.5);
            }
            else if (total_pwr > pwr_sft) {
                job.set_cpu_limit("30000", "100000");
                gpu_mon.set_power_limit(gpu_stats.default_limit * 0.8);
            }
            else {
                job.set_cpu_limit("90000", "100000");
                gpu_mon.reset_power_limit();
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

            logger(LogLevel::INFO, "PWR: " + std::to_string(total_pwr) + " W (CPU: " + std::to_string(pwr) + "W, GPU: " + std::to_string(gpu_stats.power_w) + "W)");
            if (bad_cnt > 0) {
                logger(LogLevel::INFO, "Caught " + std::to_string(bad_cnt) + " procs");
            }
            
            iters++;

            double interval_h = 2.0 / 3600.0;
            double intensity_val = (last_intensity > 0) ? last_intensity : 450.0;
            double carbon_iteration = (total_pwr / 1000.0) * interval_h * intensity_val;
            total_carbon_used_g += carbon_iteration;

            double baseline_w = 30.0; 
            if (total_pwr < baseline_w) {
                total_carbon_saved_g += ((baseline_w - total_pwr) / 1000.0) * interval_h * intensity_val;
            }

            // Persistence Logging (every ~10 iterations/20s)
            if (iters % 10 == 0) {
                db.log_stats(total_carbon_used_g, total_carbon_saved_g);
            }

            // Export JSON State
            json state;
            state["timestamp"] = std::time(nullptr);
            state["power_w"] = total_pwr;
            state["cpu_power_w"] = pwr;
            state["gpu_power_w"] = gpu_stats.power_w;
            state["carbon_intensity"] = last_intensity;
            state["total_carbon_used_g"] = total_carbon_used_g;
            state["total_carbon_saved_g"] = total_carbon_saved_g;
            state["soft_limit"] = pwr_sft;
            state["hard_limit"] = pwr_hrd;
            
            // History
            json hist_json = json::array();
            auto hist = db.get_history(7);
            for (const auto& ds : hist) {
                json d;
                d["date"] = ds.date;
                d["used"] = ds.used_g;
                d["saved"] = ds.saved_g;
                hist_json.push_back(d);
            }
            state["history"] = hist_json;
            
            // Add GPU procs
            json gpu_procs = json::array();
            auto gpu_util = gpu_mon.get_process_utilization();
            for (auto const& [pid, mem] : gpu_util) {
                json gp;
                gp["pid"] = pid;
                gp["name"] = process_name(pid);
                gp["vram_mb"] = mem;
                gpu_procs.push_back(gp);
            }
            state["gpu_processes"] = gpu_procs;
            
            json procs_json = json::array();
            for (const auto& ent : udata) {
                pid_t pid = static_cast<pid_t>(ent.first);
                uint64_t t_ns = ent.second;
                double cpu_percent = (t_ns / 2000000000.0) * 100.0; // 2s interval
                if (cpu_percent < 0.1 && (thrtld.find(pid) == thrtld.end())) continue;

                json p;
                p["pid"] = pid;
                p["name"] = process_name(pid);
                p["user"] = process_user(pid);
                p["cpu_percent"] = cpu_percent;
                p["is_throttled"] = (thrtld.find(pid) != thrtld.end());
                procs_json.push_back(p);
            }
            state["processes"] = procs_json;

            std::ofstream ofs("/tmp/veridis_stats.json");
            if (ofs.is_open()) {
                ofs << std::setw(4) << state << std::endl;
            }

            sleep(2);
        }

    } catch (const std::exception& e) {
        logger(LogLevel::ERROR, std::string("Fatal: ") + e.what());
        return 1;
    }
    
    logger(LogLevel::INFO, "Exit");
    return 0;
}
