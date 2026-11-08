module;

#include <expected>
#include <string_view>

export module rangequery.config;

export import rangequery.structures.kind;

namespace rq {

export struct Settings {
  StructureKind structure{StructureKind::ordered_vector};
  bool show_help{false};
};

export [[nodiscard]] std::expected<Settings, std::string_view> read_command_line(int argc,
                                                                                 char* argv[]);

}  // namespace rq
