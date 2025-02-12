#pragma once

#include "3rd_party/magic_enum.hpp"
#include "sourceNode.hpp"
#include <array>
#include <cassert>
#include <climits>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

class myFlexLexer;
class myBisonParser;

namespace parsetree {

struct Node;
class Literal;
class Identifier;
class Operator;
class Modifier;
class BasicType;
class Corrupted;

// The base node in the parse tree.
struct Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

  // Enum for node types
  enum class Type {
    Type,
    BasicType,
    ArrayType,
    Variable,
    QualifiedName,

    Modifier,
    Operator,
    Identifier,
    Corrupted,
    Literal,

    ProgramDecl,
    PackageDecl,

    ImportDeclList,
    SingleImportDecl,
    MultiImportDecl,

    ClassDecl,
    InterfaceDecl,
    FieldDecl,
    MethodDecl,
    ConstructorDecl,
    AbstractMethodDecl,
    LocalDecl,

    ModifierList,
    InterfaceTypeList,
    ClassBodyDeclList,
    InterfaceBodyDeclList,
    ParameterList,
    StatementList,
    ArgumentList,

    SuperClass,
    Parameter,
    Block,
    Statement,
    ReturnStatement,
    IfStatement,
    WhileStatement,
    ForStatement,
    ExprStatement,
    LocalDeclStatement,

    Expression,
    ArrayCreation,
    ArrayAccess,
    ArrayCastType,

    Assignment,
    ClassCreation,
    FieldAccess,
    MethodInvocation,
    Cast
  };

  /// leaf nodes
  Node(source::SourceRange loc, Type type)
      : loc{loc}, type{type}, args{nullptr}, num_args{0} {}

  // Non leaf nodes
  template <typename... Args_>
  Node(source::SourceRange loc, Type type, Args_ &&...args_)
      : loc{loc}, type{type}, args{std::vector<std::shared_ptr<Node>>{
                                  std::forward<Args_>(args_)...}},
        num_args{sizeof...(Args_)} {
    static_assert(sizeof...(Args_) > 0, "Must have at least one child");
    static_assert((std::is_convertible_v<Args_, std::shared_ptr<Node>> && ...),
                  "All arguments must be convertible to std::shared_ptr<Node>");
  }

  size_t num_children() const { return num_args; }

  // Gets the child at index i of this child
  std::shared_ptr<Node> child_at(size_t i) const { return args[i]; }

  // Gets node type
  Type get_node_type() const { return type; }

  // Return Type String
  std::string type_string() const {
    return std::string(magic_enum::enum_name(type));
  }

  // If the node is corrupted, the tree has been corrupted
  bool is_corrupted() const;

  virtual std::ostream &print(std::ostream &os, int depth = 0) const {
    std::string indent(depth * 2, ' ');
    os << indent << "(" << magic_enum::enum_name(type) << std::endl;

    for (size_t i = 0; i < num_args; ++i) {
      if (!args[i]) {
        os << indent << "ε\n";
      } else {
        args[i]->print(os, depth + 1);
      }
    }
    os << indent << ")\n";
    return os;
  }

  const std::vector<std::shared_ptr<Node>> &children() const { return args; }
  source::SourceRange loc;

private:
  Type type;
  std::vector<std::shared_ptr<Node>> args;
  size_t num_args;
};

class Corrupted : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;
  std::string_view name;

public:
  Corrupted(const char *name) : Node{loc, Node::Type::Corrupted}, name{name} {}
  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    os << indent << "(Corrupted: '" << name << "')\n";
    return os;
  }
};

////////////////////////////////////////////////////////////////////////////////

// A node in the parse tree representing a literal value.
class Literal : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

public:
  // Enum for literal types
  enum class Type { Integer, Boolean, Character, String, Null };

  // Constructor for Literal
  Literal(Type type, char const *value)
      : Node{loc, Node::Type::Literal}, type{type},
        isNegative{false}, value{value} {}

  // Override printing for this leaf node
  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    os << indent << "Literal(Type: " << magic_enum::enum_name(type)
       << ", Value: " << value;
    if (isNegative)
      os << ", Negative: true";
    os << ")\n";
    return os;
  }

  void setNegative() { isNegative = true; }
  bool isValid() const {
    if (type != Type::Integer) {
      return true;
    }
    errno = 0;
    try {
      const auto x = std::stoll(value);

      if (x > INT_MAX && !isNegative) {
        return false;
      }
      if (-x < INT_MIN && isNegative) {
        return false;
      }
    } catch (const std::exception &) {
      return false;
    }
    return true;
  }

  Type getType() const { return type; }
  std::string getValue() const { return value; }

private:
  Type type;
  bool isNegative;
  std::string value;
};

////////////////////////////////////////////////////////////////////////////////

// A node representing an identifier.
class Identifier : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

public:
  Identifier(std::string name)
      : Node{loc, Node::Type::Identifier}, name{std::move(name)} {}

  const char *get_name() const { return name.c_str(); }

  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    os << indent << "Identifier(Name: " << name << ")\n";
    return os;
  }

private:
  std::string name;
};

////////////////////////////////////////////////////////////////////////////////

// A node representing operator.
class Operator : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

public:
  // Enum for operator types
  enum class Type {
    Assign,
    GreaterThan,
    LessThan,
    Not,
    Equal,
    LessThanOrEqual,
    GreaterThanOrEqual,
    NotEqual,
    And,
    Or,
    BitwiseAnd,
    BitwiseOr,
    BitwiseNot,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Plus,
    Minus,
    InstanceOf
  };

  // Constructor for Operator
  Operator(Type type) : Node{loc, Node::Type::Operator}, type{type} {}

  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    return os << indent << "(Type: " << magic_enum::enum_name(type) << ")\n";
  }

  // Getter
  Type getType() const { return type; }

private:
  Type type;
};

////////////////////////////////////////////////////////////////////////////////

// A node representing modifier.
class Modifier : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

public:
  // Enum for modifier types
  enum class Type { Public, Protected, Static, Abstract, Final, Native };

  // Constructor for Modifier
  Modifier(Type type) : Node{loc, Node::Type::Modifier}, type{type} {}

  // Get the type of the modifier
  Type get_type() const { return type; }

  // Print the string representation of the modifier
  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    os << indent << "Modifier(Type: " << magic_enum::enum_name(type) << ")\n";
    return os;
  }

private:
  Type type;
};

////////////////////////////////////////////////////////////////////////////////

// A lex node in the parse tree representing a basic type.
class BasicType : public Node {
  friend class ::myFlexLexer;
  friend class ::myBisonParser;

public:
  // Enum for basic types
  enum class Type { Byte, Short, Int, Char, Boolean };

  // Constructor for BasicType
  BasicType(Type type) : Node{loc, Node::Type::BasicType}, type{type} {}

  Type getType() const { return type; }

  // Print the string representation of the basic type
  std::ostream &print(std::ostream &os, int depth = 0) const override {
    std::string indent(depth * 2, ' ');
    os << indent << "BasicType(Type: " << magic_enum::enum_name(type) << ")\n";
    return os;
  }

private:
  Type type;
};

} // namespace parsetree
