#include <iostream>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "../system/cgroup.hpp"
#include "monitor.hpp"

class EBPF {
private:
  struct bpf_object *obj;
  struct bpf_program *prog;
  struct bpf_link *link;

public:
  struct bpf_map *map;
  int map_fd;
  
EBPF() {

  this->obj = bpf_object__open_file("./target/veridis.bpf.o", nullptr);
  if(!obj) {
    std::cerr << "Failed to open BPF bytecode\n";
    return 1;
  }

  if(bpf_object__load(this->obj)) {
    std::cerr << "Failed to load BPF bytecode\n";
    return 1;
  }

  this->map = bpf_object__find_map_by_name(obj, "cpu_time");
  if (!map) {
    std::cerr << "Map cpu_time not found\n";
    return 1;
  }
  this->map_fd = bpf_map__fd(this->map);

  this->prog = bpf_object__find_program_by_name(this->obj, "handle_sched_switch");
  if(!prog) {
    std::cerr << "Program not found\n";
    return 1;
  }

  this->link = bpf_program__attach(this->prog);
  if(!link) {
    std::cerr << "Failed to attach\n";
    return 1;
  }

  std::cout << "eBPF program running";
}

void fill_map(std::vector<std::pair<uint32_t, uint64_t>>& usage_data) { 
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
 
};
