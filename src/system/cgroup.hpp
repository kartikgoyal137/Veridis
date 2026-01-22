#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class Cgroup {
private:
    fs::path path;
    
public:
    Cgroup(fs::path p);
    ~Cgroup();
    Cgroup(const Cgroup&) = delete; 
    Cgroup& operator=(const Cgroup&) = delete;

    void enable_controllers(const std::string& controllers);
    Cgroup create_child(const std::string& name);
    void set_cpu_limit(const std::string &quota, const std::string& period);
    void set_memory_limit(const std::string& bytes);
    void set_pids_limit(const std::string& max_pids);
    void add_process(int pid);
    void create();
    std::string get_path() const;
};
