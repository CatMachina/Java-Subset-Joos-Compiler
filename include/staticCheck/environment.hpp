#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ast/ast.hpp"

namespace static_check {

class Class;
class Interface;
class Method;
class Field;
class Variable;

// trie tree structure
class Package {

  std::string name;

public:
  using packageChild = std::variant<std::shared_ptr<Package>,
                                    std::shared_ptr<Decl>, std::nullptr_t>;
  // children could be either package or decl
  // public for now, easier to code
  std::unordered_map<std::string, packageChild> children;

  explicit Package() {} // for root
  explicit Package(std::string packageName) : name(packageName) {}

  bool hasChild(std::string childName) {
    return children.find(std::string(childName)) != children.end();
  }

  std::shared_ptr<Package> addPackage(std::string childName) {
    if (!hasChild(childName)) {
      children[std::string(childName)] = std::make_shared<Package>(childName);
    }
    if (std::holds_alternative<std::shared_ptr<Decl>>(
            children[std::string(childName)])) {
      throw std::runtime_error("Package already exists as Decl with name: " +
                               std::string(childName));
    }
    return std::get<std::shared_ptr<Package>>(children[std::string(childName)]);
  }

  packageChild getChild(std::string childName) {
    auto it = children.find(std::string(childName));
    return (it != children.end()) ? it->second : nullptr;
  }

  void printStructure(int depth = 0) const {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";
    std::cout << name << "\n";
    for (const auto &[name, child] : children) {
      if (std::holds_alternative<std::shared_ptr<Package>>(child)) {
        std::get<std::shared_ptr<Package>>(child)->printStructure(depth + 1);
      } else if (std::holds_alternative<std::shared_ptr<Decl>>(child)) {
        std::get<std::shared_ptr<Decl>>(child)->printDecl(depth + 1);
      }
    }
  }
};

class Body : public Decl {
public:
  explicit Body(std::shared_ptr<parsetree::ast::Decl> body) : Decl{body} {}
  void printDecl(int depth = 0) const {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";
    std::cout << "(Body: " << getAstNode()->getName() << ")"
              << "\n";
  }
};

class Class : public Decl {
public:
  explicit Class(std::shared_ptr<parsetree::ast::ClassDecl> cls) : Decl{cls} {}
  void printDecl(int depth = 0) const {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";
    std::cout << "(Class: " << getAstNode()->getName() << ")"
              << "\n";
  }
};

} // namespace static_check