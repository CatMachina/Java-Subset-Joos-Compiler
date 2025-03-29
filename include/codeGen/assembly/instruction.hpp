#pragma once

#include "codeGen/assembly/operand.hpp"
#include <memory>
#include <string>
#include <vector>

namespace codegen::assembly {

class Instruction {
  // used operands
  std::vector<std::shared_ptr<Operand>> operands;

protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

public:
  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
};

// mov	move data from src to dest

// add, sub, imul, idiv	arithmetic operations

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
