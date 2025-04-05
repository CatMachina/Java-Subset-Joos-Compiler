#pragma once

#include <memory>
#include <string>

#include "tir/FuncDecl.hpp"
#include "tir/Node.hpp"

namespace tir {

class CompUnit : public Node {
  // Question: are they the same?
  std::string name;
  std::string class_name;

  // These point to functions inside of child_functions
  std::unordered_map<std::string, std::shared_ptr<FuncDecl>> functions;
  std::vector<std::shared_ptr<FuncDecl>> child_functions;

  std::unordered_map<std::string, std::shared_ptr<Expr>> static_fields;
  std::vector<std::pair<std::string, std::shared_ptr<Expr>>>
      child_static_fields;

  bool staticFieldsCanonicalized = false;

  std::vector<std::pair<std::string, std::shared_ptr<Stmt>>>
      child_canonical_static_fields;

  std::vector<std::shared_ptr<Node>> nodes;

  std::vector<std::shared_ptr<Stmt>> start_statements;

public:
  CompUnit(std::string name)
      : name(name), nodes(std::vector<std::shared_ptr<Node>>()) {}

  CompUnit(std::string name, std::vector<std::shared_ptr<Node>> nodes)
      : name(name), nodes(nodes) {
    for (auto node : nodes) {
      if (auto funcDeclNode = std::dynamic_pointer_cast<FuncDecl>(node)) {
        appendFunc(funcDeclNode->getName(), funcDeclNode);
      }
    }
  }

  // Getters
  std::shared_ptr<FuncDecl> getFunction(std::string name) {
    if (!functions.count(name)) {
      throw std::runtime_error("Could not find function with name " + name +
                               " in the IR.");
    }
    return functions[name];
  }

  std::string getName() const { return name; }

  std::unordered_map<std::string, std::shared_ptr<FuncDecl>>
  getFunctions() const {
    return functions;
  }
  std::unordered_map<std::string, std::shared_ptr<Expr>> getFields() const {
    return static_fields;
  }
  std::string label() const override { return "COMPUNIT (" + class_name + ")"; }

  std::vector<std::shared_ptr<FuncDecl>> &getFunctionList() {
    return child_functions;
  }

  std::vector<std::pair<std::string, std::shared_ptr<Expr>>>
  getFieldList() const {
    if (staticFieldsCanonicalized)
      throw std::runtime_error(
          "Static field initalizers are canonicalized; use getCanonFieldList");
    return child_static_fields;
  }

  std::vector<std::pair<std::string, std::shared_ptr<Stmt>>>
  getCanonFieldList() const {
    if (!staticFieldsCanonicalized)
      throw std::runtime_error(
          "Static field initalizers are not canonicalized; use getFieldList");
    return child_canonical_static_fields;
  }

  std::vector<std::shared_ptr<Stmt>> getStartStmts() const {
    return start_statements;
  }

  bool areStaticFieldsCanonicalized() const {
    return staticFieldsCanonicalized;
  }

  // Setters
  void setClassName(std::string name) { class_name = name; }

  void appendFunc(std::string name, std::shared_ptr<FuncDecl> func) {
    child_functions.push_back(func);
    functions[name] = child_functions.back();
  }

  void appendField(std::string name, std::shared_ptr<Expr> value) {
    child_static_fields.push_back(std::make_pair(name, value));
    static_fields[name] = child_static_fields.back().second;
  }

  void appendStartStmt(std::shared_ptr<Stmt> stmt) {
    start_statements.push_back(stmt);
  }

  void canonicalizeStaticFields(
      std::vector<std::pair<std::string, std::shared_ptr<Stmt>>>
          &child_canonical_static_fields) {
    this->child_canonical_static_fields = child_canonical_static_fields;
    staticFieldsCanonicalized = true;
  }

  void visitChildren(InsnMapsBuilder &v) override {
    for (auto &[name, func] : functions) {
      v.visit(func);
    }
  }

  std::ostream &print(std::ostream &os, int indent = 0) const override {
    printIndent(os, indent);
    os << "(CompUnit " << name << "\n";
    printIndent(os, indent + 1);
    os << "nodes: [\n";
    for (const auto node : nodes) {
      node->print(os, indent + 2);
    }
    printIndent(os, indent + 1);
    os << "]\n";
    printIndent(os, indent);
    os << ")\n";
    return os;
  }

  std::vector<std::shared_ptr<Node>> getChildren() const override {
    std::vector<std::shared_ptr<Node>> children;

    for (auto pair : getFunctions()) {
      children.push_back(pair.second);
    }

    if (areStaticFieldsCanonicalized()) {
      for (auto pair : getCanonFieldList()) {
        children.push_back(pair.second);
      }
    } else {
      for (auto pair : getFieldList()) {
        children.push_back(pair.second);
      }
    }

    for (auto stmt : getStartStmts()) {
      children.push_back(stmt);
    }

    return children;
  }
};

} // namespace tir
