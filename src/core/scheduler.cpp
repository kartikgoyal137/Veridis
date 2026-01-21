#include <iostream>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main() {
  struct bpf_object *obj;
  struct bpf_program *prog;
  struct bpf_link *link;
  struct bpf_map *map;

  obj = bpf_object__open_file("./target/veridis.bpf.o", nullptr);
  if(!obj) {
    std::cerr << "Failed to open BPF bytecode\n";
    return 1;
  }

  if(bpf_object__load(obj)) {
    std::cerr << "Failed to load BPF bytecode\n";
    return 1;
  }

  map = bpf_object__find_map_by_name(obj, "cpu_time");
  if (!map) {
    std::cerr << "Map cpu_time not found\n";
    return 1;
  }
  int map_fd = bpf_map__fd(map);

  prog = bpf_object__find_program_by_name(obj, "handle_sched_switch");
  if(!prog) {
    std::cerr << "Program not found\n";
    return 1;
  }

  link = bpf_program__attach(prog);
  if(!link) {
    std::cerr << "Failed to attach\n";
    return 1;
  }

  std::cout << "eBPF program running";

  while(1) {
    sleep(1);
    printf("------------\n");

    uint32_t pid = 0; uint32_t next_pid;
    uint64_t time;

    while(bpf_map_get_next_key(map_fd, &pid, &next_pid) == 0) {
      bpf_map_lookup_elem(map_fd, &next_pid, &time);
      printf("PID %-6u CPU %.3f ms\n",
                   next_pid, time / 1e6);
      pid = next_pid;
    }
  }
  
  bpf_link__destroy(link);
  bpf_object__close(obj);
}
