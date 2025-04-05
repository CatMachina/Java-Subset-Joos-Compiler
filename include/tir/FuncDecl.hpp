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

  void buildInsnMapsEnter(InsnMapsBuilder &v) override {
    v.addNameToCurrentIndex(name);
    v.addInsn(shared_from_this());
    return;
  }

  std::shared_ptr<Node> buildInsnMaps(InsnMapsBuilder &v) override {
    return shared_from_this();
  }

  std::vector<std::shared_ptr<Temp>> getParams() { return params; }

  void visitChildren(InsnMapsBuilder &v) override { v.visit(body); }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(FuncDecl " << name << "\n";
    if (body) {
      printIndent(os, indent + 1);
      os << "body: {\n";
      body->print(os, indent + 2);
      printIndent(os, indent + 1);
      os << "}\n";
    }
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;
    children.push_back(body);
    return children;
  }
};

} // namespace tir