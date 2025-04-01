#include "tir/InsnMapsBuilder.hpp"
#include "tir/Node.hpp"

namespace tir {
std::shared_ptr<Node> InsnMapsBuilder::visit(std::shared_ptr<Node> n) {
  if (!n)
    return nullptr;
  n->buildInsnMapsEnter(*this);
  n->visitChildren(*this);
  std::shared_ptr<Node> ret = n->buildInsnMaps(*this);
  return ret;
}
} // namespace tir
