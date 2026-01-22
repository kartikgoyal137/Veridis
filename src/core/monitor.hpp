#ifndef EBPF_HPP
#define EBPF_HPP

#include <iostream>
#include <unistd.h>
#include <vector>
#include <stdexcept> 
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <map>

class EBPF {
private:
    struct bpf_object *obj = nullptr;
    struct bpf_program *prog = nullptr;
    struct bpf_link *link = nullptr;

public:
    struct bpf_map *map = nullptr;
    std::map<uint32_t, uint64_t> last_snapshot;
    int map_fd = -1;

     EBPF() {
        this->obj = bpf_object__open_file("./target/veridis.bpf.o", nullptr);
        if (!this->obj) {
            throw std::runtime_error("Failed to open BPF bytecode: ./target/veridis.bpf.o");
        }

        if (bpf_object__load(this->obj)) {
            bpf_object__close(this->obj);
            throw std::runtime_error("Failed to load BPF bytecode");
        }

        this->map = bpf_object__find_map_by_name(this->obj, "cpu_time");
        if (!this->map) {
            throw std::runtime_error("Map 'cpu_time' not found");
        }
        this->map_fd = bpf_map__fd(this->map);

        this->prog = bpf_object__find_program_by_name(this->obj, "handle_sched_switch");
        if (!this->prog) {
            throw std::runtime_error("Program 'handle_sched_switch' not found");
        }

        this->link = bpf_program__attach(this->prog);
        if (!this->link) {
            throw std::runtime_error("Failed to attach BPF link");
        }

        std::cout << "eBPF program running successfully\n";
    }

    ~EBPF() {
        if (this->link) {
            bpf_link__destroy(this->link);
            std::cout << "BPF link destroyed\n";
        }
        if (this->obj) {
            bpf_object__close(this->obj);
            std::cout << "BPF object closed\n";
        }
    }

    void fill_map(std::vector<std::pair<uint32_t, uint64_t>>& usage_data);
};

#endif 
