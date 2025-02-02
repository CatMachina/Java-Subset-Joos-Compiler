#include "parseTree.hpp"

namespace parsetree {

bool Node::is_corrupted() const {
  if (type == Type::Corrupted) {
    const Corrupted *corrupt_node = dynamic_cast<const Corrupted *>(this);
    std::cout << "Corrupted Node: ";
    corrupt_node->print(std::cout);
    std::cout << "\n";
    return true;
  }
  for (size_t i = 0; i < num_args; ++i) {
    if (args[i] == nullptr)
      continue;
    if (args[i]->is_corrupted())
      return true;
  }
  return false;
}

} // namespace parsetree