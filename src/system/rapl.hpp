#ifndef RAPL_HPP
#define RAPL_HPP

#include <filesystem>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include "helper.h"

namespace fs = std::filesystem;

class Rapl {
public:
  std::string path;
  uint64_t max_energy;

  Rapl(const std::string p, const uint64_t e);
  double get_power();
  uint64_t read_energy_uj();
};

Rapl init_rapl();
fs::path find_rapl_path();

#endif
