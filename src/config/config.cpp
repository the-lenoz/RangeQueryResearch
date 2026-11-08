module;

#include <expected>
#include <string_view>

module rangequery.config;

namespace rq {

std::expected<Settings, std::string_view> read_command_line(const int argc, char* argv[]) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "-h" || argument == "--help") {
      return Settings{.show_help = true};
    }
  }

  Settings settings;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--structure" && index + 1 < argc) {
      const auto structure = parse_structure_name(argv[++index]);
      if (structure) {
        settings.structure = *structure;
      } else {
        return std::unexpected<std::string_view>{
            "unknown structure; expected vector, set, avl, or treap"};
      }
    } else if (argument == "--structure") {
      return std::unexpected<std::string_view>{"--structure requires vector, set, avl, or treap"};
    } else {
      return std::unexpected<std::string_view>{"unknown command-line argument"};
    }
  }
  return settings;
}

}  // namespace rq
