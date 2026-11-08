module;

#include <optional>
#include <string_view>

export module rangequery.structures.kind;

namespace rq {

export enum class StructureKind { ordered_vector, std_set, avl, treap };

export [[nodiscard]] constexpr std::optional<StructureKind> parse_structure_name(
    const std::string_view name) {
  if (name == "vector") return StructureKind::ordered_vector;
  if (name == "set") return StructureKind::std_set;
  if (name == "avl") return StructureKind::avl;
  if (name == "treap") return StructureKind::treap;
  return std::nullopt;
}

export [[nodiscard]] constexpr std::string_view to_string(const StructureKind kind) {
  switch (kind) {
    case StructureKind::ordered_vector:
      return "vector";
    case StructureKind::std_set:
      return "set";
    case StructureKind::avl:
      return "avl";
    case StructureKind::treap:
      return "treap";
    default:
      return "unknown";
  }
}

}  // namespace rq
