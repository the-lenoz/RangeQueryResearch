module;

#include <CLI/CLI.hpp>

#include <expected>
#include <map>
#include <string>

module rangequery.config;

namespace rq {

namespace {

CLI::Option* add_structure_option(CLI::App& app, StructureKind& structure) {
  const std::map<std::string, StructureKind> names{{"vector", StructureKind::ordered_vector},
                                                   {"set", StructureKind::std_set},
                                                   {"avl", StructureKind::avl},
                                                   {"treap", StructureKind::treap}};
  // rq::to_string returns string_view; CLI11's default capture expects string.
  return app.add_option_no_stream("--structure", structure, "Set implementation")
      ->transform(CLI::CheckedTransformer(names));
}

int exit_cli(const CLI::App& app, const CLI::ParseError& error) {
  return app.exit(error) == 0 ? 0 : 2;
}

}  // namespace

std::expected<Settings, int> read_command_line(int argc, char* argv[]) {
  Settings settings;
  CLI::App app{"Process order-statistic set commands from stdin"};
  add_structure_option(app, settings.structure)->default_str("vector");
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& error) {
    return std::unexpected{exit_cli(app, error)};
  }
  return settings;
}

std::expected<BenchmarkSettings, int> read_benchmark_command_line(int argc, char* argv[]) {
  BenchmarkSettings settings;
  CLI::App app{"Measure order-statistic set operations from a stdin workload"};
  add_structure_option(app, settings.structure)->required();
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
    return std::unexpected{exit_cli(app, error)};
  }
  return {std::move(settings)};
}

}  // namespace rq
