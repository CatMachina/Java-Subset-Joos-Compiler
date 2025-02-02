#include "ast/astNode.hpp"

namespace parsetree::ast {

ClassDecl::ClassDecl(
    std::shared_ptr<Modifiers> modifiers, std::string_view name,
    std::shared_ptr<QualifiedIdentifier> superClass,
    std::vector<std::shared_ptr<QualifiedIdentifier>> interfaces,
    std::vector<std::shared_ptr<Decl>> classBodyDecls)
    : Decl{name}, modifiers{modifiers}, superClass{superClass},
      interfaces{interfaces} {
  // Check for valid modifiers.
  if (!modifiers) {
    throw std::runtime_error("Class Decl Invalid modifiers.");
  }
  if (modifiers->isPackagePrivate()) {
    throw std::runtime_error("A class cannot be package private.");
  }
  if (modifiers->isAbstract() && modifiers->isFinal()) {
    throw std::runtime_error("A class cannot be both abstract and final.");
  }
  // Separate fields, constructors and methods
  bool foundConstructor = false;
  for (const auto &decl : classBodyDecls) {
    auto field = std::dynamic_pointer_cast<FieldDecl>(decl);
    if (field) {
      this->fields.push_back(field);
      continue;
    }
    auto methodDecl = std::dynamic_pointer_cast<MethodDecl>(decl);
    if (methodDecl && methodDecl->isConstructor()) {
      foundConstructor = true;
      this->constructors.push_back(methodDecl);
    } else if (methodDecl) {
      // Non-constructor
      this->methods.push_back(methodDecl);
    }
  }
  if (!foundConstructor) {
    throw std::runtime_error(
        "Every class must contain at least one explicit constructor");
  }
}

InterfaceDecl::InterfaceDecl(
    std::shared_ptr<Modifiers> modifiers, std::string_view name,
    std::vector<std::shared_ptr<QualifiedIdentifier>> extendsInterfaces,
    std::vector<std::shared_ptr<Decl>> interfaceBody)
    : Decl{name}, modifiers{modifiers}, extendsInterfaces{extendsInterfaces},
      interfaceBody{interfaceBody} {
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
      }
    }
  }
}

MethodDecl::MethodDecl(std::shared_ptr<Modifiers> modifiers,
                       std::string_view name, std::shared_ptr<Type> returnType,
                       std::vector<std::shared_ptr<VarDecl>> params,
                       bool isConstructor, std::shared_ptr<Block> methodBody)
    : Decl{name}, modifiers{modifiers}, returnType{returnType}, params{params},
      isConstructor_{isConstructor}, methodBody{methodBody} {
  // Check for valid modifiers
  if (!modifiers) {
    throw std::runtime_error("Method Decl Invalid modifiers for method " +
                             std::string(name));
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
          "An abstract or native method cannot have a body for method " +
          std::string(name));
    }
    if (!modifiers->isAbstract() && !modifiers->isNative() && !methodBody) {
      throw std::runtime_error(
          "A non-abstract and non-native method must have a body for method " +
          std::string(name));
    }
  }
  // Other restrictions for modifiers. These apply to constructors and methods.
  if (modifiers->isPublic() && modifiers->isProtected()) {
    throw std::runtime_error("A method or constructor cannot be both public "
                             "and protected for method " +
                             std::string(name));
  }
  if (modifiers->isAbstract() &&
      (modifiers->isStatic() || modifiers->isFinal())) {
    throw std::runtime_error("An abstract method cannot be static or final for "
                             "method " +
                             std::string(name));
  }
  if (modifiers->isStatic() && modifiers->isFinal()) {
    throw std::runtime_error("A static method cannot be final for method " +
                             std::string(name));
  }
  if (modifiers->isNative() && !modifiers->isStatic()) {
    throw std::runtime_error("A native method must be static for method " +
                             std::string(name));
  }
  checkSuperThisCalls(methodBody);
}

void MethodDecl::checkSuperThisCalls(std::shared_ptr<Block> block) const {
  if (!block) {
    return;
  }
  for (auto statement : block->getStatements()) {
    if (auto expressionStmt =
            std::dynamic_pointer_cast<ExpressionStmt>(statement)) {
      if (auto methodInvocation = std::dynamic_pointer_cast<MethodInvocation>(
              expressionStmt->getStatementExpr())) {
        auto qid = methodInvocation->getQualifiedIdentifier();
        if (qid->toString() == "this") {
          throw std::runtime_error("A method or constructor must not contain "
                                   "explicit this() calls for method " +
                                   getName());
        } else if (qid->toString() == "super") {
          throw std::runtime_error("A method or constructor must not contain "
                                   "explicit super() calls for method " +
                                   getName());
        }
      }
    } else if (auto nestedBlock = std::dynamic_pointer_cast<Block>(statement)) {
      checkSuperThisCalls(nestedBlock);
    }
  }
}

FieldDecl::FieldDecl(std::shared_ptr<Modifiers> modifiers,
                     std::shared_ptr<Type> type, std::string_view name)
    : modifiers{modifiers}, VarDecl{type, name} {
  if (!modifiers) {
    throw std::runtime_error("Field Decl Invalid modifiers.");
  }
  if (modifiers->isFinal()) {
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

// Prints
std::ostream &InterfaceDecl::print(std::ostream &os) const {
  os << "InterfaceDecl {}\n";
  return os;
}

std::ostream &ProgramDecl::print(std::ostream &os) const {
  os << "ProgramDecl {}\n";
  return os;
}

std::ostream &ClassDecl::print(std::ostream &os) const {
  os << "ProgramDecl {}\n";
  return os;
}

} // namespace parsetree::ast
