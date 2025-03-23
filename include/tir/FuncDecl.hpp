#pragma once

#include <memory>
#include <string>

#include "tir/Node.hpp"

namespace tir {

class FuncDecl : public Node {
  std::string name;
  std::shared_ptr<Stmt> body;
  int num_params;

public:
  FuncDecl(std::string name, std::shared_ptr<Stmt> body, int num_params)
      : name{name}, body{body}, num_params{num_params} {}

  std::string getName() const { return name; }
  std::shared_ptr<Stmt> getBody() const { return body; }
  int getNumParams() const { return num_params; }

  void setBody(std::shared_ptr<Stmt> other) { body = other; }

  std::string label() const override { return "FUNC " + name; }
};

} // namespace tir