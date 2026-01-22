#include <iostream>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "../system/cgroup.hpp"
#include "monitor.hpp"
#include <map>

void EBPF::fill_map(std::vector<std::pair<uint32_t, uint64_t>>& usage_data) { 
    static std::map<uint32_t, uint64_t> last_snapshot;

    usage_data.clear();

    uint32_t pid = 0; uint32_t next_pid;
    uint64_t total_time;

    while(bpf_map_get_next_key(map_fd, &pid, &next_pid) == 0) {
      
      if(bpf_map_lookup_elem(map_fd, &next_pid, &total_time)) {
        
        if(last_snapshot.find(next_pid) == last_snapshot.end()) {
          last_snapshot[next_pid] = total_time;
          pid = next_pid;
          continue;
        }

        uint64_t delta = total_time - last_snapshot[next_pid];
        last_snapshot[next_pid] = total_time;
        if( delta > 0 ) usage_data.emplace_back(next_pid, delta);
        pid = next_pid;
      }

    }
}

