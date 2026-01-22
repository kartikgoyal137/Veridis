#include <filesystem>
#include <fstream>
#include <cstdint>
#include "helper.h"
#include <string>
#include <sys/types.h>
#include <pwd.h>

namespace fs = std::filesystem; 
  
void write_file(const fs::path& path, const std::string& value) {
  std::ofstream file(path);
  if(!file)
    throw std::runtime_error("Failed to open: "+ path.string());
  file << value;
}

uint64_t read_uint64(const fs::path& path) {
  std::ifstream file(path);
  if(!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path.string() );
  }

  uint64_t value;
  file >> value;

  return value;
}

std::string process_name(pid_t pid) {
  fs::path p = "/proc/"+std::to_string(pid)+"/comm";
  std::ifstream file(p);

  std::string name;
  std::getline(file, name);

  return name;
}

std::string process_user(pid_t pid) {
  fs::path p = "/proc/"+std::to_string(pid)+"/status";
  std::ifstream file(p);
  std::string line;

  while(std::getline(file,line)) {
    if(line.rfind("Uid:", 0)==0) {
      uid_t uid = std::stoi(line.substr(4));
      struct passwd *pw = getpwuid(uid);
      return pw ? pw->pw_name : std::to_string(uid);
    }
  }

  return "user";
}
