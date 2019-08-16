#include <iostream>
#include <fstream>
#include <vector>

#include <hyperloglog.hpp>

using hll_t = hll::HyperLogLog<10, 20>;

#include "measures.hpp"

int main(int /*argc*/, const char** /*argv*/) {
  size_t size = 1ul << 14;

  for (size_t i = 0; i < size; i++) {

    if ((i & (i - 1)) == 0 && i > 32) {
      // i is a power of two
      for (size_t j = i-i/2; j < i+i/2; j++)
        std::cout << j << " ";
      std::cout << "\n";

      for (size_t j = i-i/2; j < i+i/2; j++)
        std::cout << hll_estimator<size_t>::p_larger((double)j, i) << " ";
      std::cout << "\n";
    }
  }

}
