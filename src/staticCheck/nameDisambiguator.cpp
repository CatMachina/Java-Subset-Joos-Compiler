#include "staticCheck/nameDisambiguator.hpp"

namespace static_check {

//////////////////// Helpers ////////////////////

/*
// TODO: is this an non-static field in the current class?
bool NameDisambiguator::isNonStaticField(std::string &id) { return true; }

// TODO: is this a static field in the current class?
bool NameDisambiguator::isStaticField(std::string &id) { return true; }
*/

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
    // A: There is a getInheritedMethods in hiearchy checking, we might need a
    // getInheritedFields?
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

std::shared_ptr<parsetree::ast::Decl>
NameDisambiguator::reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                                  std::shared_ptr<MemberName> node) {
  if (auto decl = lookupNamedDecl(currentContext, node->getName())) {
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, varDecl);
      return varDecl;
    } else if (auto fieldDecl =
                   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
      return fieldDecl;
    }
  }
  if (auto ctxDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)) {
    if (auto parentCtx = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(
            ctxDecl->getParent())) {
      return reclassifyDecl(parentCtx, node);
    }
  }
  return false;
}

// JLS 6.5.2

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

// TODO
std::shared_ptr<Decl> NameDisambiguator::findInScope(
    std::shared_ptr<parsetree::ast::MemberName> identifier) {}

// TODO: Decl obj for field?
std::shared_ptr<Decl> NameDisambiguator::findInContainedSet(
    std::shared_ptr<parsetree::ast::MemberName> identifier) {
  auto currentClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(currentContext);
  if (!currentClass)
    throw std::runtime_error("Current class not exist");
  auto declaredFields = currentClass->getFields();
  for (auto field : declaredFields) {
    if (field->getName() == identifier->getName()) {
      return field;
    }
  }
  // Check inherited fields
  std::unordered_map<std::string, std::shared_ptr<parsetree::ast::FieldDecl>>
      inheritedFields;
  hierarchyCheck->getInheritedFields(currentClass, inheritedFields);
  auto field = inheritedFields[identifier->getName()];
  return field;
}

/*
Q: Not sure if we need to follow JLS 6.5.2 entirely
This implementation idea is from the online notes
My understanding is that: given a qualified name a.b.c.d
We only need to check the prefix during name disambiguation stage
If `a` is neither a local variable or a field contained in the current class
Then check `a.b`, `a.b.c`, `a.b.c.d` => Can it be resolved to a type in the
global environment? To record this info, can we store a pointer in the AST node
to a Decl? (Did we have Decl objects for inherited methods/fields?) I believe
the rest is deferred to type checking stage (static / non-static fields, etc)
The env manager can be used for local scopes?
We need to keep track of current class/interface as well
(Can we have a context manager for all scopes, using env manager and global
symbol table maybe?) (It seems that name disambiguation isn't a lot of work?)
(Maybe we can just have a type checker that includes name disambiguation?)
*/

void NameDisambiguator::disambiguate(
    std::shared_ptr<parsetree::ast::MemberName> expr) {
  /*
  Edward:
  we can just check all MemberNames instead? not Expr, or MethodName
  and step 1 and step 2 can just be combined and use reclassifyDecl instead?
  */
  auto decl = reclassifyDecl(currentContext, expr);
  if (decl != nullptr) {
    expr->setResolveDecl(decl);
    return;
  }
  /*
  std::vector<std::shared_ptr<parsetree::ast::MemberName>> memberNames;
  for (auto exprNode : expr->getExprNodes()) {
    if (auto memberName =
            std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode)) {
      memberNames.push_back(memberName);
    }
  }
  // 1. If local variable a_1 is in scope, we use it
  auto localDecl = findInScope(memberNames[0]);
  if (localDecl != nullptr) {
    expr->setResolveDecl(localDecl);
    return;
  }
  // 2. If field a_1 is contained in the current class, we use it
  auto fieldDecl = findInContainSet(memberNames[0]);
  if (fieldDecl != nullptr) {
    expr->setResolveDecl(fieldDecl);
    return;
  }
  */

  // 3. If a_1.(...).a_k is a type in the global environment
  // maybe this?
  auto &context = typeLinker->getContext(currentProgram);
  auto import = context.find(expr->getName()) != context.end()
                    ? context[expr->getName()]
                    : nullptr;
  if (!import) {
    throw std::runtime_error("No import for " + expr->getName());
  }
  if (auto decl = std::get_if<std::shared_ptr<parsetree::ast::Decl>>(import)) {
    if (!decl) {
      throw std::runtime_error("Ambiguous import-on-demand conflict");
    }
    // data->reclassify(ExprName::Type::TypeName, decl);
    expr->setResolveDecl(decl);
  } else if (auto pkg = std::get_if<std::shared_ptr<Package>>(import)) {
    // this is a package name...
    // data->reclassify(ExprName::Type::PackageName, pkg);
  }

  /*
  std::vector<std::string> identifiers;
  Package::packageChild typeDecl;
  for (int i = 0; i < memberNames.size(); ++i) {
    identifiers.push_back(memberNames[i]->getName());
    try {
      typeDecl = typeLinker->resolveImport(identifiers);
      if (typeDecl != nullptr) {
        expr->setResolveDecl(typeDecl);
      }
    } catch (const std::runtime_error &err) {
      std::cerr << "Runtime error: " << err.what() << std::endl;
    }
  }
  if (typeDecl == nullptr) {
    throw std::runtime_error("Failed to disambiguate");
  }
  */
}

// TODO: Method invocation: done, we skip method for disambiguation
void NameDisambiguator::resolveExprNode(
    std::shared_ptr<parsetree::ast::ExprNode> node) {
  auto name = std::dynamic_pointer_cast<parsetree::ast::MemberName>(node);
  if (!name)
    return; // currently only member name need disambiguation
  if (auto method =
          std::dynamic_pointer_cast<parsetree::ast::MethodName>(name)) {
    return; // method invocation don't need disambiguation
  }
  disambiguate(name);
}

void NameDisambiguator::resolveAST(
    std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");
  if (auto programDecl =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(node)) {
    beginProgram(programDecl);
  }
  if (auto codeBody =
          std::dynamic_pointer_cast<parsetree::ast::CodeBody>(node)) {
    beginContext(codeBody);
  }
  if (auto expr = std::dynamic_pointer_cast<parsetree::ast::ExprNode>(node)) {
    resolveExprNode(expr);
  }
  auto block = std::dynamic_pointer_cast<parsetree::ast::Block>(node);
  if (block) {
    enterScope();
  }
  for (auto child : node->getChildren()) {
    if (!child) {
      continue;
    }
    resolveAST(child);
  }
  if (block) {
    leaveScope();
  }
}

} // namespace static_check
