#include <filesystem>
#include <fstream>
#include <cstdint>
#include "helper.h"
#include <string>
#include <sys/types.h>
#include <pwd.h>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem; 
  
void write_file(const fs::path& path, const std::string& value) {
  std::ofstream file(path);
  if(!file)
    throw std::runtime_error("Failed to open: "+ path.string());
  file << value;
}


void logger(LogLevel level, const std::string& message) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::string levelStr;
    std::ostream* out = &std::cout;

    switch(level) {
        case LogLevel::DEBUG: 
            levelStr = "[DEBUG]"; 
            break;
        case LogLevel::INFO:  
            levelStr = "[INFO] "; 
            break;
        case LogLevel::WARN:  
            levelStr = "[WARN] "; 
            out = &std::cerr; 
            break; 
        case LogLevel::ERROR: 
            levelStr = "[ERROR]"; 
            out = &std::cerr; 
            break;
    }

    *out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " " 
         << levelStr << " " << message << std::endl;
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
  if (std::getline(file, name)) {
      name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c) { 
          return std::iscntrl(c); 
      }), name.end());

      auto first = name.find_first_not_of(" \t\n\r");
      if (first != std::string::npos) {
          auto last = name.find_last_not_of(" \t\n\r");
          return name.substr(first, (last - first + 1));
      }
  }

  // Fallback to /proc/pid/stat for the process name in parentheses if comm is empty
  fs::path p_stat = "/proc/"+std::to_string(pid)+"/stat";
  std::ifstream f_stat(p_stat);
  std::string stat_line;
  if(std::getline(f_stat, stat_line)) {
      size_t start = stat_line.find('(');
      size_t end = stat_line.find_last_of(')');
      if(start != std::string::npos && end != std::string::npos && end > start) {
          std::string n = stat_line.substr(start + 1, end - start - 1);
          if(!n.empty()) return n;
      }
  }

  return "[unknown]";
}

std::string process_user(pid_t pid) {
  fs::path p = "/proc/"+std::to_string(pid)+"/status";
  std::ifstream file(p);
  if(!file.is_open()) return "root";

  std::string line;
  while(std::getline(file,line)) {
    if(line.rfind("Uid:", 0)==0) {
      try {
          uid_t uid = std::stoi(line.substr(4));
          struct passwd *pw = getpwuid(uid);
          return pw ? pw->pw_name : std::to_string(uid);
      } catch (...) { return "root"; }
    }
  }

  return "root";
}
