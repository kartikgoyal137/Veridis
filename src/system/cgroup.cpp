#include<fstream>
#include<filesystem>
#include<string>
#include<unistd.h>
#include<stdexcept>
#include<iostream>
#include"helper.h"
#include"cgroup.hpp"

namespace fs = std::filesystem;

const std::string CGROUP_ROOT = "/sys/fs/cgroup";


Cgroup::Cgroup (const fs::path p) {
    this->path = p;

    if(!fs::exists(path)) {
      this->create();
    }

}

void Cgroup::create() {
    fs::create_directory(this->path);
  }

void Cgroup::enable_controllers(const std::string& controllers) {
    write_file(path/"cgroup.subtree_control", controllers);
  }

Cgroup Cgroup::create_child(const std::string& name) {
    fs::path child_path = this->path / name;
    return Cgroup(child_path);
  }

void Cgroup::set_memory_limit(const std::string& bytes) {
    write_file(path / "memory.max" , bytes);
  }

void Cgroup::set_cpu_limit(const std::string &quota, const std::string& period) {
    write_file(path / "cpu.max", quota + " " + period);
  }

void Cgroup::set_pids_limit(const std::string& max_pids) {
    write_file(path / "pids.max", max_pids);
  }

void Cgroup::add_process(pid_t pid) {
    write_file(path / "cgroup.procs" , std::to_string(pid) );
  }

Cgroup::~Cgroup() {
    try {
       if (fs::exists(this->path)) {
            std::ifstream procs_file(this->path / "cgroup.procs");
            std::string pid;
            while (std::getline(procs_file, pid)) {
                try {
                    write_file(fs::path("/sys/fs/cgroup/cgroup.procs"), pid);
                } catch (...) {
                }
            }
            procs_file.close();

            if (fs::remove_all(this->path)) {
                std::cout << "Successfully removed cgroup: " << this->path << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Cleanup failed for " << this->path << ": " << e.what() << std::endl;
    }
}
