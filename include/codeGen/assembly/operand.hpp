#pragma once

#include <ostream>
#include <string>

namespace codegen::assembly {

// operand of assembly instruction can be:
// memory address, label use, register (real or abstract) or immediate
class Operand {
  bool isRead_ = false;
  bool isWrite_ = false;

protected:
  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

public:
  void setRead() { isRead_ = true; }
  void setWrite() { isWrite_ = true; }

  bool isRead() const { return isRead_; }
  bool isWrite() const { return isWrite_; }

  virtual std::ostream &print(std::ostream &os, int indent = 0) const = 0;
  virtual std::string toString() const = 0;
  virtual ~Operand() {}
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
  MemAddrOp(std::string base, std::string index = "", int scale = 1,
            int displacement = 0)
      : base{base}, index{index}, scale{scale}, disp{displacement} {}

  MemAddrOp(std::string base, int displacement)
      : base{base}, disp{displacement} {
    index = "";
    scale = 1;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(MemAddrOp " << base << " + " << index << " * "
              << std::to_string(scale) << " - " << std::to_string(disp)
              << ")\n";
  }

  std::string getBase() const { return base; }
  void setBase(std::string base) { this->base = base; }

  std::string getIndex() const { return index; }
  void setIndex(std::string index) { this->index = index; }

  std::string toString() const override {
    if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
      throw std::runtime_error("scale must be 1, 2, 4, 8, but got " +
                               std::to_string(scale));
    }

    std::string output = base;
    if (index != "") {
      output += " + ";
      if (scale != 1) {
        output += "(" + index + " * " + std::to_string(scale) + ")";
      } else {
        output += index;
      }
    }

    if (disp != 0) {
      output += (disp > 0) ? " + " : " - ";
      output += std::to_string(std::abs(disp));
    }
    return "[" + output + "]";
  }
};

// labels used in places like jumps or data references
class LabelOp : public Operand {
  std::string label;

public:
  LabelOp(std::string label) : label{label} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(LabelOp " << label << ")\n";
  }

  std::string toString() const override { return label; }
};

// register operand like "eax"
class RegisterOp : public Operand {
  std::string reg;

public:
  RegisterOp(std::string reg) : reg{reg} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(RegisterOp " << reg << ")\n";
  }

  std::string getReg() const { return reg; }
  void setReg(std::string reg) { this->reg = reg; }

  std::string toString() const override { return reg; }
};

// immediate constant
class ImmediateOp : public Operand {
  int value;

public:
  ImmediateOp(int value) : value{value} {}

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    return os << "(ImmediateOp " << std::to_string(value) << ")\n";
  }

  std::string toString() const override { return std::to_string(value); }
};

} // namespace codegen::assembly
