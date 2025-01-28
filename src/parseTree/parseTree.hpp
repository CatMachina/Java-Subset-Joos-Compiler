#pragma once

#include <array>
#include <iostream>
#include <string>
#include <type_traits>
#include <string_view>
#include <stdexcept>
#include <cassert>
#include <memory>
#include <climits>
#include <magic_enum.hpp>

class myFlexLexer;
class myBisonParser;

namespace parsetree {

struct Node;
class Literal;
class Identifier;
class Operator;
class Modifier;
class BasicType;

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

        ProgramDeclaration,
        PackageDeclaration,

        SingleImportDeclaration,
        MultiImportDeclaration,

        ClassDeclaration,
        FieldDeclaration,
        MethodDeclaration,
        ConstructorDeclaration,
        AbstractMethodDeclaration,
        LocalDeclaration,

        ModifierList,
        InterfaceTypeList,
        ClassBodyDeclarationList,
        InterfaceBodyDeclarationList,
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

         Expression,
         ArrayCreate,
         ArrayAccess,
         ArrayCastType,

         ClassCreation,
         FieldAccess,
         MethodInvocation,

    };

   /// leaf nodes
    Node(Type type) : type{type}, args{nullptr}, num_args{0} {}

   // Non leaf nodes
   template <typename... Args_>
    Node(Type type, Args_&&... args_)
        : type{type},
          args{std::shared_ptr<std::shared_ptr<Node>[]>(
            new std::shared_ptr<Node>[sizeof...(Args_)],
            std::default_delete<std::shared_ptr<Node>[]>() )},
         num_args{sizeof...(Args_)} {
      static_assert(sizeof...(Args_) > 0, "Must have at least one child");
      static_assert((std::is_convertible_v<Args_, std::shared_ptr<Node>> && ...),
                     "All arguments must be convertible to std::shared_ptr<Node>");

      std::shared_ptr<Node> temp_args[] = {std::forward<Args_>(args_)...};
      for (std::size_t i = 0; i < num_args; ++i) {
         args[i] = temp_args[i];
      }
   }

   size_t get_num_children() const { return num_args; }

   // Gets the child at index i of this child
   std::shared_ptr<Node> child_at(size_t i) const { return args[i]; }

   // Gets node type
   Type get_node_type() const { return type; }

   // If the node is corrupted, the tree has been corrupted
   bool is_corrupted() const {
      if (type == Type::Corrupted) return true;
      for (size_t i = 0; i < num_args; ++i) {
         if (args[i] == nullptr) continue;
         if (args[i]->is_corrupted()) return true;
      }
      return false;
   }

   virtual std::ostream& print(std::ostream& os) const {
      os << "(" << magic_enum::enum_name(type);

      for (size_t i = 0; i < num_args; ++i) {
         os << " ";
         if (!args[i]) {
               os << "ε";
         } else {
               args[i]->print(os);
         }
      }
      os << ")";
      return os;
   }

private:
   Type type;
   std::shared_ptr<std::shared_ptr<Node>[]> args;
   size_t num_args;
};

////////////////////////////////////////////////////////////////////////////////

// A node in the parse tree representing a literal value.
class Literal : public Node {
   friend class ::myFlexLexer;
   friend class ::myBisonParser;

public:
   // Enum for literal types
   enum class Type {
      Integer,
      Boolean,
      Character,
      String,
      Null
   };

   // Constructor for Literal
   Literal(Type type, char const* value)
         : Node{Node::Type::Literal},
           type{type},
           isNegative{false},
           value{value} {}

   // Override printing for this leaf node
   std::ostream& print(std::ostream& os) const override {
      os << "Literal(Type: " << magic_enum::enum_name(type) << ", Value: " << value;
      if (isNegative) os << ", Negative: true";
      os << ")";
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
         if (x < -INT_MAX - 1 && isNegative) {
               return false;
         }
      } catch (const std::exception&) {
         return false;
      }
      return true;
   }


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
         : Node{Node::Type::Identifier}, name{std::move(name)} {}

   const char* get_name() const { return name.c_str(); }

   std::ostream& print(std::ostream& os) const override {
      os << "Identifier(Name: " << name << ")";
      return os;
   }

private:
   std::string name;
};

////////////////////////////////////////////////////////////////////////////////

// A node representing  operator.
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
    Operator(Type type) : Node{Node::Type::Operator}, type{type} {}

   std::ostream& print(std::ostream& os) const override {
      return os << magic_enum::enum_name(type);
   }

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
   enum class Type {
      Public,
      Protected,
      Static,
      Abstract,
      Final
   };

   // Constructor for Modifier
    Modifier(Type type) : Node{Node::Type::Modifier}, type{type} {}

   // Get the type of the modifier
   Type get_type() const { return type; }

   // Print the string representation of the modifier
   std::ostream& print(std::ostream& os) const override {
      os << "Modifier(Type: " << magic_enum::enum_name(type) << ")";
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
   enum class Type {
      Byte,
      Short,
      Int,
      Char,
      Boolean
   };

   // Constructor for BasicType
    BasicType(Type type) : Node{Node::Type::BasicType}, type{type} {}

   Type get_type() const { return type; }

   // Print the string representation of the basic type
   std::ostream& print(std::ostream& os) const override {
      os << "BasicType(Type: " << magic_enum::enum_name(type) << ")";
      return os;
   }

private:
   Type type;
};


} // namespace parsetree
