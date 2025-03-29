#pragma once

namespace codegen::assembly {

// operand of assembly instruction can be:
// memory address, label use, register (real or abstract) or immediate
class Operand {
  bool isRead = false;
  bool isWrite = false;

protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

  void setRead() { isRead = true; }
  void setWrite() { isWrite = true; }

public:
  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
};

// x86 memory operands
// x86 addressing mode format:
// [base + index * scale + disp]
class MemAddrOp : public Operand {
  std::string base = "";
  std::string index;
  int scale;
  int disp;

public:
  EffectiveAddress(std::string base, std::string index = "", int scale = 1,
                   int displacement = 0)
      : Operand{}, base{base}, index{index}, scale{scale}, disp{displacement} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(MemAddrOp " << base << " + " << index << " * "
              << std::to_string(scale) << " - " << std::to_string(disp)
              << ")\n";
  }
};

// labels used in places like jumps or data references
class LabelOp : public Operand {
  std::string label;

public:
  LabelOp(std::string label) : Operand{}, label{label} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(LabelOp " << label << ")\n";
  }
};

// register operand like "eax"
class RegisterOp : public Operand {
  std::string reg;

public:
  RegisterOp(std::string reg) : Operand{}, reg{reg} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(RegisterOp " << reg << ")\n";
  }
};

// immediate constant
class ImmediateOp : public Operand {
  int value;

public:
  ImmediateOp(int value) : Operand{}, value{value} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(ImmediateOp " << std::to_string(value) << ")\n";
  }
};

} // namespace codegen::assembly
