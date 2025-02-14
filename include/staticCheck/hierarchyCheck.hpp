#pragma once

#include "ast/ast.hpp"
#include "ast/astManager.hpp"
#include "environment.hpp"

namespace static_check {

class HierarchyCheck {
  std::shared_ptr<Package> rootPackage;
  // std::unique_ptr<parsetree::ast::ASTManager> astManager;
public:
  HierarchyCheck(std::shared_ptr<Package> rootPackage
                 //, std::unique_ptr<parsetree::ast::ASTManager> astManager
                 )
      : rootPackage{rootPackage} // , astManager(std::move(astManager))
  {}

  bool check() {}
};

} // namespace static_check
