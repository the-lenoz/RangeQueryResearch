module;

#include <expected>
#include <CLI/CLI.hpp>

export module rangequery.benchmark.config;

import rangequery.config;


namespace rq_benchmark {


export enum class BenchmarkMode { queries, insert, dynamic };

export struct BenchmarkSettings {
  std::size_t setup_count{};
  rq::StructureKind structure{rq::StructureKind::ordered_vector};
  BenchmarkMode mode{BenchmarkMode::queries};
};

export [[nodiscard]] std::expected<BenchmarkSettings, int> read_benchmark_command_line(
    int argc, char* argv[]) {
  BenchmarkSettings settings;
  CLI::App app{"Measure order-statisctic set operations from a stdin workload"};
  rq::add_structure_option(app, settings.structure)->required();
  const std::map<std::string, BenchmarkMode> modes{{"queries", BenchmarkMode::queries},
                                                   {"insert", BenchmarkMode::insert},
                                                   {"dynamic", BenchmarkMode::dynamic}};
  app.add_option("--mode", settings.mode, "Measured workload mode")
      ->required()
      ->transform(CLI::CheckedTransformer(modes));
  const auto* setup = app.add_option("--setup-count", settings.setup_count,
                                     "Number of setup insertions (dynamic mode only)")
                          ->check(CLI::PositiveNumber);
  try {
    app.parse(argc, argv);
    if (settings.mode == BenchmarkMode::dynamic && setup->count() == 0) {
      throw CLI::ValidationError("--setup-count", "required in dynamic mode");
    }
    if (settings.mode != BenchmarkMode::dynamic && setup->count() != 0) {
      throw CLI::ValidationError("--setup-count", "only valid in dynamic mode");
    }
  } catch (const CLI::ParseError& error) {
    return std::unexpected{rq::exit_cli(app, error)};
  }
  return {std::move(settings)};
}
};  // namespace rq_benchmark
