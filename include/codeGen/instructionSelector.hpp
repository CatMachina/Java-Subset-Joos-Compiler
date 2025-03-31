#pragma once

#include "codeGen/assembly.hpp"
#include "codeGen/codeGenLabels.hpp"
#include "codeGen/tile.hpp"
#include "tir/TIR.hpp"

namespace codegen {

/*
Since we are connecting tiles with fresh temporaries,
the cost of executing a tiling can be approximated by:
some fixed tile cost plus the total cost of the tiled subtrees.

Assuming this approximation is accurate,
the tiling problem exhibits optimal substructure:
the best possible tiling of an IR node uses best possible tilings of its
subtrees.

we can use dynamic programming or memoization to accelerate it.
The key is to remember and reuse the optimal tilings that are found.

Optimal tiling:
*/
class InstructionSelector {
  // computed best tile for the subtree rooted at every IR in the IR tree
  std::unordered_map<std::shared_ptr<tir::Expr>, std::shared_ptr<Tile>>
      exprTileCache;
  std::unordered_map<std::shared_ptr<tir::Stmt>, std::shared_ptr<Tile>>
      stmtTileCache;

  std::shared_ptr<CodeGenLabels> codeGenLabels;

  // static for new virtual register name
  static size_t virtualRegCounter;
  static std::string newVirtualRegister() {
    return "%_vreg" + std::to_string(virtualRegCounter++);
  }

public:
  InstructionSelector(std::shared_ptr<CodeGenLabels> codeGenLabels)
      : codeGenLabels(codeGenLabels) {}

  // ExprTile and StmtTile defined in tile.hpp
  ExprTile selectTile(std::shared_ptr<tir::Expr> expr,
                      const std::string &regName);
  StmtTile selectTile(std::shared_ptr<tir::Stmt> stmt);
};

} // namespace codegen
