#include "staticCheck/nameDisambiguator.hpp"

namespace static_check {

// TODO: is this an instance field in the current class?
bool NameDisambiguator::isInstanceField(std::string &id) { return true; }

// TODO: is this a static field in the current class?
bool NameDisambiguator::isStaticField(std::string &id) { return true; }

// JLS 6.5.2
void NameDisambiguator::disambiguate(
    std::shared_ptr<parsetree::ast::QualifiedIdentifier> qualifiedIdentifier) {
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
  throw std::runtime_error("Fail to disambiguate");
}

void NameDisambiguator::resolveAST(
    std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");
  for (auto child : node->getChilden()) {
    if (!child)
      continue;
    // Case: Qualified Name
    if (auto qualifiedIdentifier =
            std::dynamic_pointer_cast<parse::ast::QualifiedIdentifier>(node)) {
      disambiguate(qualifiedIdentifier);
    }
    // TODO: Method Invocation?
  }
}

void NameDisambiguator::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast->getBody());
  }
}

} // namespace static_check
