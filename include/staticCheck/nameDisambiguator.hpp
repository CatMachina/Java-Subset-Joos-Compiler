#pragma once

#include "ast/ast.hpp"
#include "staticCheck/envManager.hpp"
#include <memory>

namespace static_check {

class NameDisambiguator {
public:
  NameDisambiguator(std::unique_ptr<parsetree::ast::ASTManager> astManager,
                    std::unique_ptr<EnvManager> envManager)
      : astManager(std::move(astManager)), envManager(std::move(envManager)) {}

  void disambiguate(
      std::shared_ptr<parsetree::ast::QualifiedIdentifier> qualifiedIdentifier);

private:
  std::unique_ptr<parsetree::ast::ASTManager> astManager;
  std::unique_ptr<EnvManager> envManager;
};

} // namespace static_check