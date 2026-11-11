#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <expected>
#include <iomanip>
#include <iostream>
#include <limits>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

import rangequery.config;
import rangequery.structures.factory;

namespace {

using rq::BenchmarkMode;
using Settings = rq::BenchmarkSettings;

struct Command {
  char kind{};
  int first{};
  int second{};
  unsigned long long index{};
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
  benchmark::IterationCount benchmark_iterations{};
};

constexpr std::uint64_t kFnvOffset = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t kFnvPrime = 1'099'511'628'211ULL;

std::expected<std::vector<Command>, std::string_view> read_commands() {
  std::vector<Command> commands;
  char kind{};
  while (std::cin >> kind) {
    Command command{.kind = kind};
    switch (kind) {
      case 'k':
      case 'n':
        if (!(std::cin >> command.first)) {
          return std::unexpected<std::string_view>{"invalid command: expected an integer"};
        }
        break;
      case 'q':
        if (!(std::cin >> command.first >> command.second)) {
          return std::unexpected<std::string_view>{"invalid q command: expected two integers"};
        }
        break;
      case 'm': {
        long long index{};
        if (!(std::cin >> index) || index <= 0) {
          return std::unexpected<std::string_view>{"invalid m command: expected a positive index"};
        }
        command.index = static_cast<unsigned long long>(index);
        break;
      }
      default:
        return std::unexpected<std::string_view>{"unknown workload command"};
    }
    commands.push_back(command);
  }
  if (!std::cin.eof()) {
    return std::unexpected<std::string_view>{"malformed workload"};
  }
  return commands;
}

std::expected<WorkloadLayout, std::string_view> validate_layout(
    const std::vector<Command>& commands, const Settings& settings) {
  WorkloadLayout layout;
  if (commands.empty()) {
    return std::unexpected<std::string_view>{"workload must not be empty"};
  }
  if (settings.mode == BenchmarkMode::insert) {
    if (!std::ranges::all_of(
            commands, [](char kind) { return kind == 'k'; }, &Command::kind)) {
      return std::unexpected<std::string_view>{"insert mode accepts only k commands"};
    }
    layout.measured_count = commands.size();
    return layout;
  }

  if (settings.mode == BenchmarkMode::dynamic) {
    if (settings.setup_count >= commands.size()) {
      return std::unexpected<std::string_view>{
          "dynamic mode requires at least one measured command after setup"};
    }
    if (!std::ranges::all_of(
            commands | std::views::take(settings.setup_count),
            [](char kind) { return kind == 'k'; }, &Command::kind)) {
      return std::unexpected<std::string_view>{"dynamic setup accepts only k commands"};
    }
    layout.setup_count = settings.setup_count;
    layout.measured_count = commands.size() - layout.setup_count;
    return layout;
  }

  while (layout.setup_count < commands.size() && commands[layout.setup_count].kind == 'k') {
    ++layout.setup_count;
  }
  if (layout.setup_count == 0) {
    return std::unexpected<std::string_view>{"queries mode requires leading setup insertions"};
  }
  if (layout.setup_count == commands.size()) {
    return std::unexpected<std::string_view>{"queries mode requires at least one query"};
  }
  if (std::ranges::contains(commands | std::views::drop(layout.setup_count), 'k', &Command::kind)) {
    return std::unexpected<std::string_view>{
        "queries mode forbids insertions after the first query"};
  }
  layout.measured_count = commands.size() - layout.setup_count;
  return layout;
}

// The runner owns repetitions and workload selection. Do not let Google
// Benchmark environment defaults introduce warmup passes or extra output files.
void initialize_benchmark() {
  std::vector<std::string> arguments{
      "rangequery_benchmark",
      "--benchmark_min_warmup_time=0",
      "--benchmark_list_tests=false",
      "--benchmark_out=",
      "--v=0",
  };
  std::vector<char*> argv;
  for (auto& argument : arguments) argv.push_back(argument.data());
  int argc = static_cast<int>(argv.size());
  argv.push_back(nullptr);
  benchmark::Initialize(&argc, argv.data());
}

class MeasurementReporter final : public benchmark::BenchmarkReporter {
 public:
  explicit MeasurementReporter(Measurement& measurement) : measurement_(measurement) {}

  bool ReportContext(const Context&) override {
    return true;
  }

  void ReportRuns(const std::vector<Run>& reports) override {
    for (const auto& run : reports) {
      if (run.run_type != Run::RT_Iteration) continue;
      ++report_count_;
      if (run.skipped != benchmark::internal::NotSkipped || run.iterations != 1 ||
          !std::isfinite(run.real_accumulated_time) || run.real_accumulated_time < 0 ||
          !std::isfinite(run.cpu_accumulated_time) || run.cpu_accumulated_time < 0) {
        failed_ = true;
        continue;
      }
      measurement_.wall_elapsed_ns =
          static_cast<std::uint64_t>(run.real_accumulated_time * 1'000'000'000.0);
      measurement_.cpu_elapsed_ns =
          static_cast<std::uint64_t>(run.cpu_accumulated_time * 1'000'000'000.0);
      measurement_.benchmark_iterations = run.iterations;
    }
  }

  [[nodiscard]] bool valid() const {
    return report_count_ == 1 && !failed_;
  }

 private:
  Measurement& measurement_;
  std::size_t report_count_{};
  bool failed_{};
};

void hash_byte(std::uint64_t& checksum, std::uint8_t value) {
  checksum ^= value;
  checksum *= kFnvPrime;
}

void hash_record(std::uint64_t& checksum, std::uint8_t code, bool has_value, std::uint64_t value) {
  hash_byte(checksum, code);
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
  return {selection.has_value(),
          static_cast<std::uint64_t>(static_cast<std::int64_t>(selection.value_or(0)))};
}

template <rq::structures::OrderStatisticSet Set>
std::uint64_t validate_queries(const Set& structure, const std::vector<Command>& commands,
                               std::size_t setup_count) {
  std::uint64_t checksum = kFnvOffset;
  for (const Command& command : commands | std::views::drop(setup_count)) {
    const auto [has_value, value] = query_result(structure, command);
    hash_record(checksum, static_cast<std::uint8_t>(command.kind), has_value, value);
  }
  return checksum;
}

template <rq::structures::OrderStatisticSet Set>
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

template <rq::structures::OrderStatisticSet Set>
std::uint64_t validate_insertions(const Set& structure, const std::vector<Command>& commands) {
  std::vector<int> values{std::from_range, commands | std::views::transform(&Command::first)};

  std::uint64_t checksum = kFnvOffset;
  hash_structure_state(checksum, structure, std::move(values));
  return checksum;
}

template <rq::structures::OrderStatisticSet Set>
std::uint64_t validate_dynamic(const Set& measured_structure, const std::vector<Command>& commands,
                               std::size_t setup_count) {
  Set replay;
  std::vector<int> values;
  values.reserve(commands.size());
  for (std::size_t index = 0; index < setup_count; ++index) {
    replay.insert(commands[index].first);
    values.push_back(commands[index].first);
  }

  std::uint64_t checksum = kFnvOffset;
  for (const Command& command : commands | std::views::drop(setup_count)) {
    if (command.kind == 'k') {
      replay.insert(command.first);
      values.push_back(command.first);
    } else {
      const auto [has_value, value] = query_result(replay, command);
      hash_record(checksum, static_cast<std::uint8_t>(command.kind), has_value, value);
    }
  }
  hash_structure_state(checksum, measured_structure, std::move(values));
  return checksum;
}

template <rq::structures::OrderStatisticSet Set>
void run_benchmark(benchmark::State& state, Set& structure, const std::vector<Command>& commands,
                   const BenchmarkMode mode, const WorkloadLayout layout,
                   Measurement& measurement) {
  if (mode == BenchmarkMode::queries || mode == BenchmarkMode::dynamic) {
    for (const int value :
         commands | std::views::take(layout.setup_count) | std::views::transform(&Command::first)) {
      structure.insert(value);
    }
  }

  // Escape the structure before the timed loop so its writes are observable.
  auto* escaped = &structure;
  benchmark::DoNotOptimize(escaped);

  // One iteration is the entire measured workload, never a single command.
  for (auto _ : state) {
    if (mode == BenchmarkMode::insert) {
      for (const Command& command : commands) structure.insert(command.first);
    } else {
      for (std::size_t index = layout.setup_count; index < commands.size(); ++index) {
        const Command& command = commands[index];
        if (command.kind == 'k') {
          structure.insert(command.first);
        } else {
          auto [has_value, value] = query_result(structure, command);
          benchmark::DoNotOptimize(has_value);
          benchmark::DoNotOptimize(value);
        }
      }
    }
    benchmark::ClobberMemory();
  }

  // Google Benchmark stops both timers on loop exit. Validation and teardown
  // must stay outside the loop, including the independent dynamic replay.
  measurement.validation_checksum =
      mode == BenchmarkMode::insert    ? validate_insertions(structure, commands)
      : mode == BenchmarkMode::dynamic ? validate_dynamic(structure, commands, layout.setup_count)
                                       : validate_queries(structure, commands, layout.setup_count);
  measurement.measured_operation_count = layout.measured_count;
  state.SetItemsProcessed(static_cast<std::int64_t>(layout.measured_count));
}

}  // namespace

int main(int argc, char* argv[]) try {
  const auto settings = rq::read_benchmark_command_line(argc, argv);
  if (!settings) return settings.error();

  auto commands = read_commands();
  if (!commands) {
    std::println(stderr, "{}", commands.error());
    return 2;
  }
  const auto layout = validate_layout(*commands, *settings);
  if (!layout) {
    std::println(stderr, "{}", layout.error());
    return 2;
  }

  auto structure = rq::structures::make_structure(settings->structure);
  if (!structure) {
    std::println(stderr, "{}", structure.error());
    return 2;
  }
  initialize_benchmark();
  Measurement measurement;
  std::size_t invocation_count{};
  std::visit(
      [&](auto& selected) {
        auto callback = [&, selected_ptr = &selected](benchmark::State& state) {
          ++invocation_count;
          run_benchmark(state, *selected_ptr, *commands, settings->mode, *layout, measurement);
        };
        benchmark::RegisterBenchmark("workload", callback)
            ->Iterations(1)
            ->Repetitions(1)
            ->MeasureProcessCPUTime()
            ->UseRealTime()
            ->Unit(benchmark::kNanosecond);
      },
      *structure);
  MeasurementReporter reporter{measurement};
  const auto matched = benchmark::RunSpecifiedBenchmarks(&reporter, "^workload/");
  benchmark::ClearRegisteredBenchmarks();
  benchmark::Shutdown();
  if (matched != 1 || invocation_count != 1 || !reporter.valid()) {
    std::println(stderr, "benchmark must measure exactly one workload pass");
    return 2;
  }

  std::ostringstream output;
  output << "{\"wall_elapsed_ns\":" << measurement.wall_elapsed_ns
         << ",\"cpu_elapsed_ns\":" << measurement.cpu_elapsed_ns
         << ",\"measured_operation_count\":" << measurement.measured_operation_count
         << ",\"validation_checksum\":\"" << std::hex << std::setfill('0') << std::setw(16)
         << measurement.validation_checksum << std::dec
         << "\",\"cpu_clock\":\"process\",\"wall_clock\":\"google_benchmark\""
         << ",\"benchmark_engine\":\"google_benchmark\",\"benchmark_version\":\""
         << RQ_BENCHMARK_VERSION
         << "\",\"benchmark_iterations\":" << measurement.benchmark_iterations
         << ",\"compiler\":" << std::quoted(RQ_COMPILER)
         << ",\"compiler_version\":" << std::quoted(RQ_COMPILER_VERSION)
         << ",\"build_config\":" << std::quoted(RQ_BUILD_CONFIG) << "}";
  std::println("{}", output.str());
  return 0;
} catch (const std::exception& error) {
  std::println(stderr, "rangequery_benchmark: {}", error.what());
  return 2;
} catch (...) {
  std::println(stderr, "rangequery_benchmark: unknown exception");
  return 2;
}
