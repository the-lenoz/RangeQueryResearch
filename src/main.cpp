import rangequery.config;
import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;

#include <iostream>
#include <variant>

namespace {

template <rq::structures::OrderStatisticSet Set>
int run_commands(Set& structure) {
  char command{};
  while (std::cin >> command) {
    if (command == 'k') {
      int value{};
      if (!(std::cin >> value)) {
        std::cerr << "invalid k command: expected an integer\n";
        return 2;
      }
      structure.insert(value);
    } else if (command == 'q') {
      int left{}, right{};
      if (!(std::cin >> left >> right)) {
        std::cerr << "invalid q command: expected two integers\n";
        return 2;
      }
      std::cout << structure.count_in_open_closed_range(left, right) << '\n';
    } else if (command == 'm') {
      long long index{};
      if (!(std::cin >> index) || index <= 0) {
        std::cerr << "invalid m command: expected a positive index\n";
        return 2;
      }
      const auto result = structure.select(static_cast<rq::structures::Count>(index));
      if (!result.has_value) {
        std::cerr << "m index is outside the set\n";
        return 2;
      }
      std::cout << result.value << '\n';
    } else if (command == 'n') {
      int value{};
      if (!(std::cin >> value)) {
        std::cerr << "invalid n command: expected an integer\n";
        return 2;
      }
      std::cout << structure.count_less(value) << '\n';
    } else {
      std::cerr << "unknown command: " << command << '\n';
      return 2;
    }
  }

  if (!std::cin.eof()) {
    std::cerr << "malformed input\n";
    return 2;
  }
  return 0;
}

using SelectedStructure = std::variant<rq::structures::OrderedVector,
                                       rq::structures::StdSet,
                                       rq::structures::AvlTree>;

}  // namespace

int main(int argc, char* argv[]) {
  const auto settings = rq::read_command_line(argc, argv);

  if (settings.show_help) {
    std::cout << "Usage: rangequery --structure <vector|set|avl>\n";
    return 0;
  }

  if (!settings.valid) {
    std::cerr << settings.error_message << '\n';
    return 2;
  }

  SelectedStructure structure{std::in_place_type<rq::structures::OrderedVector>};
  switch (settings.structure) {
    case rq::StructureKind::ordered_vector:
      break;
    case rq::StructureKind::std_set:
      structure.emplace<rq::structures::StdSet>();
      break;
    case rq::StructureKind::avl:
      structure.emplace<rq::structures::AvlTree>();
      break;
  }

  return std::visit([](auto& selected) { return run_commands(selected); }, structure);
}
