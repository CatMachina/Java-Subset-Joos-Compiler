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
  Add(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg1) {
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
  Sub(std::shared_ptr<Operand> arg1, std::shared_ptr<Operand> arg1) {
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

// jmp	unconditional jump

// je	conditional jump on equal

// push, pop	stack operations (single operand)

// test, cmp	perform ALU operations (and, sub) but only set condition codes.

// call	subroutine call

// ret	subroutine return

// lea computes the address of a memory operand and moves that address
// into a register rather than loading the data from that address

// setz, setnz, setl setg, setle, setge
// set destination to 1 or 0 based on flags from Cmp

} // namespace codegen::assembly
