#include <iostream>
#include <fstream>
#include <vector>

#include <hyperloglog.hpp>

int main(int /*argc*/, const char** /*argv*/) {
  size_t max = 29;
  constexpr unsigned short p = 10;

  std::vector<hll::HyperLogLog<p, 20>> hll_ests;
  for (uint32_t seed = 0; seed < 1000; seed++)
    hll_ests.emplace_back(true, seed);

  for (size_t i = 1; i <= (1ul<<max); i++) {
    for (auto&& est: hll_ests)
      est.insert(i);
    if ((i & (i - 1)) == 0) {
      // i is a power of two
      std::cout << i;
      for (auto&& est: hll_ests)
        std::cout << " " << est.estimate();
      std::cout << "\n";
    }
  }
}
