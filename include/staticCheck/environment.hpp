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

class Decl {
  std::shared_ptr<parsetree::ast::Decl> astNode;

public:
  explicit Decl(std::shared_ptr<parsetree::ast::Decl> node)
      : astNode(std::move(node)) {}
  virtual void printDecl(int depth = 0) const = 0;
  std::string getName() const { return astNode->getName(); }
  std::shared_ptr<parsetree::ast::Decl> getAstNode() const { return astNode; }
};

// trie tree structure
class Package {

  std::string_view name;

public:
  using packageChild = std::variant<std::shared_ptr<Package>,
                                    std::shared_ptr<Decl>, std::nullptr_t>;
  // children could be either package or decl
  // public for now, easier to code
  std::unordered_map<std::string, packageChild> children;

  explicit Package() {} // for root
  explicit Package(std::string_view packageName) : name(packageName) {}

  bool hasChild(std::string_view childName) {
    return children.find(std::string(childName)) != children.end();
  }

  std::shared_ptr<Package> addPackage(std::string_view childName) {
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

  packageChild getChild(std::string_view childName) {
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
  void printDecl(int depth = 0) const override {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";
    std::cout << "(Body: " << getAstNode()->getName() << ")"
              << "\n";
  }
};

// class Class : public Decl {
//   std::string name;
//   std::unordered_set<std::shared_ptr<Method>> methods;
//   std::unordered_set<std::shared_ptr<Field>> fields;
//   std::unordered_set<std::shared_ptr<Class>> superclasses;
//   std::unordered_set<std::shared_ptr<Interface>> extendedInterfaces;
//   std::unordered_set<std::string> modifiers;

// public:
//   explicit Class(std::string name) : name{name} {}

//   void addMethod(std::shared_ptr<Method> method) { methods.insert(method); }
//   void addField(std::shared_ptr<Field> field) { fields.insert(field); }
//   void addSuperclass(std::shared_ptr<Class> superclass) {
//     superclasses.insert(superclass);
//   }
//   void addExtendedInterface(std::shared_ptr<Interface> interface) {
//     extendedInterfaces.insert(interface);
//   }
//   void addModifier(const std::string &modifier) { modifiers.insert(modifier);
//   }

//   const std::string &getName() const { return name; }
// };

// class Interface : public Decl {
//   std::string name;
//   std::unordered_set<std::shared_ptr<Method>> methods;
//   std::unordered_set<std::shared_ptr<Interface>> extendedInterfaces;
//   std::unordered_set<std::string> modifiers;

// public:
//   explicit Interface(std::string name) : name{name} {}

//   void addMethod(std::shared_ptr<Method> method) { methods.insert(method); }
//   void addExtendedInterface(std::shared_ptr<Interface> interface) {
//     extendedInterfaces.insert(interface);
//   }
//   void addModifier(const std::string &modifier) { modifiers.insert(modifier);
//   }

//   const std::string &getName() const { return name; }
// };

// class Method : public Decl {
//   std::string name;
//   std::unordered_set<std::shared_ptr<Variable>> localVars;
//   std::unordered_set<std::string> modifiers;

// public:
//   explicit Method(std::string name) : name{name} {}

//   void addLocalVar(std::shared_ptr<Variable> var) { localVars.insert(var); }
//   void addModifier(const std::string &modifier) { modifiers.insert(modifier);
//   }

//   const std::string &getName() const { return name; }
// };

// class Field : public Decl {
// private:
//   std::string name;
//   std::unordered_set<std::string> modifiers;

// public:
//   explicit Field(std::string name) : name{name} {}

//   void addModifier(const std::string &modifier) { modifiers.insert(modifier);
//   } const std::string &getName() const { return name; }
// };

// class Variable : public Decl {
//   std::string name;

// public:
//   explicit Variable(std::string name) : name{name} {}
//   const std::string &getName() const { return name; }
// };

// class Environment {
//   // Hashmap to map names to Decls
//   std::unordered_map<std::string, std::shared_ptr<Decl>> simpleNamesToDecls;

//   // Reference to the scope (AST node) which the environment is for
//   std::shared_ptr<parsetree::ast::AstNode> scope;

//   // Reference to the outer enclosing environment?
//   // Do we need this, given we maintain a stack?
// };

// class GlobalEnvironment {
//   // Maybe this is the "global symbol table". According to some previous
//   course
//   // notes, "The global environment should record all class names along with
//   // their corresponding package names from the files passed to the compiler
//   for
//   // linking". So it will just look like this for now?
// public:
//   GlobalEnvironment();

//   void addDecl(const std::string &qualifiedName, std::shared_ptr<Decl> decl);
//   [[nodiscard]] std::shared_ptr<Decl>
//   getDecl(const std::string &qualifiedName) const;

// private:
//   // From fully qualified names to declarations
//   std::unordered_map<std::string, std::shared_ptr<Decl>>
//   qualifiedNamesToDecls;
// };

} // namespace static_check