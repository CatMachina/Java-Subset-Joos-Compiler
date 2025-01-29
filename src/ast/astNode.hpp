#pragma once

#include "parsetree/ParseTree.h"
#include <iostream>
#include <string>
#include <vector>

namespace ast {

// Base class for all AST nodes //////////////////////////////////////////////

class AstNode {
public:
  virtual ~AstNode() = default;
  virtual std::ostream &print(std::ostream &os) const = 0;
};

class Decl : public AstNode {
  std::string name;

public:
  explicit Decl(std::string_view name) : name{name} {}
  [[nodiscard]] std::string getName() const noexcept { return name; }
};

class ProgramDecl : public AstNode {};

class Type : public AstNode {
public:
  ~Type() override = default;
  [[nodiscard]] virtual std::string toString() const = 0;

  std::ostream &print(std::ostream &os) const override {
    return os << toString();
  }
};

class Stmt : public AstNode {};

std::ostream &operator<<(std::ostream &os, const AstNode &astNode);

// Other classes /////////////////////////////////////////////////////////////

class Modifiers {
  bool isPublic_ = false;
  bool isProtected_ = false;
  bool isStatic_ = false;
  bool isFinal_ = false;
  bool isAbstract_ = false;
  bool isNative_ = false;

public:
  void set(parsetree::Modifier modifier);
  void set(ast::Modifiers modifier);

  [[nodiscard]] bool isPublic() const noexcept { return isPublic_; }
  [[nodiscard]] bool isProtected() const noexcept { return isProtected_; }
  [[nodiscard]] bool isStatic() const noexcept { return isStatic_; }
  [[nodiscard]] bool isFinal() const noexcept { return isFinal_; }
  [[nodiscard]] bool isAbstract() const noexcept { return isAbstract_; }
  [[nodiscard]] bool isNative() const noexcept { return isNative_; }

  void setPublic();
  void setProtected();
  void setStatic();
  void setFinal();
  void setAbstract();
  void setNative();

  [[nodiscard]] std::string toString() const;
};

class QualifiedIdentifier {
  std::vector<std::string> identifiers;

public:
  void addIdentifier(std::string_view identifier) {
    identifiers.emplace_back(identifier);
  }

  [[nodiscard]] std::string toString() const {
    if (identifiers.empty())
      return "";

    std::string result;
    for (const auto &identifier : identifiers) {
      result += identifier + '.';
    }
    result.pop_back();
    return result;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const QualifiedIdentifier &qid) {
    return os << qid.toString();
  }
};

} // namespace ast
