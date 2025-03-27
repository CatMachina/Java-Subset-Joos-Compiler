#pragma once

#include <memory>
#include <string>

#include "tir/FuncDecl.hpp"
#include "tir/Node.hpp"

namespace tir {

class CompUnit : public Node {
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

public:
  CompUnit(std::string name, std::vector<std::shared_ptr<Node>> nodes)
      : name(name), nodes(nodes) {}

  std::vector<std::shared_ptr<Stmt>> start_statements;

  // Getters
  std::shared_ptr<FuncDecl> getFunc(std::string name) {
    if (functions.find(name) == functions.end()) {
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

  std::vector<std::shared_ptr<FuncDecl>> getFunctionList() const {
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

  // TODO: implement
  std::ostream &print(std::ostream &os, int indent = 0) const override {
    os << "(CompUnit)\n";
    return os;
  }
};

} // namespace tir
