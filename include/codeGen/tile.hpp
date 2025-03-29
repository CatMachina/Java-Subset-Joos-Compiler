#pragma once

#include "codeGen/assembly.hpp"

#include <climits>

namespace codegen {

class Tile;

using StmtTile = std::shared_ptr<Tile>;
using ExprTile = std::pair<std::shared_ptr<Tile>, std::string>;
using AssemblyInstruction = std::shared_ptr<assembly::instruction>;

// view each assembly instruction as a tile that covers some part of the IR
// statement tree
class Tile {
  // instructions that implement this tile
  std::vector<std::variant<AssemblyInstruction, StmtTile, ExprTile>>
      instructions;

  // cost of this tile
  int cost;

  // whether this tile is cost calculated
  bool isCalculated;

public:
  Tile(std::vector<std::variant<AssemblyInstruction, StmtTile, ExprTile>>
           instructions = {})
      : instructions(instructions) {
    if (instructions.empty()) {
      cost = INT_MAX;
      isCalculated = true;
    } else {
      cost = 0;
      isCalculated = false;
    }
  }
};

} // namespace codegen
