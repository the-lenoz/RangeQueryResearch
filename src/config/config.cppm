export module rangequery.config;

export namespace rq {

enum class StructureKind { ordered_vector, std_set, avl };

struct Settings {
  StructureKind structure{StructureKind::ordered_vector};
  bool show_help{false};
  bool valid{true};
  const char* error_message{nullptr};
};

[[nodiscard]] Settings read_command_line(int argc, char* argv[]);
[[nodiscard]] const char* to_string(StructureKind kind);

}  // namespace rq
