#include <filesystem>
#include <fstream>
#include <cstdint>
#include <helper.h>

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
