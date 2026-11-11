module;

#include <expected>

export module rangequery.config;

import cli11;
export import rangequery.structures.kind;

namespace rq {
export struct Settings {
  StructureKind structure{StructureKind::ordered_vector};
};

export CLI::Option* add_structure_option(CLI::App& app, StructureKind& structure);
export int exit_cli(const CLI::App& app, const CLI::ParseError& error);
// An unexpected value requests process exit: 0 after help, 2 after a CLI error.
// CLI11 has already printed the corresponding help or diagnostic.
export [[nodiscard]] std::expected<Settings, int> read_command_line(int argc, char* argv[]);

}  // namespace rq
