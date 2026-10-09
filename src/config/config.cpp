module rangequery.config;

namespace rq {

namespace {

bool equals(const char* left, const char* right) {
  while (*left != '\0' && *left == *right) {
    ++left;
    ++right;
  }
  return *left == *right;
}

StructureKind parse_structure(const char* name, bool& valid) {
  if (equals(name, "vector")) return StructureKind::ordered_vector;
  if (equals(name, "set")) return StructureKind::std_set;
  if (equals(name, "avl")) return StructureKind::avl;
  if (equals(name, "treap")) return StructureKind::treap;
  valid = false;
  return StructureKind::ordered_vector;
}

}  // namespace

Settings read_command_line(int argc, char* argv[]) {
  Settings settings;
  for (int index = 1; index < argc; ++index) {
    const char* argument = argv[index];
    if (equals(argument, "-h") || equals(argument, "--help")) {
      settings.show_help = true;
      continue;
    }
    if (equals(argument, "--structure") && index + 1 < argc) {
      settings.structure = parse_structure(argv[++index], settings.valid);
      if (!settings.valid) settings.error_message = "unknown structure; expected vector, set, avl, or treap";
    } else if (equals(argument, "--structure")) {
      settings.valid = false;
      settings.error_message = "--structure requires vector, set, avl, or treap";
    } else {
      settings.valid = false;
      settings.error_message = "unknown command-line argument";
    }
  }
  return settings;
}

const char* to_string(StructureKind kind) {
  switch (kind) {
    case StructureKind::ordered_vector: return "ordered_vector";
    case StructureKind::std_set: return "std_set";
    case StructureKind::avl: return "avl";
    case StructureKind::treap: return "treap";
  }
  return "unknown";
}

}  // namespace rq
