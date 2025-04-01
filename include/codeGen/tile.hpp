#pragma once

#include "codeGen/assembly.hpp"

#include <climits>
#include <list>

namespace codegen {

class Tile;

using StmtTile = std::shared_ptr<Tile>;
using ExprTile = std::pair<std::shared_ptr<Tile>, std::string>;
using AssemblyInstruction = std::shared_ptr<assembly::instruction>;
using TileInstruction = std::variant<AssemblyInstruction, StmtTile, ExprTile>;

// view each assembly instruction as a tile that covers some part of the IR
// statement tree
class Tile {
  // instructions that implement this tile
  std::vector<TileInstruction> instructions;

  // cost of this tile
  int cost;

  // whether this tile is cost calculated
  bool isCalculated;

  void calculateCost() {
    cost = 0;
    for (auto &instruction : instructions) {
      if (auto assemblyInstruction =
              std::get_if<AssemblyInstruction>(&instruction)) {
        cost += 1;
      } else if (auto stmtTile = std::get_if<StmtTile>(&instruction)) {
        cost += stmtTile->getCost();
      } else if (auto exprTile = std::get_if<ExprTile>(&instruction)) {
        cost += exprTile->first->getCost();
      }
    }
  }

public:
  Tile(std::vector<TileInstruction> instructions = {})
      : instructions(instructions) {
    if (instructions.empty()) {
      cost = INT_MAX;
      isCalculated = true;
    } else {
      cost = 0;
      isCalculated = false;
    }
  }

  void addInstructions(const std::vector<TileInstruction> &instructions,
                       bool start = false) {
    isCalculated = false;

    if (start) {
      this->instructions.insert(this->instructions.begin(),
                                instructions.begin(), instructions.end());
    } else {
      this->instructions.insert(this->instructions.end(), instructions.begin(),
                                instructions.end());
    }
  }

  int getCost() {
    if (!isCalculated) {
      cost = 0;
      calculateCost();
      isCalculated = true;
    }
    return cost;
  }

  static const inline std::string VIRTUAL_REG = "__PLACEHOLDER_VIRTUAL_REG__";

  void assignVirtual(std::string reg) {
    for (auto &instruction : instructions) {
      if (auto assemblyInstruction =
              std::get_if<AssemblyInstruction>(&instruction)) {
        assemblyInstruction->replaceRegister(Tile::VIRTUAL_REG, reg);
      } else if (auto stmtTile = std::get_if<StmtTile>(&instruction)) {
      } else if (auto exprTile = std::get_if<ExprTile>(&instruction)) {
        if (exprTile.second == reg) {
          exprTile.first->assignVirtual(exprTile.second);
        }
      } else {
        throw std::runtime_error(
            "Instruction is not an AssemblyInstruction, StmtTile, or ExprTile")
      }
    }
  }

  std::list<AssemblyInstruction> getInstructions() {
    std::list<AssemblyInstruction> returnList;

    for (auto instruction : instructions) {
      if (auto assemblyInstruction =
              std::get_if<AssemblyInstruction>(&instruction)) {
        returnList.push_back(assemblyInstruction);
      } else if (auto stmtTile = std::get_if<StmtTile>(&instruction)) {
        for (auto stmtInstruction : stmtTile->getInstructions()) {
          returnList.push_back(stmtInstruction);
        }
      } else if (auto exprTile = std::get_if<ExprTile>(&instruction)) {
        exprTile.first->assignVirtual(exprTile.second);
        for (auto exprInstruction : exprTile.first->getInstructions()) {
          returnList.push_back(exprInstruction);
        }
      } else {
        throw std::runtime_error(
            "Instruction is not an AssemblyInstruction, StmtTile, or ExprTile")
      }
    }
  }
};

} // namespace codegen
