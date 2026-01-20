#include <string>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

#ifndef HELPER_H
#define HELPER_H

void write_file(const fs::path& path, const std::string& value);
uint64_t read_uint64(const fs::path& path);

#endif
