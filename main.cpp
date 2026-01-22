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

volatile std::sig_atomic_t running = 1;

std::string CGROUP_ROOT = "/sys/fs/cgroup/";
const double POWER_LIMIT_SOFT = 15;
const double POWER_LIMIT_HARD = 30;
std::set<std::string> USER = {"kartik"};

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
       running = 0;
    }
}

const uint64_t CPU_THRESHOLD_MS = 500; 
const uint64_t NS_CONVERSION = 1000000; 

int main() {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);
  
    try {
        std::vector<std::pair<uint32_t, uint64_t>> usage_data;
        std::set<std::string> allowed;
        usage_data.reserve(1024); 

        Rapl rapl = init_rapl();
        double power = 0;

        EBPF obj;         
        Cgroup root(fs::path(CGROUP_ROOT) / "veridis");
        root.enable_controllers("+cpu +memory +pids");

        Cgroup job = root.create_child("bad_jobs");
        job.set_memory_limit("524288000");
        job.set_pids_limit("128");

        std::cout << "Scheduler Active " 
                  << CPU_THRESHOLD_MS << "ms/sec." << std::endl;

        while (running) {
            power = rapl.get_power();
            obj.fill_map(usage_data);
            
            if(power > POWER_LIMIT_HARD) {
              job.set_cpu_limit("5000", "100000");
            }
            else if (power > POWER_LIMIT_SOFT) {
              job.set_cpu_limit("30000", "100000");
            }
            else {
              job.set_cpu_limit("90000", "100000");
            }

            int bad_count = 0;
            for (const auto& entry : usage_data) {
                pid_t pid = static_cast<pid_t>(entry.first);
                uint64_t time_ns = entry.second;

                if (time_ns > (CPU_THRESHOLD_MS * NS_CONVERSION)) {
                    std::string name = process_name(pid);
                    std::string user = process_user(pid);
                    if(allowed.find(name)==allowed.end() && USER.find(user) != USER.end()) {
                      job.add_process(pid);
                      bad_count++;
                    }
                }
            }
            std::cout << "CPU Power Used : " << power << " W" <<  std::endl;
            if (bad_count > 0) {
                 std::cout << "Throttled " << bad_count << " processes this cycle." << std::endl;
            }
            std::cout << "\n\n";
            
            sleep(1);
        }

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Exiting gracefully." << std::endl;
    return 0;
}
