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

class Cgroup {
  fs::path path;
public:
  Cgroup (const fs::path p) {
    this->path = p;

    if(!fs::exists(path)) {
      this->create();
    }

  }

  void create() {
    fs::create_directory(this->path);
  }

  void enable_controllers(const std::string& controllers) {
    write_file(path/"cgroup.subtree_control", controllers);
  }

  Cgroup create_child(const std::string& name) {
    fs::path child_path = this->path / name;
    return Cgroup(child_path);
  }

  void set_memory_limit(const std::string& bytes) {
    write_file(path / "memory.max" , bytes);
  }

  void set_cpu_limit(const std::string &quota, const std::string& period) {
    write_file(path / "cpu.max", quota + " " + period);
  }

  void set_pids_limit(const std::string& max_pids) {
    write_file(path / "pids.max", max_pids);
  }

  void add_process(pid_t pid) {
    write_file(path / "cgroup.procs" , std::to_string(pid) );
  }

  void remove() {
    if(fs::remove_all(path)) {
      std::cout << "directory removed\n";
    }
    else {
      std::cout << "not found\n";
    }
  }

};


