#include "ast/ast.hpp"

namespace parsetree::ast {

void Decl::setParent(std::shared_ptr<CodeBody> rawParent) {
  if (rawParent == nullptr)
    throw std::runtime_error("parent cannot be null!");
  // if (!this->parent.expired())
  //   throw std::runtime_error("parent already set!");
  // // std::shared_ptr<CodeBody> parentShared = rawParent->weak_from_this();
  // this->parent =
  //     rawParent
  //         ->weak_from_this(); // Store as weak_ptr to avoid ownership issues
  this->parent = rawParent;
}

ProgramDecl::ProgramDecl(std::shared_ptr<ReferenceType> package,
                         std::vector<std::shared_ptr<ImportDecl>> imports,
                         std::shared_ptr<CodeBody> body)
    : package{package}, imports{imports}, body{body} {
  std::unordered_map<std::string, std::string> existingImports;

  auto decl = std::dynamic_pointer_cast<Decl>(body);
  if (decl) {
    // decl->setParent(this);
  } else {
    throw std::runtime_error("Body must be a Decl.");
  }

  for (const auto &importDecl : imports) {
    if (importDecl->hasStar()) {
      continue;
    }

    auto qualifiedIdentifier = std::dynamic_pointer_cast<UnresolvedType>(
        importDecl->getQualifiedIdentifier());
    if (!qualifiedIdentifier) {
      throw std::runtime_error(
          "Import Decl is not an Unresolved Type in the AST construction");
    }
    std::string qualifiedName = qualifiedIdentifier->toString();
    std::string importedType = qualifiedIdentifier->getIdentifiers().back();

    // Ensure no conflicting single-type-import declarations
    if (existingImports.find(importedType) != existingImports.end() &&
        existingImports[importedType] != qualifiedName) {
      throw std::runtime_error(
          "No two single-type-import declarations clash with each other.");
    }

    existingImports.insert({importedType, qualifiedName});
  }
}

ClassDecl::ClassDecl(std::shared_ptr<Modifiers> modifiers, std::string name,
                     std::shared_ptr<ReferenceType> superClass,
                     std::shared_ptr<ReferenceType> objectType,
                     std::vector<std::shared_ptr<ReferenceType>> interfaces,
                     std::vector<std::shared_ptr<Decl>> classBodyDecls)
    : Decl{name}, modifiers{modifiers}, superClasses{superClass, objectType},
      interfaces{interfaces}, classBodyDecls{classBodyDecls} {
  // Check for valid modifiers.
  if (!modifiers) {
    throw std::runtime_error("Class Decl Invalid modifiers.");
  }
  if (modifiers->isInvalid()) {
    throw std::runtime_error("A class cannot be package private.");
  }
  if (modifiers->isAbstract() && modifiers->isFinal()) {
    throw std::runtime_error("A class cannot be both abstract and final.");
  }
  // Separate fields, constructors and methods
  bool foundConstructor = false;
  std::unordered_set<std::string> fieldNames;

  for (const auto &decl : classBodyDecls) {
    auto field = std::dynamic_pointer_cast<FieldDecl>(decl);
    if (field) {
      const auto &fieldName = field->getName();
      if (!fieldNames.insert(fieldName).second) {
        throw std::runtime_error("Field \"" + fieldName +
                                 "\" is already declared.");
      }
      continue;
    }
    auto method = std::dynamic_pointer_cast<MethodDecl>(decl);
    if (method && method->isConstructor()) {
      foundConstructor = true;
    }
    if (field == nullptr && method == nullptr) {
      throw std::runtime_error("Class Decl Invalid declarations for class " +
                               name);
    }
  }
  if (!foundConstructor) {
    throw std::runtime_error(
        "Every class must contain at least one explicit constructor");
  }

  for (const auto &decl : classBodyDecls) {
    auto method = std::dynamic_pointer_cast<MethodDecl>(decl);
    if (!method || !(method->isConstructor()))
      continue;
    if (name != method->getName()) {
      throw std::runtime_error("Constructor name must match class name.");
    }
  }

  std::unordered_set<std::string> interfaceNames;
  for (const auto &interface : interfaces) {
    const auto &interfaceName = interface->toString();
    if (!interfaceNames.insert(interfaceName).second) {
      throw std::runtime_error("Interface \"" + interfaceName +
                               "\" is already implemented.");
    }
  }
}

void ClassDecl::setParent(std::shared_ptr<CodeBody> parent) {
  // std::cout << "ClassDecl::setParent" << std::endl;
  auto program = std::dynamic_pointer_cast<ProgramDecl>(parent);
  Decl::setParent(parent);
  if (!(program->isDefaultPackage())) {
    // change name
  }

  for (auto &field : getFields())
    field->setParent(std::static_pointer_cast<CodeBody>(shared_from_this()));
  for (auto &method : getMethods())
    method->setParent(std::static_pointer_cast<CodeBody>(shared_from_this()));
}

std::vector<std::shared_ptr<MethodDecl>> ClassDecl::getConstructors() const {
  std::vector<std::shared_ptr<MethodDecl>> constructors;

  for (const auto &decl : classBodyDecls) {
    if (auto methodDecl = std::dynamic_pointer_cast<MethodDecl>(decl)) {
      if (methodDecl->isConstructor()) {
        constructors.push_back(methodDecl);
      }
    }
  }
  return constructors;
}

InterfaceDecl::InterfaceDecl(
    std::shared_ptr<Modifiers> modifiers, std::string name,
    std::vector<std::shared_ptr<ReferenceType>> interfaces,
    std::shared_ptr<ReferenceType> objectType,
    std::vector<std::shared_ptr<Decl>> interfaceBody)
    : Decl{name}, modifiers{modifiers}, interfaces{interfaces},
      objectType{objectType}, interfaceBodyDecls{interfaceBody} {
  if (!modifiers) {
    throw std::runtime_error("Interface Decl Invalid modifiers for interface " +
                             name);
  }
  if (modifiers->isFinal()) {
    throw std::runtime_error("An interface cannot be final for interface " +
                             name);
  }
  if (!modifiers->isPublic()) {
    throw std::runtime_error(
        "Interface must have a visibility modifier for interface " + name);
  }
  // Check declarations
  for (const auto &decl : interfaceBody) {
    auto field = std::dynamic_pointer_cast<FieldDecl>(decl);
    if (field) {
      throw std::runtime_error("An interface cannot contain fields.");
    }
    auto method = std::dynamic_pointer_cast<MethodDecl>(decl);
    if (method) {
      if (method->isConstructor()) {
        throw std::runtime_error("An interface cannot contain constructors.");
      }
      if (method->hasBody()) {
        throw std::runtime_error("An interface method cannot have a body.");
      }
      auto methodModifiers = method->getModifiers();
      if (methodModifiers) {
        if (methodModifiers->isStatic()) {
          throw std::runtime_error("An interface method cannot be static.");
        }
        if (methodModifiers->isFinal()) {
          throw std::runtime_error("An interface method cannot be final.");
        }
        if (methodModifiers->isNative()) {
          throw std::runtime_error("An interface method cannot be native.");
        }
        if (!methodModifiers->isAbstract()) {
          throw std::runtime_error(
              "An interface method must be abstract for interface " + name);
        }
      }
    }
  }
}

void InterfaceDecl::setParent(std::shared_ptr<CodeBody> parent) {
  // std::cout << "InterfaceDecl::setParent" << std::endl;
  auto program = std::dynamic_pointer_cast<ProgramDecl>(parent);
  Decl::setParent(parent);
  if (!program->isDefaultPackage()) {
    // change name
  }
  for (auto &method : getMethods())
    method->setParent(std::static_pointer_cast<CodeBody>(shared_from_this()));
}

MethodDecl::MethodDecl(std::shared_ptr<Modifiers> modifiers, std::string name,
                       std::shared_ptr<Type> returnType,
                       std::vector<std::shared_ptr<VarDecl>> params,
                       bool isConstructor, std::shared_ptr<Block> methodBody,
                       const source::SourceRange loc)
    : Decl{name, loc}, modifiers{modifiers}, returnType{returnType},
      params{params}, isConstructor_{isConstructor}, methodBody{methodBody} {
  // Check for valid modifiers
  if (!modifiers || modifiers->isInvalid()) {
    throw std::runtime_error("Method Decl Invalid modifiers for method " +
                             name);
  }
  // Restrictions for constructors
  if (isConstructor) {
    if (modifiers->isAbstract()) {
      throw std::runtime_error("A constructor cannot be abstract.");
    }
    if (modifiers->isFinal()) {
      throw std::runtime_error("A constructor cannot be final.");
    }
    if (modifiers->isStatic()) {
      throw std::runtime_error("A constructor cannot be static.");
    }
    if (modifiers->isNative()) {
      throw std::runtime_error("A constructor cannot be native.");
    }
  } else {
    // Restritions for non-constructor methods
    // A method has a body iff it is neither abstract nor native
    if ((modifiers->isAbstract() || modifiers->isNative()) && methodBody) {
      throw std::runtime_error(
          "An abstract or native method cannot have a body for method " + name);
    }
    if (!modifiers->isAbstract() && !modifiers->isNative() && !methodBody) {
      throw std::runtime_error(
          "A non-abstract and non-native method must have a body for method " +
          name);
    }
  }
  // Other restrictions for modifiers. These apply to constructors and methods.
  if (modifiers->isPublic() && modifiers->isProtected()) {
    throw std::runtime_error("A method or constructor cannot be both public "
                             "and protected for method " +
                             name);
  }
  if (!modifiers->isPublic() && !modifiers->isProtected()) {
    throw std::runtime_error(
        "A method must have a visibility modifier for method " + name);
  }
  if (modifiers->isAbstract() &&
      (modifiers->isStatic() || modifiers->isFinal())) {
    throw std::runtime_error("An abstract method cannot be static or final for "
                             "method " +
                             name);
  }
  if (modifiers->isStatic() && modifiers->isFinal()) {
    throw std::runtime_error("A static method cannot be final for method " +
                             name);
  }

  // check native methods
  if (modifiers->isNative()) {
    if (!modifiers->isStatic()) {
      throw std::runtime_error("A native method must be static for method " +
                               name);
    }
    if (params.size() != 1) {
      throw std::runtime_error("A native method must have exactly one "
                               "parameter for method " +
                               name);
    }
    if (auto type = std::dynamic_pointer_cast<BasicType>(returnType)) {
      if (type->getType() != BasicType::Type::Int) {
        throw std::runtime_error("A native method must return int for method " +
                                 name);
      }
    }
    if (auto type =
            std::dynamic_pointer_cast<BasicType>(params[0]->getType())) {
      if (type->getType() != BasicType::Type::Int) {
        throw std::runtime_error("A native method must have parameter of type "
                                 "int for method " +
                                 name);
      }
    }
  }
  checkSuperThisCalls(methodBody);
}

void MethodDecl::setParent(std::shared_ptr<CodeBody> parent) {
  // std::cout << "MethodDecl::setParent" << std::endl;
  Decl::setParent(parent);
  auto parentDecl = std::dynamic_pointer_cast<Decl>(parent);
  if (!parentDecl)
    throw std::runtime_error("Field Decl Parent must be a Decl");
  if (!(std::dynamic_pointer_cast<ClassDecl>(parentDecl) ||
        std::dynamic_pointer_cast<InterfaceDecl>(parentDecl))) {
    throw std::runtime_error(
        "Method Decl Parent must be a ClassDecl or InterfaceDecl");
  }
  if (modifiers->isStatic()) {
    // change name
  }

  for (auto &local : localDecls) {
    local->setParent(std::static_pointer_cast<CodeBody>(shared_from_this()));
    if (local->getParent().get() != this) {
      throw std::runtime_error("child decl set Parent failed at MethodDecl");
    }
  }
}

void MethodDecl::checkSuperThisCalls(std::shared_ptr<Block> block) const {
  if (!block) {
    return;
  }
  std::shared_ptr<MethodInvocation> methodToCheck;
  for (auto statement : block->getStatements()) {
    if (!statement)
      continue;
    if (auto nestedBlock = std::dynamic_pointer_cast<Block>(statement)) {
      checkSuperThisCalls(nestedBlock);
      continue;
    }
    if (auto expressionStmt =
            std::dynamic_pointer_cast<ExpressionStmt>(statement)) {
      if (!(expressionStmt->getStatementExpr()))
        continue;
      auto opNode = expressionStmt->getStatementExpr()->getLastExprNode();
      if (auto methodInvocation =
              std::dynamic_pointer_cast<MethodInvocation>(opNode)) {
        methodToCheck = methodInvocation;
      }
    } else if (auto returnStmt =
                   std::dynamic_pointer_cast<ReturnStmt>(statement)) {
      if (!(returnStmt->getReturnExpr()))
        continue;
      auto opNode = returnStmt->getReturnExpr()->getLastExprNode();
      if (auto methodInvocation =
              std::dynamic_pointer_cast<MethodInvocation>(opNode)) {
        methodToCheck = methodInvocation;
      }
    }
    if (!methodToCheck) {
      continue;
    }
    // TODO
    // auto qid = methodToCheck->getQualifiedIdentifier();
    // if (qid.empty()) {
    //   // TODO: This is the primary DOT ID case. Need to check whether the
    //   // primary (i.e. expr) is a this() or super() method invocation.
    //   continue;
    // }
    // for (const auto &expr : qid) {
    //   auto memberName = std::dynamic_pointer_cast<MemberName>(expr);
    //   if (!memberName) {
    //     continue; // will it happen?
    //   }
    //   auto id = memberName->getName();
    //   if (id == "this") {
    //     throw std::runtime_error("A method or constructor must not contain "
    //                              "explicit this() calls for method " +
    //                              getName());
    //   } else if (id == "super") {
    //     throw std::runtime_error("A method or constructor must not contain "
    //                              "explicit super() calls for method " +
    //                              getName());
    //   }
    // }
  }
}

FieldDecl::FieldDecl(std::shared_ptr<Modifiers> modifiers,
                     std::shared_ptr<Type> type, std::string name,
                     std::shared_ptr<Expr> initializer,
                     std::shared_ptr<ScopeID> scope,
                     const source::SourceRange &loc, bool allowFinal)
    : modifiers{modifiers}, VarDecl{type, name, initializer, scope, loc} {
  if (!modifiers) {
    throw std::runtime_error("Field Decl Invalid modifiers.");
  }
  if (!allowFinal && modifiers->isFinal()) {
    throw std::runtime_error("A field cannot be final");
  }
  if (modifiers->isAbstract()) {
    throw std::runtime_error("A field cannot be abstract");
  }
  if (modifiers->isNative()) {
    throw std::runtime_error("A field cannot be native");
  }
  if (modifiers->isPublic() && modifiers->isProtected()) {
    throw std::runtime_error("A field cannot be both public and protected.");
  }
  if (!modifiers->isPublic() && !modifiers->isProtected()) {
    throw std::runtime_error("A field must have a visibility modifier.");
  }
}

std::ostream &AstNode::printIndent(std::ostream &os, int indent) const {
  for (int i = 0; i < indent; ++i) {
    os << "  ";
  }
  return os;
}

// Prints
std::ostream &InterfaceDecl::print(std::ostream &os, int indent) const {
  os << "InterfaceDecl {}\n";
  return os;
}

std::ostream &ProgramDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ProgramDecl \n";

  // Print Package (For some reason, we're not using PackageDecl)
  printIndent(os, indent + 1);
  os << "(Package \n";
  package->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << ")\n";

  // Print imports
  for (auto &importDecl : imports) {
    importDecl->print(os, indent + 1);
  }

  // Print Body
  body->print(os, indent + 1);
  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &ImportDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ImportDecl \n";
  qualifiedIdentifier->print(os, indent + 1);
  printIndent(os, indent);
  os << ")\n";
  return os;
}

// It seems like we never instantiate a PackageDecl...
std::ostream &PackageDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(PackageDecl \n";
  qualifiedIdentifier->print(os, indent + 1);
  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &ClassDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ClassDecl \n";
  // Print Modifiers
  printIndent(os, indent + 1);
  os << "modifiers: {\n";
  printIndent(os, indent + 2);
  os << *modifiers << "\n";
  printIndent(os, indent + 1);
  os << "}\n";

  // Print Superclasses
  printIndent(os, indent + 1);
  os << "superClasses: {\n";
  for (auto &superClass : superClasses) {
    if (superClass) {
      superClass->print(os, indent + 2);
    }
  }
  printIndent(os, indent + 1);
  os << "}\n";

  // Print Interfaces
  printIndent(os, indent + 1);
  os << "interfaces: [";
  bool interfaceNewline = false;
  for (auto &interface : interfaces) {
    if (interface) {
      os << "\n";
      interface->print(os, indent + 2);
      interfaceNewline = true;
    }
  }
  if (interfaceNewline) {
    os << "\n";
    printIndent(os, indent + 1);
  }
  os << "]\n";

  // Print Object Type
  if (objectType) {
    objectType->print(os, indent + 1);
  }

  // Print Body
  for (auto &decl : classBodyDecls) {
    if (decl) {
      decl->print(os, indent + 1);
    }
  }

  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &VarDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  if (dynamic_cast<const FieldDecl *>(this)) {
    os << "(FieldDecl \n";
  } else {
    os << "(VarDecl \n";
  }

  // Print Type
  type->print(os, indent + 1);

  // Print Scope
  printIndent(os, indent + 1);
  os << "ScopeID: " << *scope << "\n";

  // Print Initializer
  if (initializer) {
    printIndent(os, indent + 1);
    os << "Initializer: { \n";
    initializer->print(os, indent + 2);
    printIndent(os, indent + 1);
    os << "}\n";
  } else {
    printIndent(os, indent + 1);
    os << "Initializer: N/A\n";
  }

  printIndent(os, indent);
  os << ")\n";

  return os;
}

std::ostream &MethodDecl::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(MethodDecl \n";

  // Print Modifiers
  printIndent(os, indent + 1);
  os << "modifiers: {\n";
  printIndent(os, indent + 2);
  os << *modifiers << "\n";
  printIndent(os, indent + 1);
  os << "}\n";

  // Print Type
  if (isConstructor_) {
    printIndent(os, indent + 1);
    os << "IsConstructor: True\n";
  } else {
    printIndent(os, indent + 1);
    os << "IsConstructor: False\n";
    printIndent(os, indent + 1);
    os << "Return Type: ";
    if (returnType)
      returnType->print(os);
    else
      os << "Void\n";
  }

  // Print Params
  printIndent(os, indent + 1);
  os << "params: [";
  bool paramsFirst = true;
  for (auto &param : params) {
    if (paramsFirst)
      os << "\n";
    if (param) {
      param->print(os, indent + 2);
    }
    paramsFirst = false;
  }
  if (!paramsFirst)
    printIndent(os, indent + 1);
  os << "]\n";

  // Print LocalDecls
  printIndent(os, indent + 1);
  os << "localDecls: [";
  bool localDeclsFirst = true;
  for (auto &localDecl : localDecls) {
    if (localDeclsFirst)
      os << "\n";
    if (localDecl) {
      localDecl->print(os, indent + 2);
    }
    localDeclsFirst = false;
  }
  if (!localDeclsFirst)
    printIndent(os, indent + 1);
  os << "]\n";

  // Print Body
  if (methodBody) {
    methodBody->print(os, indent + 1);
  } else {
    printIndent(os, indent + 1);
    os << "MethodBody: N/A\n";
  }

  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &Block::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(Block \n";

  // Print Statements
  for (auto &statement : statements) {
    statement->print(os, indent + 1);
  };

  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &IfStmt::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(IfStmt \n";

  // Print condition
  printIndent(os, indent + 1);
  os << "condition: { \n";
  condition->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print If
  printIndent(os, indent + 1);
  os << "ifBody: { \n";
  ifBody->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print Else
  if (elseBody) {
    printIndent(os, indent + 1);
    os << "elseBody: { \n";
    ifBody->print(os, indent + 2);
    printIndent(os, indent + 1);
    os << "}\n";
  }

  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &WhileStmt::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(WhileStmt \n";

  // Print condition
  printIndent(os, indent + 1);
  os << "condition: { \n";
  if (condition)
    condition->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print body
  printIndent(os, indent + 1);
  os << "whileBody: { \n";
  whileBody->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  printIndent(os, indent);
  os << ")\n";
  return os;
}

std::ostream &ForStmt::print(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ForStmt \n";

  // Print forInit
  printIndent(os, indent + 1);
  os << "forInit: { \n";
  if (forInit)
    forInit->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print condition
  printIndent(os, indent + 1);
  os << "condition: { \n";
  if (condition)
    condition->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print forUpdate
  printIndent(os, indent + 1);
  os << "forUpdate: { \n";
  if (forUpdate)
    forUpdate->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  // Print forBody
  printIndent(os, indent + 1);
  os << "forBody: { \n";
  forBody->print(os, indent + 2);
  printIndent(os, indent + 1);
  os << "}\n";

  printIndent(os, indent);
  os << ")\n";
  return os;
}

void FieldDecl::setParent(std::shared_ptr<CodeBody> parent) {
  // std::cout << "FieldDecl::setParent" << std::endl;
  Decl::setParent(parent);
  auto parentDecl = std::dynamic_pointer_cast<Decl>(parent);
  if (!parentDecl)
    throw std::runtime_error("Field Decl Parent must be a Decl");
  if (modifiers->isStatic()) {
    // change name
  }
}

bool ReferenceType::operator==(const Type &other) const {
  const ReferenceType *otherRef = dynamic_cast<const ReferenceType *>(&other);
  if (!otherRef) {
    return false;
  }

  if (resolvedDecl->getAstNode() != otherRef->resolvedDecl->getAstNode()) {
    return false;
  }

  return true;
}

} // namespace parsetree::ast
