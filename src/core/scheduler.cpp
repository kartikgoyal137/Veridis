#include <iostream>
#include <vector>
#include <csignal>
#include <atomic>
#include "../system/cgroup.hpp"
#include "monitor.hpp" 

volatile std::sig_atomic_t running = 1;

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
        usage_data.reserve(1024); 

        EBPF obj;         
        Cgroup root(fs::path(CGROUP_ROOT) / "veridis");
        root.enable_controllers("+cpu +memory +pids");

        Cgroup job = root.create_child("bad_jobs");
        job.set_memory_limit("524288000");
        job.set_cpu_limit("50000", "100000");
        job.set_pids_limit("128");

        std::cout << "Scheduler Active " 
                  << CPU_THRESHOLD_MS << "ms/sec." << std::endl;

        while (running) {
            obj.fill_map(usage_data);

            int bad_count = 0;
            for (const auto& entry : usage_data) {
                pid_t pid = static_cast<pid_t>(entry.first);
                uint64_t time_ns = entry.second;

                if (time_ns > (CPU_THRESHOLD_MS * NS_CONVERSION)) {
                    job.add_process(pid);
                    bad_count++;
                }
            }
            
            if (bad_count > 0) {
                 std::cout << "Throttled " << bad_count << " processes this cycle." << std::endl;
            }

            sleep(1);
        }

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return 1;
    }
    
    job.remove();
    std::cout << "Exiting gracefully." << std::endl;
    return 0;
}
