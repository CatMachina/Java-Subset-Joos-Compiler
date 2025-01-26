#pragma once

#include <array>
#include <iostream>
#include <memory_resource>
#include <string>
#include <type_traits>
#include <string_view>
#include <stdexcept>
#include <cassert>

#include <array>
#include <string_view>
#include <stdexcept>
#include <cassert>

class FlexLexer;
class BisonParser;

namespace parsetree {

struct Node;
class Literal;
class Identifier;
class Operator;
class Modifier;
class BasicType;

// For efficiently allocate memory for the nodes in the parse tree
using BumpPtrAllocator = std::pmr::polymorphic_allocator<std::byte>;

/// @brief The basic type-tagged node in the parse tree.
struct Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

   /// @brief Enum for node types
   enum class Type {
        // Leaf nodes
        Literal,
        Identifier,
        QualifiedName, // Updated from QualifiedIdentifier to align with grammar
        Operator,
        BasicType, // Covers types like INT, CHAR, etc.
        ArrayType,
        Type, // Represents type -> BOOLEAN | INT | ...
        Poison,

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

        LAST_MEMBER // Marker for bounds checking
    };


   /// @brief String representations of the enum values
   static constexpr std::array<std::string_view, static_cast<size_t>(Type::LAST_MEMBER)> type_strings{
        "Literal", "Identifier", "QualifiedName", "Operator", "BasicType",
        "ArrayType", "Type", "Poison", "Program", "PackageDeclaration",
        "ImportDeclarations", "ImportDeclaration", "TypeDeclaration",
        "ClassDeclaration", "InterfaceDeclaration", "ModifierList",
        "PublicModifier", "ProtectedModifier", "StaticModifier",
        "AbstractModifier", "FinalModifier", "ClassBody",
        "ClassBodyDeclaration", "FieldDeclaration", "MethodDeclaration",
        "ConstructorDeclaration", "SuperclassOpt", "InterfacesOpt",
        "InterfaceList", "InterfaceBody", "InterfaceMemberDeclarationList",
        "MethodHeader", "FormalParameterList", "FormalParameter",
        "VoidType", "Statement", "Block", "IfStatement",
        "IfElseStatement", "WhileStatement", "ForStatement",
        "ReturnStatement", "EmptyStatement", "ExpressionStatement",
        "VariableDeclarator", "LocalVariableDeclaration",
        "VariableDeclaratorList", "FieldInitializer", "Expression",
        "ArgumentList", "FieldAccess", "ArrayAccess", "CastExpression",
        "MethodInvocation", "ArrayCreationExpression",
        "ClassInstanceCreationExpression", "Dims", "NumLiteral",
        "CharLiteral", "StringLiteral", "BooleanLiteral", "NullLiteral",
        "TestExpression"
    };


   /// @brief Convert Type to string
   static std::string_view to_string(Type type) {
      size_t index = static_cast<size_t>(type);
      if (index >= type_strings.size()) {
         throw std::out_of_range("Invalid enum value for Type");
      }
      return type_strings[index];
   }

   /// @brief Protected constructor for leaf nodes
   explicit Node(Type type) : type{type}, args{nullptr}, num_args{0} {}

   /// @brief Protected constructor for non-leaf nodes
   template <typename... Args>
   Node(BumpPtrAllocator& alloc, Type type, Args&&... args)
         : type{type},
           args{static_cast<Node**>(alloc.allocate_bytes(
                 sizeof...(Args) * sizeof(Node*), alignof(Node*)))} {
      static_assert(sizeof...(Args) > 0, "Must have at least one child");
      static_assert((std::is_convertible_v<Args, Node*> && ...),
                    "All arguments must be convertible to Node*");
      num_args = sizeof...(Args);
      std::array<Node*, sizeof...(Args)> tmp{std::forward<Args>(args)...};
      for (size_t i = 0; i < sizeof...(Args); ++i) {
         this->args[i] = tmp[i];
      }
   }

   /// @brief Gets the number of children
   size_t num_children() const { return num_args; }

   /// @brief Gets the child at index i
   Node* child(size_t i) const { return args[i]; }

   /// @brief Gets the type of the node
   Type get_node_type() const { return type; }

   /// @brief Get string representation of the node's type
   std::string type_string() const { return std::string(to_string(type)); }

   /// @brief Check if the tree has been poisoned
   bool is_poisoned() const {
      if (type == Type::Poison) return true;
      for (size_t i = 0; i < num_args; ++i) {
         if (args[i] == nullptr) continue;
         if (args[i]->is_poisoned()) return true;
      }
      return false;
   }

   /// @brief Virtual function to print the node
   virtual std::ostream& print(std::ostream& os) const;

   /// @brief Print the node as a dot file
   std::ostream& printDot(std::ostream& os) const;

private:
   Type type;
   Node** args;
   size_t num_args;

   void printType(std::ostream& os) const;
   void printTypeAndValue(std::ostream& os) const;
   int printDotRecursive(std::ostream& os, const Node& node, int& id_counter) const;
};


// Output stream operator for a parse tree node
std::ostream& operator<<(std::ostream& os, Node const& n);

////////////////////////////////////////////////////////////////////////////////

/// @brief A lex node in the parse tree representing a literal value.
class Literal : public Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

public:
   /// @brief Enum for literal types
   enum class Type {
      Integer,
      Character,
      String,
      Boolean,
      Null,
      LAST_MEMBER // Marker for bounds checking
   };

   /// @brief String representations of the literal types
   static constexpr std::array<std::string_view, static_cast<size_t>(Type::LAST_MEMBER)> literal_strings{
      "Integer", "Character", "String", "Boolean", "Null"
   };

   /// @brief Convert Type to string
   static std::string_view to_string(Type type) {
      size_t index = static_cast<size_t>(type);
      if (index >= literal_strings.size()) {
         throw std::out_of_range("Invalid enum value for Literal::Type");
      }
      return literal_strings[index];
   }

private:
   /// @brief Constructor for Literal
   Literal(BumpPtrAllocator const& alloc, Type type, char const* value)
         : Node{Node::Type::Literal},
           type{type},
           isNegative{false},
           value{value, alloc} {}

public:
   // Override printing for this leaf node
   std::ostream& print(std::ostream& os) const override {
      os << "Literal(Type: " << to_string(type) << ", Value: " << value;
      if (isNegative) os << ", Negative: true";
      os << ")";
      return os;
   }

   // Set the value of the literal to negative
   void setNegative() { isNegative = true; }

   // Check if the literal is valid
   bool isValid() const {
      // Implement validation logic if needed
      return !value.empty();
   }

private:
   Type type;
   bool isNegative;
   std::pmr::string value;
};

////////////////////////////////////////////////////////////////////////////////

/// @brief A lex node in the parse tree representing an identifier.
class Identifier : public Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

private:
   /// @brief Constructor for Identifier
   Identifier(BumpPtrAllocator const& alloc, char const* name)
         : Node{Node::Type::Identifier}, name{name, alloc} {}

public:
   // Get the name of the identifier
   const char* get_name() const { return name.c_str(); }

   // Override printing for this leaf node
   std::ostream& print(std::ostream& os) const override {
      os << "Identifier(Name: " << name << ")";
      return os;
   }

private:
   std::pmr::string name;
};

////////////////////////////////////////////////////////////////////////////////

/// @brief A lex node in the parse tree representing an operator.
class Operator : public Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

public:
   /// @brief Enum for operator types
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
      BitwiseXor,
      BitwiseNot,
      Add,
      Subtract,
      Multiply,
      Divide,
      Modulo,
      Plus,
      Minus,
      InstanceOf,
      LAST_MEMBER // Marker for bounds checking
   };

   /// @brief String representations of the operator types
   static constexpr std::array<std::string_view, static_cast<size_t>(Type::LAST_MEMBER)> operator_strings{
      "Assign", "GreaterThan", "LessThan", "Not", "Equal", 
      "LessThanOrEqual", "GreaterThanOrEqual", "NotEqual", "And", "Or", 
      "BitwiseAnd", "BitwiseOr", "BitwiseXor", "BitwiseNot", 
      "Add", "Subtract", "Multiply", "Divide", "Modulo", 
      "Plus", "Minus", "InstanceOf"
   };

   /// @brief Convert Type to string
   static std::string_view to_string(Type type) {
      size_t index = static_cast<size_t>(type);
      if (index >= operator_strings.size()) {
         throw std::out_of_range("Invalid enum value for Operator::Type");
      }
      return operator_strings[index];
   }

private:
   /// @brief Constructor for Operator
   explicit Operator(Type type) : Node{Node::Type::Operator}, type{type} {}

public:
   // Override printing for this leaf node
   std::ostream& print(std::ostream& os) const override {
      return os << to_string();
   }

   // Get the string representation of the operator
   std::string to_string() const {
      return std::string(to_string(type));
   }

private:
   Type type;
};

////////////////////////////////////////////////////////////////////////////////

/// @brief A lex node in the parse tree representing a modifier.
class Modifier : public Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

public:
   /// @brief Enum for modifier types
   enum class Type {
      Public,
      Protected,
      Static,
      Abstract,
      Final,
      Native,
      LAST_MEMBER // Marker for bounds checking
   };

   /// @brief String representations of the modifier types
   static constexpr std::array<std::string_view, static_cast<size_t>(Type::LAST_MEMBER)> modifier_strings{
      "Public", "Protected", "Static", "Abstract", "Final", "Native"
   };

   /// @brief Convert Type to string
   static std::string_view to_string(Type type) {
      size_t index = static_cast<size_t>(type);
      if (index >= modifier_strings.size()) {
         throw std::out_of_range("Invalid enum value for Modifier::Type");
      }
      return modifier_strings[index];
   }

private:
   /// @brief Constructor for Modifier
   explicit Modifier(Type type) : Node{Node::Type::Modifier}, modty{type} {}

public:
   // Get the type of the modifier
   Type get_type() const { return modty; }

   // Print the string representation of the modifier
   std::ostream& print(std::ostream& os) const override {
      os << "Modifier(Type: " << to_string(modty) << ")";
      return os;
   }

private:
   Type modty;
};

////////////////////////////////////////////////////////////////////////////////

/// @brief A lex node in the parse tree representing a basic type.
class BasicType : public Node {
   friend class ::FlexLexer;
   friend class ::BisonParser;

public:
   /// @brief Enum for basic types
   enum class Type {
      Byte,
      Short,
      Int,
      Char,
      Boolean,
      LAST_MEMBER // Marker for bounds checking
   };

   /// @brief String representations of the basic types
   static constexpr std::array<std::string_view, static_cast<size_t>(Type::LAST_MEMBER)> basic_type_strings{
      "Byte", "Short", "Int", "Char", "Boolean"
   };

   /// @brief Convert Type to string
   static std::string_view to_string(Type type) {
      size_t index = static_cast<size_t>(type);
      if (index >= basic_type_strings.size()) {
         throw std::out_of_range("Invalid enum value for BasicType::Type");
      }
      return basic_type_strings[index];
   }

private:
   /// @brief Constructor for BasicType
   explicit BasicType(Type type) : Node{Node::Type::BasicType}, type{type} {}

public:
   // Get the type of the basic type
   Type get_type() const { return type; }

   // Print the string representation of the basic type
   std::ostream& print(std::ostream& os) const override {
      os << "BasicType(Type: " << to_string(type) << ")";
      return os;
   }

private:
   Type type;
};


} // namespace parsetree
