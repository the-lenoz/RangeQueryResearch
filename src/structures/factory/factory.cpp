module;

#include <expected>
#include <string_view>
#include <variant>

module rangequery.structures.factory;

namespace rq::structures {

std::expected<SelectedStructure, std::string_view> make_structure(const StructureKind kind) {
  switch (kind) {
    case StructureKind::ordered_vector:
      return SelectedStructure{std::in_place_type<OrderedVector>};
    case StructureKind::std_set:
      return SelectedStructure{std::in_place_type<StdSet>};
    case StructureKind::avl:
      return SelectedStructure{std::in_place_type<AvlTree>};
    case StructureKind::treap:
      return SelectedStructure{std::in_place_type<Treap>};
    default:
      return std::unexpected<std::string_view>{"unknown structure kind"};
  }
}

}  // namespace rq::structures
