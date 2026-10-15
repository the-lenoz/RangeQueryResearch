#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;
import rangequery.structures.treap;

namespace {

enum class StructureKind { vector, set, avl, treap };
enum class BenchmarkMode { queries, insert };

struct Settings {
  StructureKind structure{StructureKind::vector};
  BenchmarkMode mode{BenchmarkMode::queries};
  bool has_structure{false};
  bool has_mode{false};
  bool valid{true};
  const char* error{nullptr};
};

struct Command {
  char kind{};
  int first{};
  int second{};
  rq::structures::Count index{};
};

struct WorkloadLayout {
  std::size_t setup_count{};
  std::size_t measured_count{};
};

struct Measurement {
  std::uint64_t wall_elapsed_ns{};
  std::uint64_t cpu_elapsed_ns{};
  std::size_t measured_operation_count{};
  std::uint64_t validation_checksum{};
};

constexpr std::uint64_t kFnvOffset = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t kFnvPrime = 1'099'511'628'211ULL;

Settings read_settings(const int argc, char* argv[]) {
  Settings settings;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--structure" && index + 1 < argc) {
      const std::string_view value{argv[++index]};
      settings.has_structure = true;
      if (value == "vector") {
        settings.structure = StructureKind::vector;
      } else if (value == "set") {
        settings.structure = StructureKind::set;
      } else if (value == "avl") {
        settings.structure = StructureKind::avl;
      } else if (value == "treap") {
        settings.structure = StructureKind::treap;
      } else {
        settings.valid = false;
        settings.error = "unknown structure; expected vector, set, avl, or treap";
      }
    } else if (argument == "--mode" && index + 1 < argc) {
      const std::string_view value{argv[++index]};
      settings.has_mode = true;
      if (value == "queries") {
        settings.mode = BenchmarkMode::queries;
      } else if (value == "insert") {
        settings.mode = BenchmarkMode::insert;
      } else {
        settings.valid = false;
        settings.error = "unknown mode; expected queries or insert";
      }
    } else if (argument == "--structure") {
      settings.valid = false;
      settings.error = "--structure requires a value";
    } else if (argument == "--mode") {
      settings.valid = false;
      settings.error = "--mode requires a value";
    } else {
      settings.valid = false;
      settings.error = "unknown command-line argument";
    }
  }
  if (settings.valid && !settings.has_structure) {
    settings.valid = false;
    settings.error = "--structure is required";
  }
  if (settings.valid && !settings.has_mode) {
    settings.valid = false;
    settings.error = "--mode is required";
  }
  return settings;
}

bool read_commands(std::vector<Command>& commands, const char*& error) {
  char kind{};
  while (std::cin >> kind) {
    Command command{.kind = kind};
    if (kind == 'k' || kind == 'n') {
      if (!(std::cin >> command.first)) {
        error = "invalid command: expected an integer";
        return false;
      }
    } else if (kind == 'q') {
      if (!(std::cin >> command.first >> command.second)) {
        error = "invalid q command: expected two integers";
        return false;
      }
    } else if (kind == 'm') {
      long long index{};
      if (!(std::cin >> index) || index <= 0) {
        error = "invalid m command: expected a positive index";
        return false;
      }
      command.index = static_cast<rq::structures::Count>(index);
    } else {
      error = "unknown workload command";
      return false;
    }
    commands.push_back(command);
  }
  if (!std::cin.eof()) {
    error = "malformed workload";
    return false;
  }
  return true;
}

bool validate_layout(const std::vector<Command>& commands, const BenchmarkMode mode,
                     WorkloadLayout& layout, const char*& error) {
  if (commands.empty()) {
    error = "workload must not be empty";
    return false;
  }
  if (mode == BenchmarkMode::insert) {
    if (std::ranges::any_of(commands, [](const Command& command) { return command.kind != 'k'; })) {
      error = "insert mode accepts only k commands";
      return false;
    }
    layout.measured_count = commands.size();
    return true;
  }

  while (layout.setup_count < commands.size() && commands[layout.setup_count].kind == 'k') {
    ++layout.setup_count;
  }
  if (layout.setup_count == 0) {
    error = "queries mode requires leading setup insertions";
    return false;
  }
  if (layout.setup_count == commands.size()) {
    error = "queries mode requires at least one query";
    return false;
  }
  if (std::ranges::any_of(commands.begin() + static_cast<std::ptrdiff_t>(layout.setup_count),
                          commands.end(),
                          [](const Command& command) { return command.kind == 'k'; })) {
    error = "queries mode forbids insertions after the first query";
    return false;
  }
  layout.measured_count = commands.size() - layout.setup_count;
  return true;
}

void do_not_optimize(const std::uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__("" : : "g"(value) : "memory");
#else
  volatile std::uint64_t sink = value;
  (void)sink;
#endif
}

std::uint64_t cpu_time_ns() {
#if defined(__linux__)
  timespec value{};
  if (::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value) != 0) return 0;
  return static_cast<std::uint64_t>(value.tv_sec) * 1'000'000'000ULL +
         static_cast<std::uint64_t>(value.tv_nsec);
#else
  const std::clock_t value = std::clock();
  if (value == static_cast<std::clock_t>(-1)) return 0;
  return static_cast<std::uint64_t>(value) * 1'000'000'000ULL / CLOCKS_PER_SEC;
#endif
}

constexpr std::string_view cpu_clock_name() {
#if defined(__linux__)
  return "process";
#else
  return "std_clock";
#endif
}

void hash_byte(std::uint64_t& checksum, const std::uint8_t value) {
  checksum ^= value;
  checksum *= kFnvPrime;
}

void hash_record(std::uint64_t& checksum, const char code, const bool has_value,
                 const std::uint64_t value) {
  hash_byte(checksum, static_cast<std::uint8_t>(code));
  hash_byte(checksum, has_value ? 1 : 0);
  for (unsigned shift = 0; shift < 64; shift += 8) {
    hash_byte(checksum, static_cast<std::uint8_t>(value >> shift));
  }
}

template <rq::structures::OrderStatisticSet Set>
std::pair<bool, std::uint64_t> query_result(const Set& structure, const Command& command) {
  if (command.kind == 'q') {
    return {true, structure.count_in_open_closed_range(command.first, command.second)};
  }
  if (command.kind == 'n') return {true, structure.count_less(command.first)};
  const auto selection = structure.select(command.index);
  return {selection.has_value,
          static_cast<std::uint64_t>(static_cast<std::int64_t>(selection.value))};
}

template <rq::structures::OrderStatisticSet Set>
std::uint64_t validate_queries(const Set& structure, const std::vector<Command>& commands,
                               const std::size_t setup_count) {
  std::uint64_t checksum = kFnvOffset;
  for (std::size_t index = setup_count; index < commands.size(); ++index) {
    const auto [has_value, value] = query_result(structure, commands[index]);
    hash_record(checksum, commands[index].kind, has_value, value);
  }
  return checksum;
}

template <rq::structures::OrderStatisticSet Set>
std::uint64_t validate_insertions(const Set& structure, const std::vector<Command>& commands) {
  std::vector<int> values;
  values.reserve(commands.size());
  for (const Command& command : commands) values.push_back(command.first);
  std::ranges::sort(values);
  const auto unique_end = std::ranges::unique(values).begin();
  values.erase(unique_end, values.end());

  std::uint64_t checksum = kFnvOffset;
  hash_record(checksum, 's', true, structure.size());

  std::vector<rq::structures::Count> indices{
      1,
      1 + static_cast<rq::structures::Count>(values.size() / 4),
      1 + static_cast<rq::structures::Count>(values.size() / 2),
      1 + static_cast<rq::structures::Count>((3 * values.size()) / 4),
      static_cast<rq::structures::Count>(values.size()),
  };
  std::ranges::sort(indices);
  indices.erase(std::ranges::unique(indices).begin(), indices.end());
  for (const auto index : indices) {
    const auto selection = structure.select(index);
    hash_record(checksum, 'm', selection.has_value,
                static_cast<std::uint64_t>(static_cast<std::int64_t>(selection.value)));
  }

  const std::vector probes{
      std::numeric_limits<int>::min(), 0,
      std::numeric_limits<int>::max(), values.front(),
      values[values.size() / 2],       values.back(),
  };
  for (const int probe : probes) {
    hash_record(checksum, 'n', true, structure.count_less(probe));
  }
  return checksum;
}

template <rq::structures::OrderStatisticSet Set>
Measurement run_benchmark(Set& structure, const std::vector<Command>& commands,
                          const BenchmarkMode mode, const WorkloadLayout layout) {
  if (mode == BenchmarkMode::queries) {
    for (std::size_t index = 0; index < layout.setup_count; ++index) {
      structure.insert(commands[index].first);
    }
  }

  const std::uint64_t cpu_started = cpu_time_ns();
  const auto wall_started = std::chrono::steady_clock::now();
  if (mode == BenchmarkMode::insert) {
    for (const Command& command : commands) structure.insert(command.first);
  } else {
    for (std::size_t index = layout.setup_count; index < commands.size(); ++index) {
      const auto [has_value, value] = query_result(structure, commands[index]);
      do_not_optimize(value ^ (has_value ? (1ULL << 63) : 0));
    }
  }
  const auto wall_finished = std::chrono::steady_clock::now();
  const std::uint64_t cpu_finished = cpu_time_ns();

  const auto wall_elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(wall_finished - wall_started).count();
  const std::uint64_t checksum = mode == BenchmarkMode::insert
                                     ? validate_insertions(structure, commands)
                                     : validate_queries(structure, commands, layout.setup_count);
  return Measurement{
      .wall_elapsed_ns = static_cast<std::uint64_t>(wall_elapsed),
      .cpu_elapsed_ns = cpu_finished - cpu_started,
      .measured_operation_count = layout.measured_count,
      .validation_checksum = checksum,
  };
}

using SelectedStructure = std::variant<rq::structures::OrderedVector, rq::structures::StdSet,
                                       rq::structures::AvlTree, rq::structures::Treap>;

SelectedStructure make_structure(const StructureKind kind) {
  switch (kind) {
    case StructureKind::vector:
      return SelectedStructure{std::in_place_type<rq::structures::OrderedVector>};
    case StructureKind::set:
      return SelectedStructure{std::in_place_type<rq::structures::StdSet>};
    case StructureKind::avl:
      return SelectedStructure{std::in_place_type<rq::structures::AvlTree>};
    case StructureKind::treap:
      return SelectedStructure{std::in_place_type<rq::structures::Treap>};
  }
  return SelectedStructure{std::in_place_type<rq::structures::OrderedVector>};
}

}  // namespace

int main(const int argc, char* argv[]) {
  const Settings settings = read_settings(argc, argv);
  if (!settings.valid) {
    std::cerr << settings.error << '\n';
    return 2;
  }

  std::vector<Command> commands;
  const char* error = nullptr;
  if (!read_commands(commands, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  WorkloadLayout layout;
  if (!validate_layout(commands, settings.mode, layout, error)) {
    std::cerr << error << '\n';
    return 2;
  }

  auto structure = make_structure(settings.structure);
  const Measurement measurement = std::visit(
      [&](auto& selected) { return run_benchmark(selected, commands, settings.mode, layout); },
      structure);

  std::cout << "{\"wall_elapsed_ns\":" << measurement.wall_elapsed_ns
            << ",\"cpu_elapsed_ns\":" << measurement.cpu_elapsed_ns
            << ",\"measured_operation_count\":" << measurement.measured_operation_count
            << ",\"validation_checksum\":\"" << std::hex << std::setfill('0') << std::setw(16)
            << measurement.validation_checksum << "\",\"cpu_clock\":\"" << cpu_clock_name()
            << "\"}\n";
  return 0;
}
