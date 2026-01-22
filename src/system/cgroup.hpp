#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class Cgroup {
private:
    fs::path path;
    void write_file(const std::string& filename, const std::string& value);

public:
    Cgroup(fs::path p);
    void enable_controllers(const std::string& controllers);
    Cgroup create_child(const std::string& name);
    void set_cpu_limit(const std::string &quota, const std::string& period);
    void add_process(int pid);
    std::string get_path() const;
};
