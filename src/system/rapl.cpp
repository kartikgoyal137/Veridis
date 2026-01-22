#include <filesystem>
#include <string>
#include <chrono>
#include <thread>
#include "helper.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

class Rapl {
public:
  std::string path;
  uint64_t max_energy;

  Rapl (const std::string p, const uint64_t e) {
  this->path = p;
  this->max_energy = e;
  }

  uint64_t read_energy_uj();
  double get_power();
};

fs::path find_rapl_path() {
  const fs::path base{"/sys/class/powercap/intel-rapl"};

  if(!fs::exists(base))
    throw std::runtime_error("RAPL not supported on this device");

  for(const auto& entry : fs::directory_iterator(base)) {
    if(!entry.is_directory()) continue;
    if(!fs::exists(entry.path()/"name")) continue;

    std::ifstream file(entry.path()/"name");
    std::string name;
    std::getline(file, name);

    if(name=="package-0") {
      return entry.path();
    }

  }

  throw std::runtime_error("package-0 domain not found");
}

uint64_t Rapl::read_energy_uj () {
  fs::path p(this->path + "/energy_uj");
  return read_uint64(p);
}

Rapl init_rapl() {
  fs::path path = find_rapl_path();
  uint64_t max_energy = read_uint64(path / "max_energy_range_uj");

  Rapl rapl(path.string(), max_energy);

  return rapl;
}

double Rapl::get_power() {
  using clock = std::chrono::steady_clock;
  uint64_t e0 = this->read_energy_uj();
  auto t0 = clock::now();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  uint64_t e1 = this->read_energy_uj();
  auto t1 = clock::now();

  std::chrono::duration<double> dur = t1-t0;
  double delta = dur.count();

  uint64_t de;

  if(e1>= e0) {
    de = e1-e0;
  }
  else {
    de = (this->max_energy - e0) + e1;
  }

  double de_j = static_cast<double>(de)*1e-6;

  return de_j / delta;
}


