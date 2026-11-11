module;

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ranges>
#include <vector>

export module rangequery.benchmark.utils;

import rangequery.structures.contracts;
import rangequery.structures.kind;

namespace rq_benchmark {

export constexpr std::uint64_t fnv_offset = 14'695'981'039'346'656'037ULL;

namespace {

constexpr std::uint64_t kFnvPrime = 1'099'511'628'211ULL;

void hash_byte(std::uint64_t& checksum, const std::uint8_t value) {
  checksum ^= value;
  checksum *= kFnvPrime;
}

}  // namespace

export void hash_record(std::uint64_t& checksum, const std::uint8_t code, const bool has_value,
                        const std::uint64_t value) {
  hash_byte(checksum, code);
  hash_byte(checksum, has_value ? 1 : 0);
  for (unsigned shift = 0; shift < 64; shift += 8) {
    hash_byte(checksum, static_cast<std::uint8_t>(value >> shift));
  }
}

export template <rq::structures::OrderStatisticSet Set>
void hash_structure_state(std::uint64_t& checksum, const Set& structure, std::vector<int> values) {
  std::ranges::sort(values);
  values.erase(std::ranges::unique(values).begin(), values.end());

  hash_record(checksum, static_cast<std::uint8_t>('s'), true, structure.size());

  std::vector<unsigned long long> indices{
      1,
      1 + static_cast<unsigned long long>(values.size() / 4),
      1 + static_cast<unsigned long long>(values.size() / 2),
      1 + static_cast<unsigned long long>((3 * values.size()) / 4),
      static_cast<unsigned long long>(values.size()),
  };
  std::ranges::sort(indices);
  indices.erase(std::ranges::unique(indices).begin(), indices.end());
  for (const auto index : indices) {
    const auto selection = structure.select(index);
    hash_record(checksum, static_cast<std::uint8_t>('m'), selection.has_value(),
                static_cast<std::uint64_t>(static_cast<std::int64_t>(selection.value_or(0))));
  }

  const std::vector probes{
      std::numeric_limits<int>::min(), 0,
      std::numeric_limits<int>::max(), values.front(),
      values[values.size() / 2],       values.back(),
  };
  for (const int probe : probes) {
    hash_record(checksum, static_cast<std::uint8_t>('n'), true, structure.count_less(probe));
  }
}

}  // namespace rq_benchmark
