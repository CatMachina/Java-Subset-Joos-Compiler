#include "codeGen/instructionSelector.hpp"

#include <regex>

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

    // std::cout << "newVirtualRegister for binOp: " << leftRegString
    //           << ", " << rightRegString << std::endl;

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
          std::make_shared<assembly::Sub>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
      });
      break;

    case tir::BinOp::OpType::MUL:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
          std::make_shared<assembly::IMul>(
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
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
          std::make_shared<assembly::Comment>("Divide " + leftRegString +
                                              " by " + rightRegString),
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString),
              std::make_shared<assembly::ImmediateOp>(0)),
          std::make_shared<assembly::Je>(
              std::make_shared<assembly::LabelOp>("__exception")),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
          std::make_shared<assembly::Cdq>(),
          std::make_shared<assembly::IDiv>(
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX)),
      });
      break;

    case tir::BinOp::OpType::MOD:
      // same as divison, but remainder is in edx
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Comment>("MOD " + leftRegString + " by " +
                                              rightRegString),
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString),
              std::make_shared<assembly::ImmediateOp>(0)),
          std::make_shared<assembly::Je>(
              std::make_shared<assembly::LabelOp>("__exception")),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_EAX),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
          std::make_shared<assembly::Cdq>(),
          std::make_shared<assembly::IDiv>(
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R32_EDX)),
      });
      break;

    case tir::BinOp::OpType::AND:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::And>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
      });
      break;

    case tir::BinOp::OpType::OR:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Or>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString)),
      });
      break;

    case tir::BinOp::OpType::EQ:
      /*
      setZ sets AL so we need to use MovZX
      */
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::SetZ>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::NEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::SetNZ>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::LT:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::SetL>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::GT:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::SetG>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::LEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
          std::make_shared<assembly::SetLE>(
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
          std::make_shared<assembly::MovZX>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::RegisterOp>(assembly::R8_AL)),
      });
      break;

    case tir::BinOp::OpType::GEQ:
      tile->addInstructions(std::vector<TileInstruction>{
          std::make_shared<assembly::Cmp>(
              std::make_shared<codegen::assembly::RegisterOp>(leftRegString),
              std::make_shared<codegen::assembly::RegisterOp>(rightRegString)),
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
    // std::cout << "newVirtualRegister for mem: " << addressReg << std::endl;
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
      // std::string suffix =
      //     temp->getName().substr(codeGenLabels->kAbstractArgPrefix.length());
      // int argNum = std::stoi(suffix);
      int argNum = std::stoi(std::regex_replace(
          temp->getName(), std::regex(codeGenLabels->kAbstractArgPrefix), ""));

      // std::cout << "for " << temp->getName() << ", argNum: " << argNum <<
      // std::endl;

      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Mov>(
              std::make_shared<assembly::RegisterOp>(Tile::VIRTUAL_REG),
              std::make_shared<assembly::MemAddrOp>(assembly::R32_EBP,
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
              std::make_shared<assembly::RegisterOp>(std::string("%_") +
                                                     temp->getName()))});
    }

  } else {
    // std::cout << "invalid expr type?" << std::endl;
    expr->print(std::cout);
    throw std::runtime_error("Invalid expression type, should not happen!");
  }

  if (tile->getCost() == INT_MAX) {
    throw std::runtime_error("No tile for expression!");
  }

  auto currentOptimalTile = exprTileCache[expr];
  if (!currentOptimalTile || tile->getCost() < currentOptimalTile->getCost()) {
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
    auto conditionReg = newVirtualRegister();
    // std::cout << "newVirtualRegister for cjump: " << conditionReg <<
    // std::endl;

    ExprTile exprTile = selectTile(cjump->getCondition(), conditionReg);
    auto test = std::make_shared<assembly::Test>(
        std::make_shared<assembly::RegisterOp>(conditionReg),
        std::make_shared<assembly::RegisterOp>(conditionReg));
    auto jnz = std::make_shared<assembly::JNZ>(
        std::make_shared<assembly::LabelOp>(cjump->getTrueLabel()));

    tile = std::make_shared<Tile>(
        std::vector<TileInstruction>({exprTile, test, jnz}));

  } else if (auto jump = std::dynamic_pointer_cast<tir::Jump>(stmt)) {
    if (auto name = std::dynamic_pointer_cast<tir::Name>(jump->getTarget())) {
      tile = std::make_shared<Tile>(
          std::vector<TileInstruction>{std::make_shared<assembly::Jmp>(
              std::make_shared<assembly::LabelOp>(name->getName()))});
    } else {
      auto targetReg = newVirtualRegister();
      // std::cout << "newVirtualRegister for jump: " << targetReg << std::endl;
      tile = std::make_shared<Tile>(std::vector<TileInstruction>{
          selectTile(jump->getTarget(), targetReg),
          std::make_shared<assembly::Jmp>(
              std::make_shared<assembly::RegisterOp>(targetReg))});
    }

  } else if (auto callstmt = std::dynamic_pointer_cast<tir::CallStmt>(stmt)) {
    std::string functionName = "";
    auto call = callstmt->getCall();
    if (auto name = std::dynamic_pointer_cast<tir::Name>(call->getTarget())) {
      functionName = name->getName();
    }

    // special case: malloc
    if (functionName == "__malloc") {
      if (call->getNumArgs() != 1) {
        throw std::runtime_error(
            "Invalid number of arguments for malloc, got " +
            std::to_string(call->getNumArgs()));
      }
      tile = std::make_shared<Tile>(std::vector<TileInstruction>{
          selectTile(call->getArgs()[0], assembly::R32_EAX),
          std::make_shared<assembly::Call>(
              std::make_shared<assembly::LabelOp>(functionName))});

    }

    // regular case
    else {
      // push args to stack reverse order (C decl)
      for (auto &arg : call->getArgs()) {
        std::string argReg = newVirtualRegister();
        // std::cout << "newVirtualRegister for arg: " << argReg << std::endl;
        tile->addInstructions(
            std::vector<TileInstruction>{
                selectTile(arg, argReg),
                std::make_shared<assembly::Push>(
                    std::make_shared<assembly::RegisterOp>(argReg))},
            true);
      }

      // perform call
      if (functionName != "") {
        tile->addInstructions(
            std::vector<TileInstruction>{std::make_shared<assembly::Call>(
                std::make_shared<assembly::LabelOp>(functionName))});
      } else {
        auto targetReg = newVirtualRegister();
        // std::cout << "newVirtualRegister for callstmt: " << targetReg <<
        // std::endl;
        tile->addInstructions(std::vector<TileInstruction>{
            selectTile(call->getTarget(), targetReg),
            std::make_shared<assembly::Call>(
                std::make_shared<assembly::RegisterOp>(targetReg))});
      }

      // stack pointer update
      tile->addInstructions(
          std::vector<TileInstruction>{std::make_shared<assembly::Add>(
              std::make_shared<assembly::RegisterOp>(assembly::R32_ESP),
              std::make_shared<assembly::ImmediateOp>(call->getNumArgs() *
                                                      4))});
    }

  }

  else if (auto exp = std::dynamic_pointer_cast<tir::Exp>(stmt)) {
    throw std::runtime_error(
        "Exp should not exist in canonicalized statements");

  } else if (auto label = std::dynamic_pointer_cast<tir::Label>(stmt)) {
    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        std::make_shared<assembly::Label>(label->getName())});

  } else if (auto move = std::dynamic_pointer_cast<tir::Move>(stmt)) {
    auto target = move->getTarget();

    if (auto temp = std::dynamic_pointer_cast<tir::Temp>(target)) {
      if (temp->isGlobal) {
        auto tempReg = newVirtualRegister();
        // std::cout << "newVirtualRegister for move: " << tempReg << std::endl;
        tile = std::make_shared<Tile>(std::vector<TileInstruction>{
            selectTile(move->getSource(), tempReg),
            std::make_shared<assembly::Mov>(
                std::make_shared<assembly::MemAddrOp>(temp->getName()),
                std::make_shared<assembly::RegisterOp>(tempReg))});
      } else {
        tile = std::make_shared<Tile>(std::vector<TileInstruction>{
            selectTile(move->getSource(), "%_" + temp->getName())});
      }

    } else if (auto mem = std::dynamic_pointer_cast<tir::Mem>(target)) {
      auto targetReg = newVirtualRegister();
      auto sourceReg = newVirtualRegister();
      // std::cout << "newVirtualRegister for move: " << targetReg << ", " <<
      // sourceReg << std::endl; std::cout << "mem->getAddress(): ";
      // mem->getAddress()->print(std::cout);
      // std::cout << "\nmove->getSource(): ";
      // move->getSource()->print(std::cout);
      tile = std::make_shared<Tile>(std::vector<TileInstruction>{
          selectTile(move->getSource(), sourceReg),
          selectTile(mem->getAddress(), targetReg),
          std::make_shared<assembly::Mov>(
              std::make_shared<assembly::MemAddrOp>(targetReg),
              std::make_shared<assembly::RegisterOp>(sourceReg))});

    } else {
      throw std::runtime_error("Invalid move target");
    }

  } else if (auto returnIR = std::dynamic_pointer_cast<tir::Return>(stmt)) {
    if (auto ret = returnIR->getRet()) {
      tile->addInstructions(std::vector<TileInstruction>{
          selectTile(ret, assembly::R32_EAX),
      });
    }
    tile->addInstructions(std::vector<TileInstruction>{
        std::make_shared<assembly::Comment>("function epilogue"),
        std::make_shared<assembly::Mov>(
            std::make_shared<assembly::RegisterOp>(assembly::R32_ESP),
            std::make_shared<assembly::RegisterOp>(assembly::R32_EBP)),
        std::make_shared<assembly::Pop>(
            std::make_shared<assembly::RegisterOp>(assembly::R32_EBP)),
        std::make_shared<assembly::Ret>()});

  } else if (auto seq = std::dynamic_pointer_cast<tir::Seq>(stmt)) {
    for (auto &stmt : seq->getStmts()) {
      tile->addInstructions(std::vector<TileInstruction>{selectTile(stmt)});
    }

  } else if (auto comment = std::dynamic_pointer_cast<tir::Comment>(stmt)) {
    tile = std::make_shared<Tile>(std::vector<TileInstruction>{
        std::make_shared<assembly::Comment>(comment->getComment())});

  } else {
    // std::cout << "Invalid statement:" << std::endl;
    stmt->print(std::cout);
    throw std::runtime_error("Invalid statement");
  }

  if (tile->getCost() == INT_MAX) {
    throw std::runtime_error("No tile for statement!");
  }

  auto currentOptimalTile = stmtTileCache[stmt];
  if (!currentOptimalTile || tile->getCost() < currentOptimalTile->getCost()) {
    stmtTileCache[stmt] = tile;
  }

  return stmtTileCache[stmt];
}

} // namespace codegen
