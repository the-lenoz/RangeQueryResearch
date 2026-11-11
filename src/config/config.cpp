module;

#include <expected>
#include <map>
#include <string>
#include <typeinfo>

module rangequery.config;

import cli11;

namespace rq {

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

}  // namespace rq
