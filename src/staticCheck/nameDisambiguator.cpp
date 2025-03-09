#include "staticCheck/nameDisambiguator.hpp"

namespace static_check {

//////////////////// Helpers ////////////////////

// TODO: Decl obj for field?

std::shared_ptr<parsetree::ast::Decl>
NameDisambiguator::findInCurrentClass(const std::string &name) {
  auto currentClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(currentContext);
  if (!currentClass) {
    throw std::runtime_error("Current class does not exist");
  }
  for (const auto fieldDecl : currentClass->getFields()) {
    if (fieldDecl->getName() == name) {
      return fieldDecl;
    }
  }
  // Not found
  return nullptr;
}

std::shared_ptr<parsetree::ast::Decl>
NameDisambiguator::findInSuperClasses(const std::string &name) {
  auto currentClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(currentContext);
  if (!currentClass) {
    throw std::runtime_error("Current class does not exist");
  }
  // Check inherited fields
  auto inheritedFields = hierarchyChecker->getInheritedFields(currentClass);
  auto it = inheritedFields.find(name);
  if (it != inheritedFields.end()) {
    return it->second;
  }
  // Not found
  return nullptr;
}

bool NameDisambiguator::isLegalReference(bool isFieldInitializer,
                                         bool nextIsAssignment) {
  if (currentField || isFieldInitializer) {
    return nextIsAssignment;
  }
  return true;
}

/*
JLS 8.3.2.3
The initializer of a non-static field must not use (i.e., read) by simple name
(i.e., without an explicit this) itself or a non-static field declared later in
the same class.
*/
void NameDisambiguator::disambiguate(
    std::shared_ptr<parsetree::ast::QualifiedName> qualifiedName,
    bool isFieldInitializer, bool nextIsAssignment) {
  if (qualifiedName->size() == 0) {
    return;
  }
  if (qualifiedName->getName(0) == "length") {
    return;
  }
  if (qualifiedName->getName(0) == "this") {
    // qualifiedName->get(0)->setResolvedDecl(currentClass);
    auto type = std::make_shared<parsetree::ast::ReferenceType>(currentClass);
    type->setResolvedDecl(std::make_shared<Class>(currentClass));
    qualifiedName->get(0)->resolveDeclAndType(currentClass, type);
    return;
  }

  // 1. Local variable a_1 is in scope
  auto localDecl = findInScopes(qualifiedName->getName(0));
  if (localDecl != nullptr) {
    std::cout << "Found local decl: " << localDecl->getName() << std::endl;
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(localDecl)) {
      qualifiedName->get(0)->resolveDeclAndType(localDecl, varDecl->getType());
    }
    // Shouldn't be method decl
    // else if (auto methodDecl =
    //                std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
    //                    localDecl)) {
    //   qualifiedName->get(0)->resolveDeclAndType(localDecl,
    //                                             methodDecl->getReturnType());
    // }
    else {
      qualifiedName->get(0)->setResolvedDecl(localDecl);
    }
    return;
  }

  // 2. Field a_1 is contained in the current class

  // Search in the declared set of current class
  auto fieldDecl = findInCurrentClass(qualifiedName->getName(0));
  if (fieldDecl != nullptr) {
    if (!isLegalReference(isFieldInitializer, nextIsAssignment)) {
      throw std::runtime_error("Forward reference in a non-static field "
                               "initializer is not allowed.");
    }
    std::cout << "Found field in declare set: " << fieldDecl->getName()
              << std::endl;
    // qualifiedName->get(0)->setResolvedDecl(fieldDecl);
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(fieldDecl)) {
      qualifiedName->get(0)->resolveDeclAndType(fieldDecl, varDecl->getType());
    }
    // Shouldn't be method decl
    // else if (auto methodDecl =
    //                std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
    //                    fieldDecl)) {
    //   qualifiedName->get(0)->resolveDeclAndType(localDecl,
    //                                             methodDecl->getReturnType());
    // }
    else {
      qualifiedName->get(0)->setResolvedDecl(fieldDecl);
    }
    return;
  }
  // Search in the inherit set of current class
  fieldDecl = findInSuperClasses(qualifiedName->getName(0));
  if (fieldDecl != nullptr) {
    std::cout << "Found field in inherit set: " << fieldDecl->getName()
              << std::endl;
    // qualifiedName->get(0)->setResolvedDecl(fieldDecl);
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(fieldDecl)) {
      qualifiedName->get(0)->resolveDeclAndType(fieldDecl, varDecl->getType());
    } else {
      qualifiedName->get(0)->setResolvedDecl(fieldDecl);
    }
    return;
  }

  // 3. a_1.(...).a_k is a type in the global environment
  bool foundType = false;
  std::vector<std::string> prefix;
  for (int i = 0; i < qualifiedName->size(); ++i) {
    prefix.push_back(qualifiedName->getName(i));
    Package::packageChild resolved =
        typeLinker->resolveQualifiedName(prefix, currentProgram);
    if (std::holds_alternative<std::shared_ptr<Package>>(resolved)) {
      continue;
    }
    if (std::holds_alternative<std::shared_ptr<Decl>>(resolved)) {
      auto decl = std::get<std::shared_ptr<Decl>>(resolved);
      if (!decl) {
        throw std::runtime_error("Ambiguous import-on-demand conflict");
      }
      if (auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
              decl->getAstNode())) {
        // qualifiedName->get(i)->resolveDeclAndType(decl->getAstNode(),
        //                                           classDecl->getObjectType());
        qualifiedName->get(i)->setResolvedDecl(decl->getAstNode());
      } else if (auto interfaceDecl =
                     std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                         decl->getAstNode())) {
        // qualifiedName->get(i)->resolveDeclAndType(decl->getAstNode(),
        //                                           interfaceDecl->getObjectType());
        qualifiedName->get(i)->setResolvedDecl(decl->getAstNode());
      } else {
        qualifiedName->get(i)->setResolvedDecl(decl->getAstNode());
      }
      if (i < qualifiedName->size() - 1) {
        qualifiedName->get(i + 1)->setShouldBeStatic();
      }
      foundType = true;
      break;
    }
    throw std::runtime_error("Failed to disambiguate. No import for " +
                             qualifiedName->getName(i));
  }

  if (!foundType) {
    throw std::runtime_error(
        "Failed to disambiguate. No prefix can be resolved");
  }
}

void NameDisambiguator::resolveExpr(
    std::shared_ptr<parsetree::ast::Expr> expr) {
  expr->print(std::cout);
  std::cout << std::endl;
  auto exprNodes = expr->getExprNodes();
  for (int i = 0; i < exprNodes.size(); ++i) {
    auto qualifiedName =
        std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(exprNodes[i]);
    if (!qualifiedName) {
      continue;
    }
    // Check to see if this entire expr represents a field initializer
    bool isFieldInitializer = false;
    auto lastAssignmentNode =
        std::dynamic_pointer_cast<parsetree::ast::Assignment>(
            expr->getLastExprNode());
    if (lastAssignmentNode) {
      auto secondLastQualifiedName =
          std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(
              expr->getExprNodes()[expr->getExprNodes().size() - 2]);
      if (secondLastQualifiedName) {
        auto lastComponent = secondLastQualifiedName->getLastName();
        auto localDecl = findInScopes(lastComponent);
        auto fieldDecl = findInCurrentClass(lastComponent);
        if (!localDecl && fieldDecl) {
          isFieldInitializer = true;
        }
      }
    }
    // Check to see if the name represents the direct LHS of an assignment
    bool nextIsAssignment = false;
    if (i < exprNodes.size() - 1) {
      if (auto assignment =
              std::dynamic_pointer_cast<parsetree::ast::Assignment>(
                  exprNodes[i + 1]))
        nextIsAssignment = true;
    }
    // Check to see if the field access and method invocation represent valid
    // qualified names
    bool nextIsFieldAccess = false;
    bool nextIsMethodInvocation = false;
    bool needsDisambiguation = false;
    if (i < exprNodes.size() - 1) {
      if (auto fieldAccess =
              std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
                  exprNodes[i + 1])) {
        nextIsFieldAccess = true;
        needsDisambiguation = (fieldAccess->nargs() == 1);
      }
      if (auto methodInvocation =
              std::dynamic_pointer_cast<parsetree::ast::MethodInvocation>(
                  exprNodes[i + 1])) {
        nextIsMethodInvocation = true;
        needsDisambiguation = methodInvocation->getNeedsDisambiguation();
      }
    }
    if ((nextIsFieldAccess || nextIsMethodInvocation) && !needsDisambiguation) {
      continue;
    }
    if (!nextIsMethodInvocation) {
      disambiguate(qualifiedName, isFieldInitializer, nextIsAssignment);
      continue;
    }
    std::shared_ptr<parsetree::ast::QualifiedName> tmpQualifiedName =
        std::make_shared<parsetree::ast::QualifiedName>();
    for (int j = 0; j < qualifiedName->size() - 1; ++j) {
      tmpQualifiedName->add(qualifiedName->get(j));
    }
    disambiguate(tmpQualifiedName, isFieldInitializer, nextIsAssignment);
  }
}

void NameDisambiguator::addToScope(std::string name,
                                   std::shared_ptr<parsetree::ast::Decl> decl) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); --it) {
    auto result = it->find(name);
    // TODO: Is it allowed?
    if (result != it->end())
      throw std::runtime_error("Shadowing is not allowed");
  }
  scopes[scopes.size() - 1].insert({name, decl});
}

std::shared_ptr<parsetree::ast::Decl>
NameDisambiguator::findInScopes(const std::string &name) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    auto result = it->find(name);
    if (result != it->end()) {
      return result->second;
    }
  }
  return nullptr;
}

void NameDisambiguator::enterScope() {
  std::unordered_map<std::string, std::shared_ptr<parsetree::ast::Decl>> scope;
  scopes.push_back(scope);
}

void NameDisambiguator::leaveScope() { scopes.pop_back(); }

void NameDisambiguator::resolveVarDecl(
    std::shared_ptr<parsetree::ast::VarDecl> decl) {
  // TODO: shadowing?
  scopes[scopes.size() - 1].insert({decl->getName(), decl});
}

void NameDisambiguator::enterProgram(
    std::shared_ptr<parsetree::ast::ProgramDecl> programDecl) {
  currentProgram = programDecl;
}

void NameDisambiguator::enterContext(
    std::shared_ptr<parsetree::ast::CodeBody> context) {
  currentContext = context;
}

void NameDisambiguator::resolveAST(
    std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");

  if (auto programDecl =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(node)) {
    enterProgram(programDecl);
  }
  if (auto codeBody =
          std::dynamic_pointer_cast<parsetree::ast::CodeBody>(node)) {
    enterContext(codeBody);
  }

  auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node);
  auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node);
  auto interfaceDecl =
      std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(node);
  auto fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(node);

  if (classDecl)
    std::cout << "=== Class: " << classDecl->getName() << std::endl;
  if (interfaceDecl)
    std::cout << "=== Interface: " << interfaceDecl->getName() << std::endl;

  if (classDecl || interfaceDecl || block)
    enterScope();
  if (classDecl)
    currentClass = classDecl;
  if (fieldDecl)
    currentField = fieldDecl;

  if (auto expr = std::dynamic_pointer_cast<parsetree::ast::Expr>(node)) {
    resolveExpr(expr);
  } else {
    for (auto child : node->getChildren()) {
      if (!child)
        continue;
      resolveAST(child);
    }
  }

  auto varDecl = std::dynamic_pointer_cast<parsetree::ast::VarDecl>(node);
  if (varDecl)
    resolveVarDecl(varDecl);

  if (classDecl || interfaceDecl || block)
    leaveScope();
  if (fieldDecl)
    currentField = nullptr;
}

void NameDisambiguator::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast);
  }
}

} // namespace static_check
