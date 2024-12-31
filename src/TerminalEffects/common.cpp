#include "precompiled.hpp"

#include "common.hpp"

namespace {
  std::mt19937      random__generator { 19740531 };
}

std::size_t pick_a_number(std::size_t min, std::size_t max) {
  assert(max >= min);
  std::uniform_int_distribution<std::size_t> dist(min, max);
  return dist(random__generator);
}
