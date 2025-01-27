#pragma once

#include <array>
#include <iostream>
#include <string>
#include <type_traits>
#include <string_view>
#include <stdexcept>
#include <cassert>

#include <array>
#include <string_view>
#include <stdexcept>
#include <cassert>

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
        // Leaf nodes
        Literal,
        Identifier,
        QualifiedName, // Updated from QualifiedIdentifier to align with grammar
        Operator,
        BasicType, // Covers types like INT, CHAR, etc.
        ArrayType,
        Type, // Represents type -> BOOLEAN | INT | ...
        Corrupted,

        // Program structure
        Program,
        PackageDeclaration,
        ImportDeclarations,
        ImportDeclaration,

        // Type Declarations
        TypeDeclaration,
        ClassDeclaration,
        InterfaceDeclaration,

        // Modifiers
        ModifierList,
        PublicModifier, // PUBLIC
        ProtectedModifier, // PROTECTED
        StaticModifier, // STATIC
        AbstractModifier, // ABSTRACT
        FinalModifier, // FINAL

        // Class-specific constructs
        ClassBody,
        ClassBodyDeclaration,
        FieldDeclaration,
        MethodDeclaration,
        ConstructorDeclaration,
        SuperclassOpt,
        InterfacesOpt,
        InterfaceList,

        // Interface-specific constructs
        InterfaceBody,
        InterfaceMemberDeclarationList,

        // Method-specific constructs
        MethodHeader,
        FormalParameterList,
        FormalParameter,
        VoidType,

        // Statements
        Statement,
        Block,
        IfStatement,
        IfElseStatement,
        WhileStatement,
        ForStatement,
        ReturnStatement,
        EmptyStatement, // SEMI
        ExpressionStatement,

        // Variable Declarations
        VariableDeclarator,
        LocalVariableDeclaration,
        VariableDeclaratorList,
        FieldInitializer,

        // Expressions
        Expression,
        ArgumentList,
        FieldAccess,
        ArrayAccess,
        CastExpression,
        MethodInvocation,
        ArrayCreationExpression,
        ClassInstanceCreationExpression,
        Dims,

        // Literals
        NumLiteral, // NUM
        CharLiteral, // SQUOTE char SQUOTE
        StringLiteral, // DQUOTE chars DQUOTE
        BooleanLiteral, // TRUE, FALSE
        NullLiteral, // NULL

        // Test (used in control flows like IF, WHILE)
        TestExpression, // For comparison expressions like <, >, ==, !=, etc.
    };

   /// leaf nodes
   explicit Node(Type type) : type{type}, args{nullptr}, num_args{0} {}

   // Non leaf nodes
   template <typename... Args_>
    Node(Type type, Args_&&... args_)
        : type{type},
          args{std::make_unique<Node*[]>(sizeof...(Args_))},
          num_args{sizeof...(Args_)} {
        static_assert(sizeof...(Args_) > 0, "Must have at least one child");
        static_assert((std::is_convertible_v<Args_, Node*> && ...),
                      "All arguments must be convertible to Node*");

        Node* temp_args[] = {std::forward<Args_>(args_)...};
        for (std::size_t i = 0; i < num_args; ++i) {
            args[i] = temp_args[i];
        }
    }

   size_t get_num_children() const { return num_args; }

   // Gets the child at index i of this child
   Node* child_at(size_t i) const { return args[i]; }

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

   virtual std::ostream& print(std::ostream& os) const;

private:
   Type type;
   Node** args;
   size_t num_args;
};

// Output stream operator for a parse tree node
std::ostream& operator<<(std::ostream& os, Node const& n);

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

private:
   // Constructor for Literal
   Literal(Type type, char const* value)
         : Node{Node::Type::Literal},
           type{type},
           isNegative{false},
           value{value, alloc} {}

public:
   // Override printing for this leaf node
   std::ostream& print(std::ostream& os) const override {
      os << "Literal(Type: " << type << ", Value: " << value;
      if (isNegative) os << ", Negative: true";
      os << ")";
      return os;
   }

   void setNegative() { isNegative = true; }
   bool isValid() const;

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

private:
   Identifier(char const* name)
         : Node{Node::Type::Identifier}, name{name} {}

public:
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

private:
   // Constructor for Operator
   explicit Operator(Type type) : Node{Node::Type::Operator}, type{type} {}

public:
   std::ostream& print(std::ostream& os) const override {
      return os << type;
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

private:
   // Constructor for Modifier
   explicit Modifier(Type type) : Node{Node::Type::Modifier}, type{type} {}

public:
   // Get the type of the modifier
   Type get_type() const { return type; }

   // Print the string representation of the modifier
   std::ostream& print(std::ostream& os) const override {
      os << "Modifier(Type: " << type << ")";
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

private:
   // Constructor for BasicType
   explicit BasicType(Type type) : Node{Node::Type::BasicType}, type{type} {}

public:
   Type get_type() const { return type; }

   // Print the string representation of the basic type
   std::ostream& print(std::ostream& os) const override {
      os << "BasicType(Type: " << type << ")";
      return os;
   }

private:
   Type type;
};


} // namespace parsetree
