#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast/ast.hpp"

namespace static_check {

class Class;
class Interface;
class Method;
class Field;
class Variable;

class Decl {};

class Package : public Decl {
  std::string name;
  std::unordered_set<std::shared_ptr<Class>> classes;
  std::unordered_set<std::shared_ptr<Interface>> interfaces;

public:
  explicit Package(std::string packageName) : name{packageName} {}

  void addClass(std::shared_ptr<Class> classPtr) { classes.insert(classPtr); }
  void addInterface(std::shared_ptr<Interface> interface) {
    interfaces.insert(interface);
  }

  const std::string &getName() const { return name; }
  const std::unordered_set<std::shared_ptr<Class>> &getClasses() const {
    return classes;
  }
  const std::unordered_set<std::shared_ptr<Interface>> &getInterfaces() const {
    return interfaces;
  }
};

class Class : public Decl {
  std::string name;
  std::unordered_set<std::shared_ptr<Method>> methods;
  std::unordered_set<std::shared_ptr<Field>> fields;
  std::unordered_set<std::shared_ptr<Class>> superclasses;
  std::unordered_set<std::shared_ptr<Interface>> extendedInterfaces;
  std::unordered_set<std::string> modifiers;

public:
  explicit Class(std::string name) : name{name} {}

  void addMethod(std::shared_ptr<Method> method) { methods.insert(method); }
  void addField(std::shared_ptr<Field> field) { fields.insert(field); }
  void addSuperclass(std::shared_ptr<Class> superclass) {
    superclasses.insert(superclass);
  }
  void addExtendedInterface(std::shared_ptr<Interface> interface) {
    extendedInterfaces.insert(interface);
  }
  void addModifier(const std::string &modifier) { modifiers.insert(modifier); }

  const std::string &getName() const { return name; }
};

class Interface : public Decl {
  std::string name;
  std::unordered_set<std::shared_ptr<Method>> methods;
  std::unordered_set<std::shared_ptr<Interface>> extendedInterfaces;
  std::unordered_set<std::string> modifiers;

public:
  explicit Interface(std::string name) : name{name} {}

  void addMethod(std::shared_ptr<Method> method) { methods.insert(method); }
  void addExtendedInterface(std::shared_ptr<Interface> interface) {
    extendedInterfaces.insert(interface);
  }
  void addModifier(const std::string &modifier) { modifiers.insert(modifier); }

  const std::string &getName() const { return name; }
};

class Method : public Decl {
  std::string name;
  std::unordered_set<std::shared_ptr<Variable>> localVars;
  std::unordered_set<std::string> modifiers;

public:
  explicit Method(std::string name) : name{name} {}

  void addLocalVar(std::shared_ptr<Variable> var) { localVars.insert(var); }
  void addModifier(const std::string &modifier) { modifiers.insert(modifier); }

  const std::string &getName() const { return name; }
};

class Field : public Decl {
private:
  std::string name;
  std::unordered_set<std::string> modifiers;

public:
  explicit Field(std::string name) : name{name} {}

  void addModifier(const std::string &modifier) { modifiers.insert(modifier); }
  const std::string &getName() const { return name; }
};

class Variable : public Decl {
  std::string name;

public:
  explicit Variable(std::string name) : name{name} {}
  const std::string &getName() const { return name; }
};

// Maybe this is for the stack of scopes
class Environment {
  std::unordered_map<std::string, std::shared_ptr<Decl>> simpleNamesToDecls;
};

// Maybe this is the "global symbol table". According to some previous course notes,
// "The global environment should record all class names along with their 
// corresponding package names from the files passed to the compiler for linking".
// So it will just look like this for now? 
public:
  GlobalEnvironment();

  void registerDecl(const std::string &name, std::shared_ptr<Decl> decl);

  [[nodiscard]] std::shared_ptr<Decl> getDecl(const std::string &name) const;

  void registerTypeToPackage(const std::string &typeName,
                             std::string &packageName);

  [[nodiscard]] std::string getPackageName(const std::string &typeName) const;

private:
  // From fully qualified names to declarations
  std::unordered_map<std::string, std::shared_ptr<Decl>> qualifiedNamesToDecls;

  // From class/interface names to package names
  std::unordered_map<std::string, std::string> typeNamesToPackageNames;
};

} // namespace static_check