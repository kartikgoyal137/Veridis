#include <iostream>
#include <unistd.h>
#include <bpf/libbpf.h>

int main() {
  struct bpf_object *obj;
  struct bpf_program *prog;
  struct bpf_link *link;

  obj = bpf_object__open_file("./target/veridis.bpf.o", nullptr);
  if(!obj) {
    std::cerr << "Failed to open BPF bytecode\n";
    return 1;
  }

  if(bpf_object__load(obj)) {
    std::cerr << "Failed to load BPF bytecode\n";
    return 1;
  }

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
  pause();

  bpf_link__destroy(link);
  bpf_object__close(obj);
}
