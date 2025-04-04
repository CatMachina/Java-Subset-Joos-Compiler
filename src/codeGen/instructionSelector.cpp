#include "codeGen/instructionSelector.hpp"

namespace codegen {

size_t InstructionSelector::virtualRegCounter = 0;

ExprTile InstructionSelector::selectTile(std::shared_ptr<tir::Expr> expr,
                                         const std::string &regName) {
  if (exprTileCache.find(expr) != exprTileCache.end()) {
    return std::make_pair(exprTileCache[expr], regName);
  }

  // constructed tile for the expr
  std::shared_ptr<Tile> tile = std::make_shared<Tile>();

  if (auto binOp = std::dynamic_pointer_cast<tir::BinOp>(expr)) {
    auto leftRegString = newVirtualRegister();
    auto rightRegString = newVirtualRegister();
    auto leftReg =
        std::make_shared<codegen::assembly::RegisterOp>(leftRegString);
    auto rightReg =
        std::make_shared<codegen::assembly::RegisterOp>(rightRegString);

    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        selectTile(binOp->getLeft(), leftRegString),
        selectTile(binOp->getRight(), rightRegString)});

    switch (binOp->op) {
    case tir::BinOp::OpType::ADD:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Lea>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::MemAddrOp>(leftRegString,
                                                    rightRegString)),
      });
      break;

    case tir::BinOp::OpType::SUB:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Sub>(leftReg, rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              leftReg),
      });
      break;

    case tir::BinOp::OpType::MUL:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              leftReg),
          std::make_shared<assembly::IMul>(rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX)),
      });
      break;

    case tir::BinOp::OpType::DIV:
      /*
      check for divide by zero
      jump to error if divisor is 0
      else move lower 32 bits to eax
      sign extend eax to edx
      do idiv
      quotient is in eax
      */
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              rightReg, std::make_shared<assembly::ImmediateOp>(0)),
          std::make_shared<assembly::Je>(
              std::make_shared<assembly::LabelOp>("__EXEPTION__")),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              leftReg),
          std::make_shared<assembly::Cdq>(),
          std::make_shared<assembly::IDiv>(rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX)),
      });
      break;

    case tir::BinOp::OpType::MOD:
      // same as divison, but remainder is in edx
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              rightReg, std::make_shared<assembly::ImmediateOp>(0)),
          std::make_shared<assembly::Je>(
              std::make_shared<assembly::LabelOp>("__EXEPTION__")),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              leftReg),
          std::make_shared<assembly::Cdq>(),
          std::make_shared<assembly::IDiv>(rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EDX)),
      });
      break;

    case tir::BinOp::OpType::AND:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::And>(leftReg, rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              leftReg),
      });
      break;

    case tir::BinOp::OpType::OR:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Or>(leftReg, rightReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              leftReg),
      });
      break;

    case tir::BinOp::OpType::EQ:
      /*
      setZ sets AL so we need to use MovZX
      */
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetZ>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::NEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetNZ>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::LT:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetL>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::GT:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetG>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::LEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetLE>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::GEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(leftReg, rightReg),
          std::make_shared<assembly::SetGE>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    default:
      throw std::runtime_error("Invalid binop type, should not happen!");
    }

  } else if (auto call = std::dynamic_pointer_cast<tir::Call>(expr)) {
    throw std::runtime_error("Call should not exist in canonicalized IR!");

  } else if (auto constIR = std::dynamic_pointer_cast<tir::Const>(expr)) {
    // 32 bit
    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
            std::make_shared<assembly::ImmediateOp>(constIR->getValue())),
    });

  } else if (auto eseq = std::dynamic_pointer_cast<tir::ESeq>(expr)) {
    throw std::runtime_error("ESeq should not exist in cannoticalized IR!");

  } else if (auto mem = std::dynamic_pointer_cast<tir::Mem>(expr)) {
    auto addressReg = newVirtualRegister();
    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        selectTile(mem->getAddress(), addressReg),
        std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
            std::make_shared<assembly::MemAddrOp>(addressReg)),
    });

  } else if (auto name = std::dynamic_pointer_cast<tir::Name>(expr)) {
    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
            std::make_shared<assembly::LabelOp>(name->getName())),
    });

  } else if (auto temp = std::dynamic_pointer_cast<tir::Temp>(expr)) {

    // special case: abstract return is r32 eax
    if (temp->getName() == codeGenLabels->kAbstractReturn) {
      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX))});
    }
    // special case: kAbstractArgPrefixa a is stack offset from caller
    else if (temp->getName().rfind(codeGenLabels->kAbstractArgPrefix, 0) == 0) {
      std::string suffix =
          temp->getName().substr(codeGenLabels->kAbstractArgPrefix.length());
      int argNum = std::stoi(suffix);

      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::MemAddrOp>(assembly::R32_ESP,
                                                    (argNum + 2) * 4))});
    }
    // special case: static variable is in data segment
    else if (temp->isGlobal) {
      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::MemAddrOp>(temp->getName()))});
    } else {
      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(std::string("%") + "temp" +
                                                     "%" + temp->getName()))});
    }

  } else {
    throw std::runtime_error("Invalid expression type, should not happen!");
  }

  if (tile->getCost() == INT_MAX) {
    throw std::runtime_error("No tile for expression!");
  }

  auto currentOptimalTile = exprTileCache[expr];
  if (tile->getCost() < currentOptimalTile->getCost()) {
    exprTileCache[expr] = tile;
  }

  return std::make_pair(exprTileCache[expr], regName);
}

StmtTile InstructionSelector::selectTile(std::shared_ptr<tir::Stmt> stmt) {
  if (stmtTileCache.find(stmt) != stmtTileCache.end()) {
    return stmtTileCache[stmt];
  }

  std::shared_ptr<Tile> tile = std::make_shared<Tile>();

  if (auto cjump = std::dynamic_pointer_cast<tir::CJump>(stmt)) {
    auto binOp = std::dynamic_pointer_cast<tir::BinOp>(cjump->getCondition());
    ExprTile exprTile = selectTile(binOp, Tile::VIRTUAL_REG);
    auto je = std::make_shared<assembly::Je>(
        std::make_shared<assembly::RegisterOp>(exprTile.second));
    tile = std::make_shared<Tile>(
        std::vector<TileInstruction>({exprTile.first, je}));
  } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>(stmt)) {
    std::shared_ptr<tir::Expr> name =
        std::dynamic_pointer_cast<tir::Name>(jump->getName());
    ExprTile exprTile = selectTile(name, Tile::VIRTUAL_REG);
    auto jmp = std::make_shared<assembly::Jmp>(
        std::make_shared<assembly::RegisterOp>(exprTile.second));
    tile = std::make_shared<Tile>(
        std::vector<TileInstruction>({exprTile.first, jmp}));
  }
  // TODO

  if (tile->getCost() == INT_MAX) {
    throw std::runtime_error("No tile for statement!");
  }

  auto currentOptimalTile = stmtTileCache[stmt];
  if (tile->getCost() < currentOptimalTile->getCost()) {
    stmtTileCache[stmt] = tile;
  }

  return stmtTileCache[stmt];
}

} // namespace codegen
