#include "tir/Simulator.hpp"

namespace tir {

// Can toggle this for debug output
int Simulator::debugLevel = 0;

void Simulator::leave(std::shared_ptr<ExecutionFrame> frame) {
  std::cout << "leave: ";
  std::cout << frame->getCurrentInsn()->label() << "\n";
  std::shared_ptr<Node> insn = frame->getCurrentInsn();
  if (auto constNode = std::dynamic_pointer_cast<Const>(insn))
    exprStack->pushValue(constNode->getValue());
  else if (auto tempNode = std::dynamic_pointer_cast<Temp>(insn)) {
    std::string tempName = tempNode->getName();
    exprStack->pushTemp(frame->get(tempName), tempName);
  } else if (auto binOpNode = std::dynamic_pointer_cast<BinOp>(insn)) {
    int r = exprStack->popValue();
    int l = exprStack->popValue();
    int result;
    switch (binOpNode->op) {
    case BinOp::OpType::ADD:
      result = l + r;
      break;
    case BinOp::OpType::SUB:
      result = l - r;
      break;
    case BinOp::OpType::MUL:
      result = l * r;
      break;
    case BinOp::OpType::DIV:
      if (r == 0)
        throw new std::runtime_error("Division by zero!");
      result = l / r;
      break;
    case BinOp::OpType::MOD:
      if (r == 0)
        throw new std::runtime_error("Division by zero!");
      result = l % r;
      break;
    case BinOp::OpType::AND:
      result = l & r;
      break;
    case BinOp::OpType::OR:
      result = l | r;
      break;
    /** TODO: These operations aren't handled yet */
    // case BinOp::OpType::XOR:
    //   result = l ^ r;
    //   break;
    // case BinOp::OpType::LSHIFT:
    //   result = l << r;
    //   break;
    // case BinOp::OpType::RSHIFT:
    //   result = l >>> r;
    //   break;
    // case BinOp::OpType::ARSHIFT:
    //   result = l >> r;
    //   break;
    case BinOp::OpType::EQ:
      result = l == r ? 1 : 0;
      break;
    case BinOp::OpType::NEQ:
      result = l != r ? 1 : 0;
      break;
    case BinOp::OpType::LT:
      result = l < r ? 1 : 0;
      break;
    case BinOp::OpType::GT:
      result = l > r ? 1 : 0;
      break;
    case BinOp::OpType::LEQ:
      result = l <= r ? 1 : 0;
      break;
    case BinOp::OpType::GEQ:
      result = l >= r ? 1 : 0;
      break;
    default:
      throw std::runtime_error("Invalid binary operation");
    }
    exprStack->pushValue(result);
  } else if (auto memNode = std::dynamic_pointer_cast<Mem>(insn)) {
    int addr = exprStack->popValue();
    exprStack->pushAddr(read(addr), addr);
  } else if (auto callNode = std::dynamic_pointer_cast<Call>(insn)) {
    int argsCount = callNode->getNumArgs();
    std::vector<int> args(argsCount, 0);
    for (int i = argsCount - 1; i >= 0; --i)
      args[i] = exprStack->popValue();
    std::shared_ptr<StackItem> target = exprStack->pop();
    std::string targetName;
    if (target->type == StackItem::Kind::NAME)
      targetName = target->name;
    else if (indexToInsn.count(target->value)) {
      std::shared_ptr<Node> node = indexToInsn[target->value];
      if (auto funcDecl = std::dynamic_pointer_cast<FuncDecl>(node))
        targetName = funcDecl->getName();
      else
        throw std::runtime_error("Call to a non-function instruction!");
    } else
      throw std::runtime_error("Invalid function call '" + insn->label() +
                               "'(target '" + std::to_string(target->value) +
                               "' is unknown)!");

    int retVal = call(frame, targetName, args);
    exprStack->pushValue(retVal);
  } else if (auto nameNode = std::dynamic_pointer_cast<Name>(insn)) {
    std::string name = nameNode->getName();
    exprStack->pushName(libraryFunctions.contains(name) ? -1 : findLabel(name),
                        name);
  } else if (auto moveNode = std::dynamic_pointer_cast<Move>(insn)) {
    std::cout << "move begin\n";
    int r = exprStack->popValue();
    std::cout << "popped value\n";
    std::shared_ptr<StackItem> stackItem = exprStack->pop();
    std::cout << "popped item\n";
    switch (stackItem->type) {
    case StackItem::MEM:
      if (debugLevel > 0)
        std::cout << "mem[" << stackItem->addr << "]=" << r << std::endl;
      store(stackItem->addr, r);
      std::cout << "stored stack Item\n";
      break;
    case StackItem::TEMP:
      if (debugLevel > 0)
        std::cout << "temp[" << stackItem->temp << "]=" << r << std::endl;
      frame->put(stackItem->temp, r);
      std::cout << "frame put stack Item\n";
      break;
    default:
      throw std::runtime_error("Invalid MOVE!");
    }
  } else if (auto expNode = std::dynamic_pointer_cast<Exp>(insn)) {
    // Discard result.
    exprStack->pop();
  } else if (auto jumpNode = std::dynamic_pointer_cast<Jump>(insn))
    frame->setIP(exprStack->popValue());
  else if (auto irCJump = std::dynamic_pointer_cast<CJump>(insn)) {
    int top = exprStack->popValue();
    std::string label;
    if (top == 0)
      label = irCJump->getFalseLabel();
    else if (top == 1)
      label = irCJump->getTrueLabel();
    else
      throw std::invalid_argument("Invalid value in CJUMP: expected 0/1, got " +
                                  top);
    if (!label.empty())
      frame->setIP(findLabel(label));
  } else if (auto returnNode = std::dynamic_pointer_cast<Return>(insn)) {
    frame->ret = exprStack->popValue();
    frame->setIP(-1);
  }
}

} // namespace tir