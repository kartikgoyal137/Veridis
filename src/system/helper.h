#include <string>
#include <filesystem>
#include <cstdint>
#include <sys/types.h>

namespace fs = std::filesystem;

#ifndef HELPER_H
#define HELPER_H

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

void logger(LogLevel level, const std::string& message);

void write_file(const fs::path& path, const std::string& value);
uint64_t read_uint64(const fs::path& path);
std::string process_name(pid_t pid);
std::string process_user(pid_t pid);

#endif
