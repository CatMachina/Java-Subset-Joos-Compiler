#include "codeGen/instructionSelector.hpp"

namespace codegen {

size_t InstructionSelector::virtualRegCounter = 0;

ExprTile InstructionSelector::selectTile(std::shared_ptr<tir::Expr> expr,
                                         const std::string &regName) {
  if (exprTileCache.find(expr) != exprTileCache.end()) {
    return std::make_pair(exprTileCache[expr], regName);
  }

  // generic tile if no specialized tile applicable
  std::shared_ptr<Tile> tile = std::make_shared<Tile>();

  if (auto binOp = std::dynamic_pointer_cast<tir::BinOp>(expr)) {

  } else if (auto call = std::dynamic_pointer_cast<tir::Call>(expr)) {
    throw std::runtime_error("Call should not exist in canonicalized IR!");

  } else if (auto constIR = std::dynamic_pointer_cast<tir::Const>(expr)) {

  } else if (auto eseq = std::dynamic_pointer_cast<tir::ESeq>(expr)) {
    throw std::runtime_error("ESeq should not exist in cannoticalized IR!");

  } else if (auto mem = std::dynamic_pointer_cast<tir::Mem>(expr)) {

  } else if (auto name = std::dynamic_pointer_cast<tir::Name>(expr)) {

  } else if (auto temp = std::dynamic_pointer_cast<tir::Temp>(expr)) {

  } else {
    throw std::runtime_error("Invalid expression type, should not happen!");
  }
}

} // namespace codegen
