#include "staticCheck/nameDisambiguator.hpp"

namespace static_check {

// TODO: is this an instance field in the current class?
bool NameDisambiguator::isInstanceField(std::string &id) { return true; }

// TODO: is this a static field in the current class?
bool NameDisambiguator::isStaticField(std::string &id) { return true; }

std::shared_ptr<parsetree::ast::Decl>
lookupNamedDecl(std::shared_ptr<parsetree::ast::DeclContext> ctx,
                std::string_view name) {
  auto condition = [name, this](std::shared_ptr<parsetree::ast::Decl> decl) {
    if (auto typedDecl =
            std::dynamic_pointer_cast<parsetree::ast::TypedDecl>(decl);
        typedDecl) {
      bool scopeVisible =
          !this->lscope_ || this->lscope_->canView(typedDecl->scope());
      bool canAccess = true;
      if (auto fieldDecl =
              std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
        canAccess =
            isAccessible(fieldDecl->getModifiers(), fieldDecl->getParent());
      }
      return decl->getName() == name && scopeVisible && canAccess;
    }
    return false;
  };

  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctx);
      classDecl) {
    std::shared_ptr<parsetree::ast::Decl> result = nullptr;
    // how to get inherited members? in hierarchy checking?
    for (const auto decl : getInheritedMembers(classDecl)) {
      if (condition(decl)) {
        if (result)
          return nullptr; // Ambiguous case
        result = decl;
      }
    }
    return result;
  }
  for (const auto decl : ctx->getDecls()) {
    if (condition(decl))
      return decl;
  }
  return nullptr;
}

bool NameDisambiguator::reclassifyDecl(
    std::shared_ptr<parsetree::ast::CodeBody> ctx,
    std::shared_ptr<ExprName> data) {
  if (auto decl = lookupNamedDecl(currentContext, data->getNode()->getName())) {
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      data->reclassify(ExprName::Type::ExpressionName, varDecl);
      return true;
    } else if (auto fieldDecl =
                   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
      return true;
    }
  }
  if (auto ctxDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)) {
    if (auto parentCtx = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(
            ctxDecl->getParent())) {
      return reclassifyDecl(parentCtx, data);
    }
  }
  return false;
}

// JLS 6.5.2
std::shared_ptr<ExprName> NameDisambiguator::disambiguate(
    std::shared_ptr<parsetree::ast::MemberName> node) {
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
  // before being a single ambiguous name, it could be method
  if (auto method =
          std::dynamic_pointer_cast<parsetree::ast::MethodName>(node)) {
    return std::make_shared<ExprName>(ExprName::Type::MethodName, method);
  }

  std::shared_ptr<ExprName> data =
      std::make_shared<ExprName>(ExprName::Type::SingleAmbiguousName, node);

  if (reclassifyDecl(currentContext, data))
    return data;

  // imports
  auto import =
      typeLinker->getImport(currentContext, data->getNode()->getName());
  if (!import) {
    throw std::runtime_error("No import for " + data->getNode()->getName());
  }
  if (auto decl = std::get_if<std::shared_ptr<parsetree::ast::Decl>>(import)) {
    if (!decl) {
      throw std::runtime_error("Ambiguous import-on-demand conflict");
    }
    data->reclassify(ExprName::Type::TypeName, decl);
    return data;
  } else if (auto pkg = std::get_if<std::shared_ptr<Package>>(import)) {
    data->reclassify(ExprName::Type::PackageName, pkg);
    return data;
  }

  /*
  std::vector<std::string> &identifiers =
      qualifiedIdentifiers->getIdentifiers();
  // 1. If local variable a_1 is in scope, we use it
  if (envManager->inScope(identifiers[0])) {
    // a_2, ..., a_n must be instance fields
    for (int i = 1; i < identifiers.size(); ++i) {
      if (!isInstanceField(identifiers[i])) {
        throw std::runtime_error("Not an instance field: " + identifiers[i]);
      }
    }
  }
  // 2. If field a_1 is contained in the current class, we use it
  // TODO: Do we keep track of the current class? Contain set of current class?
  if (isContained(identifiers[0])) {
    // a_2, ..., a_n must be instance fields
    for (int i = 1; i < identifiers.size(); ++i) {
      if (!isInstanceField(identifiers[i])) {
        throw std::runtime_error("Not an instance field: " + identifiers[i]);
      }
    }
  }

  // 3. If a_1.(...).a_k is a type in the global environment
  // Then a_{k+1} is a static field and the rest are instance fields
  std::vector<std::string> ids;
  for (int i = 0; i < identifiers.size(); ++i) {
    ids.push_back(identifiers[i]);
    if (typeLinker->resolveImport(ids)) {
      for (int j = i + 1; j < identifiers.size(); ++j) {
        if (j == i + 1 && !isStaticField(identifiers[j])) {
          throw std::runtime_error("Not a static field: " + identifiers[j]);
        }
        if (j != i + 1 && !isInstanceField(identifiers[j])) {
          throw std::runtime_error("Not an instance field: " + identifiers[j]);
        }
      }
    }
  }
  */
  throw std::runtime_error("Fail to disambiguate");
}

void NameDisambiguator::resolveAST(
    std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");
  if (auto cu =
          std::dynamic_pointer_cast<parsetree::ast::CompilationUnit>(node))
    this->beginProgram(cu);
  if (auto ctx = std::dynamic_pointer_cast<parsetree::ast::DeclContext>(node))
    this->beginContext(ctx);
  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node))
    currentClass = classDecl;

  if (auto field = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(node)) {
    isStaticContext = field->getModifiers().isStatic();
    if (field->hasInit()) {
      isInstFieldInitializer = !field->getModifiers().isStatic();
      fieldScope = field->init()->scope();
    }
  } else if (auto method =
                 std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(node)) {
    isStaticContext = method->getModifiers().isStatic();
  }

  if (auto decl = std::dynamic_pointer_cast<parsetree::ast::TypedDecl>(node)) {
    if (auto init = decl->getInit()) {
      evaluate(init);
    }
  } else if (auto stmt =
                 std::dynamic_pointer_cast<parsetree::ast::Stmt>(node)) {
    for (auto expr : stmt->getExprs()) {
      if (!expr)
        continue;
      evaluate(expr);
    }
  }

  if (std::dynamic_pointer_cast<parsetree::ast::DeclStmt>(node))
    return;

  for (auto child : node->getChildren() | std::views::filter([](auto c) {
                      return c != nullptr;
                    })) {
    resolveAST(child);
  }
}

} // namespace static_check
