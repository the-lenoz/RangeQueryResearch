module;

#include <cstddef>
#include <expected>

export module rangequery.config;

export import rangequery.structures.kind;

namespace rq {

export struct Settings {
  StructureKind structure{StructureKind::ordered_vector};
};

export enum class BenchmarkMode { queries, insert, dynamic };

export struct BenchmarkSettings {
  std::size_t setup_count{};
  StructureKind structure{StructureKind::ordered_vector};
  BenchmarkMode mode{BenchmarkMode::queries};
};

// An unexpected value requests process exit: 0 after help, 2 after a CLI error.
// CLI11 has already printed the corresponding help or diagnostic.
export [[nodiscard]] std::expected<Settings, int> read_command_line(int argc, char* argv[]);
export [[nodiscard]] std::expected<BenchmarkSettings, int> read_benchmark_command_line(
    int argc, char* argv[]);

}  // namespace rq
