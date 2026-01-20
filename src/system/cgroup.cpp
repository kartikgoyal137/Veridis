#include<fstream>
#include<filesystem>
#include<string>
#include<unistd.h>
#include<stdexcept>
#include<iostream>
#include"helper.h"

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

};

int main(int argc, char* argv[]) {
  int target_pid = std::stoi(argv[1]);

  try {
    Cgroup root(fs::path(CGROUP_ROOT)/"veridis");
    root.enable_controllers("+cpu +memory +pids");

    Cgroup job = root.create_child("job1"); //get it from CLI argument
    job.set_memory_limit("524288000");
    job.set_cpu_limit("50000", "100000");
    job.set_pids_limit("64");
    job.add_process(target_pid);

    std::cin.get();

  }
  catch (const std::exception& e) {
    std::cerr << "ERROR" << e.what() << std::endl;
    return 1;
  }

  return 0;

}
