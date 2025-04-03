#pragma once

#include "codeGen/assembly/operand.hpp"
#include <memory>
#include <string>
#include <vector>

namespace codegen::assembly {

class Instruction {
  // used operands
  std::vector<std::shared_ptr<Operand>> operands;

  // general purpose registers the instruction always use
  // eg. imul uses 32 bit eax and edx
  std::unordered_set<std::string> readGPRs;
  std::unordered_set<std::string> writeGPRs;

protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

  void addOperand(std::shared_ptr<Operand> operand) {
    operands.push_back(operand);
  }
  void addReadGPR(std::string reg) { readGPRs.insert(reg); }
  void addWriteGPR(std::string reg) { writeGPRs.insert(reg); }

  const std::vector<std::shared_ptr<Operand>> &getOperands() const {
    return operands;
  }

private:
  // Expand register set with overlaps and remove non-registers (e.g., global
  // data)
  void sanitizeRegisterSet(std::unordered_set<std::string> &regs) {
    std::unordered_set<std::string> originalRegs = regs;

    // Add overlapping registers
    for (const auto &reg : originalRegs) {
      auto it = assembly::REG_TO_ALIAS_GROUP.find(reg);
      if (it != assembly::REG_TO_ALIAS_GROUP.end()) {
        for (const auto &overlapReg : *it->second) {
          regs.insert(overlapReg);
        }
      }
    }

    // Remove non-register entries (like global data)
    std::unordered_set<std::string> sanitizedRegs = regs;
    for (const auto &reg : sanitizedRegs) {
      CodeGenLabels dummy;
      if (reg.rfind(dummy.kGlobalPrefix, 0) == 0) {
        regs.erase(reg);
      }
    }
  }

public:
  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
  virtual std::string toString() const = 0;

  void replaceRegister(std::string oldReg, std::string newReg) {
    for (auto &operand : operands) {
      if (auto memAddrOp = std::dynamic_pointer_cast<MemAddrOp>(operand)) {
        if (memAddrOp->getBase() == oldReg) {
          memAddrOp->setBase(newReg);
        }
        if (memAddrOp->getIndex() == oldReg) {
          memAddrOp->setIndex(newReg);
        }
      } else if (auto registerOp =
                     std::dynamic_pointer_cast<RegisterOp>(operand)) {
        if (registerOp->getReg() == oldReg) {
          registerOp->setReg(newReg);
        }
      }
    }
  }

  std::unordered_set<std::string> getReadRegisters() {
    std::unordered_set<std::string> result;

    for (auto &operand : operands) {
      if (auto &memAddrOp = std::dynamic_pointer_cast<MemAddrOp>(operand)) {
        // always read, even if the operand itself isn't
        if (memAddrOp->getBase() != "") {
          result.insert(memAddrOp->getBase());
        }
        if (memAddrOp->getIndex() != "") {
          result.insert(memAddrOp->getIndex());
        }

      } else if (auto &registerOp =
                     std::dynamic_pointer_cast<RegisterOp>(operand)) {
        if (registerOp->isRead()) {
          result.insert(registerOp->getReg());
        }
      }
    }

    for (auto &reg : readGPRs) {
      result.insert(reg);
    }
    sanitizedRegs(result);

    return std::move(result);
  }

  std::unordered_set<std::string> getWriteRegisters() {
    std::unordered_set<std::string> result;
    for (auto &operand : operands) {
      if (auto &registerOp = std::dynamic_pointer_cast<RegisterOp>(operand)) {
        if (registerOp->isWrite()) {
          result.insert(registerOp->getReg());
        }
      }
    }
    for (auto &reg : writeGPRs) {
      result.insert(reg);
    }
    sanitizedRegs(result);
    return std::move(result);
  }

  std::unordered_set<std::string> getAllUsedRegisters() {
    std::unordered_set<std::string> result;

    for (auto &operand : operands) {
      if (auto &registerOp = std::dynamic_pointer_cast<RegisterOp>(operand)) {
        result.insert(registerOp->getReg());
      } else if (auto &memAddrOp =
                     std::dynamic_pointer_cast<MemAddrOp>(operand)) {
        if (memAddrOp->getBase() != "") {
          result.insert(memAddrOp->getBase());
        }
        if (memAddrOp->getIndex() != "") {
          result.insert(memAddrOp->getIndex());
        }
      }
    }
    for (auto &reg : readGPRs) {
      result.insert(reg);
    }
    for (auto &reg : writeGPRs) {
      result.insert(reg);
    }
    sanitizedRegs(result);
    return std::move(result);
  }

  std::unordered_set<std::string> getReadVirtualRegisters() {
    std::unordered_set<std::string> result;
    for (auto &reg : getReadRegisters()) {
      if (!assembly::isGPR(reg)) {
        result.insert(reg);
      }
    }
    return std::move(result);
  }

  std::unordered_set<std::string> getWriteVirtualRegisters() {
    std::unordered_set<std::string> result;
    for (auto &reg : getWriteRegisters()) {
      if (!assembly::isGPR(reg)) {
        result.insert(reg);
      }
    }
    return std::move(result);
  }

  std::unordered_set<std::string> getAllUsedVirtualRegisters() {
    std::unordered_set<std::string> result;
    for (auto &reg : getAllUsedRegisters()) {
      if (!assembly::isGPR(reg)) {
        result.insert(reg);
      }
    }
    return std::move(result);
  }
};

// mov	move data from src to dest
class Mov : public Instruction {
public:
  Mov(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src) {
    dest->setWrite();
    src->setRead();
    addOperand(dest);
    addOperand(src);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Mov ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "mov " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

// add, sub, imul, idiv	arithmetic operations
class Add : public Instruction {
public:
  Add(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg2) {
    arg1->setWrite();
    arg1->setRead();
    arg2->setRead();
    addOperand(arg1);
    addOperand(arg2);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Add ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "add " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

class Sub : public Instruction {
public:
  Sub(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg2) {
    arg1->setWrite();
    arg1->setRead();
    arg2->setRead();
    addOperand(arg1);
    addOperand(arg2);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Sub ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "sub " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

class IMul : public Instruction {
public:
  IMul(std::shared_ptr<Operand> multiplicand) {
    multiplicand->setRead();
    addOperand(multiplicand);

    addReadGPR(assembly::R32_EAX);
    addWriteGPR(assembly::R32_EAX);

    addReadGPR(assembly::R32_EDX);
    addWriteGPR(assembly::R32_EDX);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(IMul ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "imul " + getOperands()[0]->toString();
  }
};

class IDiv : public Instruction {
public:
  IDiv(std::shared_ptr<Operand> divisor) {
    divisor->setRead();
    addOperand(divisor);

    addReadGPR(assembly::R32_EAX);
    addWriteGPR(assembly::R32_EAX);

    addReadGPR(assembly::R32_EDX);
    addWriteGPR(assembly::R32_EDX);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(IDiv ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "idiv " + getOperands()[0]->toString();
  }
};

// and, or, xor 	bitwise logical operators
class And : public Instruction {
public:
  And(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg2) {
    arg1->setWrite();
    arg1->setRead();
    arg2->setRead();
    addOperand(arg1);
    addOperand(arg2);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(And ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "and " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

class Or : public Instruction {
public:
  Or(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg2) {
    arg1->setWrite();
    arg1->setRead();
    arg2->setRead();
    addOperand(arg1);
    addOperand(arg2);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Or ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "or " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

class Xor : public Instruction {
public:
  Xor(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg2) {
    arg1->setWrite();
    arg1->setRead();
    arg2->setRead();
    addOperand(arg1);
    addOperand(arg2);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Xor ";
    getOperands()[0]->print(os);
    os << " ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "xor " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

// jmp	unconditional jump
class Jmp : public Instruction {
public:
  Jmp(std::shared_ptr<Operand> target) {
    target->setRead();
    addOperand(target);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Jmp ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "jmp " + getOperands()[0]->toString();
  }
};

// je	conditional jump on equal
class Je : public Instruction {
public:
  Je(std::shared_ptr<Operand> target) {
    target->setRead();
    addOperand(target);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Je ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "je " + getOperands()[0]->toString();
  }
};

// push, pop	stack operations (single operand)
class Push : public Instruction {
public:
  Push(std::shared_ptr<Operand> operand) {
    operand->setRead();
    addOperand(operand);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Push ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "push " + getOperands()[0]->toString();
  }
};

class Pop : public Instruction {
public:
  Pop(std::shared_ptr<Operand> operand) {
    operand->setWrite();
    addOperand(operand);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Pop ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "pop " + getOperands()[0]->toString();
  }
};

// test, cmp	perform ALU operations (and, sub) but only set condition codes.
class Test : public Instruction {
public:
  Test(std::shared_ptr<Operand> left, std::shared_ptr<Operand> right) {
    left->setRead();
    right->setRead();
    addOperand(left);
    addOperand(right);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Test ";
    getOperands()[0]->print(os);
    os << ", ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "test " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

class Cmp : public Instruction {
public:
  Cmp(std::shared_ptr<Operand> left, std::shared_ptr<Operand> right) {
    left->setRead();
    right->setRead();
    addOperand(left);
    addOperand(right);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Cmp ";
    getOperands()[0]->print(os);
    os << ", ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "cmp " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

// call	subroutine call
class Call : public Instruction {
public:
  Call(std::shared_ptr<Operand> target) {
    target->setRead();
    addOperand(target);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Call ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "call " + getOperands()[0]->toString();
  }
};

// ret	subroutine return
class Ret : public Instruction {
public:
  Ret() = default;

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Ret)\n";
    return os;
  }

  std::string toString() const override { return "ret"; }
};

// lea computes the address of a memory operand and moves that address
// into a register rather than loading the data from that address
class Lea : public Instruction {
public:
  Lea(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src) {
    dest->setWrite();
    src->setRead();
    addOperand(dest);
    addOperand(src);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Lea ";
    getOperands()[0]->print(os);
    os << ", ";
    getOperands()[1]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "lea " + getOperands()[0]->toString() + ", " +
           getOperands()[1]->toString();
  }
};

// setz, setnz, setl setg, setle, setge
// set destination to 1 or 0 based on flags from Cmp
class Setcc : public Instruction {
public:
  Setcc(std::string condition, std::shared_ptr<Operand> dest)
      : cond{condition} {
    dest->setWrite();
    addOperand(dest);
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(Set" << condition << " ";
    getOperands()[0]->print(os);
    os << ")\n";
    return os;
  }

  std::string toString() const override {
    return "set" + condition + " " + getOperands()[0]->toString();
  }

private:
  std::string condition;
};

} // namespace codegen::assembly
