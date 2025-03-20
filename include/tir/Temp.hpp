#pragma once

#include <memory>
#include <string>

#include "tir/Expr.hpp"

namespace tir {

class Temp : public Expr {
  static size_t num_temps;
  std::string name;

public:
  bool isGlobal = false; // Is a static field

  Temp(std::string name, bool isGlobal = false)
      : name{name}, isGlobal{isGlobal} {}

  std::string &getName() { return name; }
  static std::string generateName(std::string prefix = "") {
    num_temps++;
    return (prefix.empty() ? "temp" : prefix) + std::to_string(num_temps);
  }

  std::string label() { return "TEMP(" + name + ")"; }

  static std::shared_ptr<Expr> makeExpr(std::string str,
                                        bool isGlobal = false) {
    return std::make_shared<Temp>(str, isGlobal);
  }
};

} // namespace tir
