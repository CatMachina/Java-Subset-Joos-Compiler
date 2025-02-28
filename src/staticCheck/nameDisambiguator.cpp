#include "staticCheck/nameDisambiguator.hpp"

namespace static_check {

//////////////////// Helpers ////////////////////

// std::shared_ptr<parsetree::ast::Decl>
// lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> context,
//                 std::string name) {
//   if (!context) {
//     return nullptr;
//   }

//   // Q: I didn't really understand this function here...
//   auto matchesCondition = [name, this](const
//   std::shared_ptr<parsetree::ast::Decl>& decl) {
//     auto typedDecl =
//     std::dynamic_pointer_cast<parsetree::ast::TypedDecl>(decl); if
//     (!typedDecl) return false;

//     // Check if it's visible in the current scope
//     bool scopeVisible = !this->lscope_ ||
//     this->lscope_->canView(typedDecl->scope());

//     // Additional access check for fields
//     bool canAccess = true;
//     if (auto fieldDecl =
//     std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
//       canAccess = isAccessible(fieldDecl->getModifiers(),
//       fieldDecl->getParent());
//     }

//     return (decl->getName() == name) && scopeVisible && canAccess;
//   };

//   // Step 2: Is it a field contained the current class?

//   auto classDecl =
//   std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(context);
//   // Check contain(C), where C is the current class.
//   // Note: contain(C) is the union of inherit(C) and declare(C)
//   if (classDecl) {
//     // Check inherit(C)
//     std::shared_ptr<parsetree::ast::Decl> foundDecl = nullptr;
//     std::unordered_map<std::string,
//     std::shared_ptr<parsetree::ast::FieldDecl>> inheritedFields;
//     heirarchyCheck->getInheritedFields(classDecl, inheritedFields);

//     for (const auto& tuple : inheritedFields) {
//       auto name = tuple.first;
//       auto fieldDecl = tuple.second;
//       if (matchesCondition(fieldDecl)) {
//         if (foundDecl) return nullptr; // Ambiguous case
//         foundDecl = fieldDecl;
//       }
//     }
//     if (foundDecl) return foundDecl;

//     // Check declare(C)
//     for (const auto& fieldDecl : classDecl->getFields()) {
//       if (matchesCondition(fieldDecl)) return fieldDecl;
//     }
//   }

//   return nullptr;
// }

// std::shared_ptr<parsetree::ast::Decl>
// NameDisambiguator::findInContext(std::shared_ptr<parsetree::ast::CodeBody>
// ctx,
//                                   std::shared_ptr<MemberName> node) {
//   if (auto decl = lookupNamedDecl(currentContext, node->getName())) {
//     if (auto varDecl =
//             std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
//       // data->reclassify(ExprName::Type::ExpressionName, varDecl);
//       return varDecl;
//     } else if (auto fieldDecl =
//                    std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl))
//                    {
//       // data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
//       return fieldDecl;
//     }
//   }
//   if (auto ctxDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)) {
//     if (auto parentCtx = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(
//             ctxDecl->getParent())) {
//       return findInContext(parentCtx, node);
//     }
//   }
//   return false;
// }
/*
JLS 6.5.2:
1. local variable decl, param decl, or field decl → ExpressionName.
2. local or member types (eg. local class) → TypeName.
3. in top-level class/interface in the same compilation unit or
single-type-import declaration → TypeName.
4. types in the same package (other compilation units) → TypeName.
5. type-import-on-demand in same compilation unit: Exactly one match →
TypeName, else → Compile-time error
6. none of the above, assume it's a package name → PackageName.

May be we can split it into reclassify Decl and reclassify import?
Decl: Search in this context, if not found, search parent context
Import: search in type linker context
*/

// Q: But 3,4,5,6 already implemented in type linking, right?
// A: seems so yeah

// TODO: Decl obj for field?
std::shared_ptr<parsetree::ast::Decl> NameDisambiguator::findInContainSet(const std::string &name) {
  std::cout << "findInContainSet: " << name << std::endl;
  auto currentClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(currentContext);
  if (!currentClass) {
    throw std::runtime_error("Current class not exist");
  }
  // Check inherited fields
  auto inheritedFields = hierarchyChecker->getInheritedFields(currentClass);
  auto it = inheritedFields.find(name);
  if (it != inheritedFields.end()) {
    return it->second;
  }
  return nullptr;
}

void NameDisambiguator::disambiguate(
    std::shared_ptr<parsetree::ast::Expr> expr,
    std::vector<std::shared_ptr<parsetree::ast::MemberName>> &memberNames) {
  if (memberNames.empty()) {
    return;
  }
  if (memberNames[0]->getName() == "length") {
    return;
  }

  // 1. Local variable a_1 is in scope
  std::cout << memberNames.size() << " identifiers: ";
  for (auto memberName : memberNames) {
    std::cout << memberName->getName() << ", ";
  }
  std::cout << std::endl;
  auto localDecl = findInScopes(memberNames[0]->getName());
  if (localDecl != nullptr) {
    std::cout << "Found " << localDecl->getName() << std::endl;
    memberNames[0]->setResolvedDecl(localDecl);
    return;
  }

  // 2. Field a_1 is contained in the current class
  auto fieldDecl = findInContainSet(memberNames[0]->getName());
  if (fieldDecl != nullptr) {
    std::cout << "Found " << fieldDecl->getName() << std::endl;
    memberNames[0]->setResolvedDecl(fieldDecl);
    return;
  }

  // 3. a_1.(...).a_k is a type in the global environment
  bool foundType = false;
  std::vector<std::string> identifiers;
  for (auto memberName : memberNames) {
    identifiers.push_back(memberName->getName());
    Package::packageChild resolved = typeLinker->resolveQualifiedName(identifiers, currentProgram);
    if (std::holds_alternative<std::shared_ptr<Package>>(resolved)) {
      continue;
    }
    if (std::holds_alternative<std::shared_ptr<Decl>>(resolved)) {
      auto decl = std::get<std::shared_ptr<Decl>>(resolved);
      if (!decl) {
        throw std::runtime_error("Ambiguous import-on-demand conflict");
      }
      memberName->setResolvedDecl(decl->getAstNode());
      foundType = true;
      break;
    }
    throw std::runtime_error("Failed to disambiguate. No import for " +
      memberName->getName());
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
  int i = 0;
  while (i < exprNodes.size()) {
    if (auto memberName = std::dynamic_pointer_cast<parsetree::ast::MemberName>(
            exprNodes[i])) {
      std::vector<std::shared_ptr<parsetree::ast::MemberName>> memberNames;
      // Find longest contiguous subarray of memberNames
      while (i < exprNodes.size()) {
        // auto fieldAccess =
        //     std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(
        //         exprNodes[i]);
        auto memberName =
            std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNodes[i]);
        if (!memberName) {
          break;
        }
        memberNames.push_back(memberName);
        i++;
      }
      // The expression is a.b.c.d() => we know d is a method
      // Still need to disambiguate a.b.c
      if (auto methodName =
              std::dynamic_pointer_cast<parsetree::ast::MethodName>(
                  memberNames.back())) {
        memberNames.pop_back();
      }
      disambiguate(expr, memberNames);
    }
    i++;
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
  std::cout << "findInScopes: " << name << std::endl;
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    std::cout << "Scope size = " << it->size() << std::endl;
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

  auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node);
  auto interfaceDecl =
      std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(node);
  auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node);

  if (classDecl)
    std::cout << "Class: " << classDecl->getName() << std::endl;
  if (interfaceDecl)
    std::cout << "Interface: " << interfaceDecl->getName() << std::endl;

  if (classDecl || interfaceDecl || block)
    enterScope();

  if (auto expr = std::dynamic_pointer_cast<parsetree::ast::Expr>(node)) {
    resolveExpr(expr);
  } else {
    for (auto child : node->getChildren()) {
      if (!child)
        continue;
      resolveAST(child);
    }
  }

  // TODO: Forbid forward declaration
  /*
  The initializer for a non-static field must not refer by simple name to
  itself or a non-static field declared later in the same class, except as the
  direct left-hand side of an assignment.
  */

  auto varDecl = std::dynamic_pointer_cast<parsetree::ast::VarDecl>(node);
  if (varDecl)
    resolveVarDecl(varDecl);

  if (classDecl || interfaceDecl || block)
    leaveScope();
}

void NameDisambiguator::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast);
  }
}

} // namespace static_check
