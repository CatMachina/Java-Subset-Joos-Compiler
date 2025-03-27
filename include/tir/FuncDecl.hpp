#pragma once

#include <memory>
#include <string>

#include "tir/Node.hpp"

namespace tir {

class FuncDecl : public Node {
  std::string name;
  std::shared_ptr<Stmt> body;
  int num_params;
  std::vector<std::shared_ptr<Temp>> params;

public:
  FuncDecl(std::string name, std::shared_ptr<Stmt> body, int num_params)
      : name{name}, body{body}, num_params{num_params} {}

  FuncDecl(std::string name, std::vector<std::shared_ptr<Temp>> params,
           std::shared_ptr<Stmt> body)
      : name{name}, params{params}, body{body} {}

  std::string getName() const { return name; }

  // mutable
  std::shared_ptr<Stmt> &getBody() { return body; }
  int getNumParams() const { return num_params; }

  void setBody(std::shared_ptr<Stmt> other) { body = other; }

  std::string label() const override { return "FUNC " + name; }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(FuncDecl " << name << "\n";
    printIndent(os, indent + 1);
    os << "body: { \n";
    body->print(os, indent + 2);
    printIndent(os, indent + 1);
    os << "}\n";
    printIndent(os, indent);
    os << ")\n";
    return os;
  }
};

} // namespace tir