#include "staticCheck/exprResolver.hpp"
#include <string>

namespace static_check {

using exprResolveType =
    std::variant<std::shared_ptr<parsetree::ast::ExprNode>,
                 std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>;

using previousType =
    std::variant<std::shared_ptr<parsetree::ast::ExprNode>,
                 std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>;

// Taken from HierarchyCheck
bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

// Taken from HierarchyCheck
bool isInterface(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(decl);
}

bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> child,
                  std::shared_ptr<parsetree::ast::AstNode> super) {
  if (!child || !super) {
    // std::cout << "Either child or super is a nullptr."
    return false;
  }
  if (!isClass(child)) {
    // std::cout << "Child class is not a class!\n";
    return false;
  }
  if (!isClass(super)) {
    // std::cout << "Super class is not a class!\n";
    return false;
  }
  auto childDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
  auto superDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(super);
  for (auto &superClass : childDecl->getSuperClasses()) {
    if (!superClass || !superClass->getResolvedDecl() ||
        !superClass->getResolvedDecl())
      continue;

    // Cast to class
    auto superClassDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        superClass->getResolvedDecl()->getAstNode());

    if (superClassDecl == superDecl)
      return true;

    if (isSuperClass(
            std::dynamic_pointer_cast<parsetree::ast::AstNode>(superClassDecl),
            super))
      return true;
  }
  return false;
}

void ExprResolver::resolve() {
  for (auto ast : astManager->getASTs()) {
    resolveAST(ast);
  }
}

void ExprResolver::resolveAST(std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");

  if (auto programDecl =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(node)) {
    BeginProgram(programDecl);
  }
  if (auto codeBody =
          std::dynamic_pointer_cast<parsetree::ast::CodeBody>(node)) {
    BeginContext(codeBody);
  }

  // only check Expr
  if (auto expr = std::dynamic_pointer_cast<parsetree::ast::Expr>(node)) {
    evaluate(expr);
  } else {
    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      resolveAST(child);
    }
  }
}

void ExprResolver::evaluate(std::shared_ptr<parsetree::ast::Expr> expr) {
  currentScope = expr->getScope();
  auto nodes = expr->getExprNodes();
  auto ret = evaluateList(nodes);
  auto resolved = resolveExprNode(ret);
  expr->setExprNodes(resolved);
  // expr->setExprNodes(resolveExprNode(ret));
  //   if (std::holds_alternative<
  //           std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(ret)) {
  //     auto retVec =
  //         std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(ret);
  //     for (auto node : retVec) {
  //       if (!node)
  //         continue;
  //       std::cout << "Expr: ";
  //       node->print(std::cout);
  //       std::cout << std::endl;
  //       resolveExprNode(node);
  //     }
  //   }
}

exprResolveType ExprResolver::evaluateList(
    std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) {
  return Evaluator<exprResolveType>::evaluateList(list);
}

std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::resolveExprNode(const exprResolveType node) {
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(node)) {
    return std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
        node);
  } else {
    // ExprNode
    auto expr = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(node);
    auto name = std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(expr);

    if (!name)
      return {expr};

    std::cout << "QualifiedName ";
    name->print(std::cout);
    std::cout << " has children num " << name->getChildren().size()
              << std::endl;

    std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;

    for (auto &n : name->getChildren()) {
      if (!n)
        continue;

      auto child = std::dynamic_pointer_cast<parsetree::ast::SimpleName>(n);
      if (!child)
        throw std::runtime_error("Node is null when traversing QualifiedName");

      std::cout << "this child: ";
      child->print(std::cout);
      std::cout << std::endl;

      if (child->getName() == "this") {
        if (!(child->isDeclResolved() && child->isTypeResolved())) {
          throw std::runtime_error("ThisNode not resolved in resolveExprNode");
        }
      }
      if (child->isDeclResolved() && child->isTypeResolved()) {
        auto resolved = recursiveReduce(child);
        ret.insert(ret.end(), resolved.begin(), resolved.end());
      } else {
        auto resolved = recursiveReduce(child);
        ret.insert(ret.end(), resolved.begin(), resolved.end());
      }
    }
    return ret;
  }
}

std::shared_ptr<parsetree::ast::Decl>
ExprResolver::lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                              std::string_view name) {
  auto condition = [name, this](std::shared_ptr<parsetree::ast::Decl> decl) {
    if (auto typedDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl);
        typedDecl) {
      bool scopeVisible =
          !currentScope || currentScope->canView(typedDecl->getScope());
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
    for (const auto decl : hierarchyChecker->getInheritedFields(classDecl)) {
      if (condition(decl.second)) {
        if (result)
          return nullptr; // Ambiguous case
        result = decl.second;
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
ExprResolver::reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                             std::shared_ptr<parsetree::ast::SimpleName> node) {
  if (auto decl = lookupNamedDecl(currentContext, node->getName())) {
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, varDecl);
      node->setValueType(parsetree::ast::ExprValue::ValueType::ExpressionName);
      node->resolveDeclAndType(varDecl, varDecl->getType());
      return varDecl;
    } else if (auto fieldDecl =
                   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
      node->setValueType(parsetree::ast::ExprValue::ValueType::ExpressionName);
      node->resolveDeclAndType(fieldDecl, fieldDecl->getType());
      return fieldDecl;
    }
  }
  if (auto ctxDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)) {
    if (auto parentCtx = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(
            ctxDecl->getParent())) {
      return reclassifyDecl(parentCtx, node);
    }
  }
  return nullptr;
}

std::shared_ptr<parsetree::ast::SimpleName> ExprResolver::resolveSimpleName(
    std::shared_ptr<parsetree::ast::SimpleName> expr) {
  auto decl = reclassifyDecl(currentContext, expr);
  if (decl != nullptr) {
    expr->setResolvedDecl(decl);
    return expr;
  }

  auto &context = typeLinker->getContext(currentProgram);
  auto import = context.find(expr->getName()) != context.end()
                    ? context[expr->getName()]
                    : nullptr;
  if (std::holds_alternative<nullptr_t>(import)) {
    throw std::runtime_error("No import for " + expr->getName());
  }
  if (auto decl = std::get_if<std::shared_ptr<Decl>>(&import)) {
    if (!decl) {
      throw std::runtime_error("Ambiguous import-on-demand conflict");
    }
    auto declNode = (*decl)->getAstNode();
    // data->reclassify(ExprName::Type::TypeName, decl);
    expr->setValueType(parsetree::ast::ExprValue::ValueType::TypeName);
    if (!expr->isTypeResolved()) {
      auto refType = std::make_shared<parsetree::ast::ReferenceType>(declNode);
      refType->setResolvedDecl(std::make_shared<Decl>(declNode));
      expr->resolveDeclAndType(declNode, refType);
    } else if (!expr->isDeclResolved()) {
      expr->setResolvedDecl(declNode);
    };

  } else if (auto pkg = std::get_if<std::shared_ptr<Package>>(&import)) {
    expr->setValueType(parsetree::ast::ExprValue::ValueType::PackageName);
    // if (!expr->isDeclResolved())
    //   expr->setResolvedDecl(pkg);
  }
}

std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::recursiveReduce(std::shared_ptr<parsetree::ast::ExprValue> node) {
  if (node->getValueType() !=
      parsetree::ast::ExprValue::ValueType::ExpressionName) {
    throw std::runtime_error("expected an expression name here");
  }

  // Base case
  if (!node->getPrev().has_value() ||
      (node->getPrev()->getValueType() !=
           parsetree::ast::ExprValue::ValueType::Other &&
       node->getPrev()->getValueType() !=
           parsetree::ast::ExprValue::ValueType::ExpressionName)) {
    return std::vector<std::shared_ptr<parsetree::ast::ExprNode>>{node};
  }

  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> list;
  if (auto prev = node->getPrev()) {
    list = recursiveReduce(prev);
  } else {
    list = std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
        node->getPrev().value());
  }
  list.push_back(node);
  if (!node->getOp()) {
    throw std::runtime_error("expected an operator here");
  }
  list.push_back(node->getOp());
  return list;
}

////////////////////////////////
// ExprNode Resolver
////////////////////////////////

exprResolveType
ExprResolver::mapValue(std::shared_ptr<parsetree::ast::ExprValue> &node) {
  return node;
}

exprResolveType ExprResolver::evalQualifiedName(
    std::shared_ptr<parsetree::ast::QualifiedName> &node) {
  // todo
  return resolveExprNode(node);
}

exprResolveType
ExprResolver::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                        const exprResolveType lhs, const exprResolveType rhs) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  auto lhsVec = resolveExprNode(lhs);
  auto rhsVec = resolveExprNode(rhs);
  ret.reserve(lhsVec.size() + rhsVec.size() + 1);
  ret.insert(ret.end(), lhsVec.begin(), lhsVec.end());
  ret.insert(ret.end(), rhsVec.begin(), rhsVec.end());
  ret.push_back(op);
  return ret;
}

exprResolveType
ExprResolver::evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                       const exprResolveType rhs) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  auto rhsVec = resolveExprNode(rhs);
  ret.reserve(rhsVec.size() + 1);
  ret.insert(ret.end(), rhsVec.begin(), rhsVec.end());
  ret.push_back(op);
  return ret;
}

exprResolveType
ExprResolver::evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                              const exprResolveType lhs,
                              const exprResolveType id) {
  previousType prev;
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs)) {
    prev =
        std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs);
  } else {
    auto node = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(lhs);
    auto nodeVal = std::dynamic_pointer_cast<parsetree::ast::ExprValue>(node);
    if (!nodeVal)
      throw std::runtime_error("cannot cast to ExprValue");
    if (nodeVal->getValueType() !=
        parsetree::ast::ExprValue::ValueType::Other) {
      prev = nodeVal;
    } else {
      if (auto name =
              std::dynamic_pointer_cast<parsetree::ast::SimpleName>(node)) {
        // todo
        throw std::runtime_error("breakpoint for simple name");
      } else if (auto name =
                     std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(
                         node)) {
        // todo
        throw std::runtime_error("breakpoint for qualified name");
      } else if (auto literal =
                     std::dynamic_pointer_cast<parsetree::ast::Literal>(node)) {
        if (!literal->isString())
          throw std::runtime_error("accessing field on non string literal");
        prev = std::vector<std::shared_ptr<parsetree::ast::ExprNode>>{literal};
      } else {
        throw std::runtime_error(
            "cannot resolve field access due to bad grammar");
      }
    }
  }

  if (auto exprNodePtr =
          std::get_if<std::shared_ptr<parsetree::ast::ExprNode>>(&prev)) {
    auto node =
        std::dynamic_pointer_cast<parsetree::ast::ExprValue>(*exprNodePtr);
    if (node->getValueType() !=
            parsetree::ast::ExprValue::ValueType::ExpressionName &&
        node->getValueType() !=
            parsetree::ast::ExprValue::ValueType::TypeName &&
        node->getValueType() !=
            parsetree::ast::ExprValue::ValueType::PackageName) {
      throw std::runtime_error("cannot resolve field access due to bad "
                               "grammar, not expr type or package namew");
    }
  }
}

exprResolveType ExprResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const exprResolveType method, const op_array &args) {
  // todo
  throw std::runtime_error("breakpoint for method invocation");
}

exprResolveType
ExprResolver::evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                            const exprResolveType object,
                            const op_array &args) {
  // todo
  throw std::runtime_error("breakpoint for new object");
}

exprResolveType
ExprResolver::evalNewArray(std::shared_ptr<parsetree::ast::ArrayCreation> &op,
                           const exprResolveType type,
                           const exprResolveType size) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(type)) {
    auto typeNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(type);
    ret.push_back(typeNode);
  } else {
    throw std::runtime_error("Incorrect grammar not type for new array");
  }
  auto sizeVec = resolveExprNode(size);
  ret.insert(ret.end(), sizeVec.begin(), sizeVec.end());
  ret.push_back(op);
  return ret;
}

exprResolveType
ExprResolver::evalArrayAccess(std::shared_ptr<parsetree::ast::ArrayAccess> &op,
                              const exprResolveType array,
                              const exprResolveType index) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  auto arrayVec = resolveExprNode(array);
  auto indexVec = resolveExprNode(index);
  ret.reserve(arrayVec.size() + indexVec.size() + 1);
  ret.insert(ret.end(), arrayVec.begin(), arrayVec.end());
  ret.insert(ret.end(), indexVec.begin(), indexVec.end());
  ret.push_back(op);
  return ret;
}

exprResolveType
ExprResolver::evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                       const exprResolveType type,
                       const exprResolveType value) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(type)) {
    auto typeNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(type);
    ret.push_back(typeNode);
  } else {
    throw std::runtime_error("Incorrect grammar not type for cast");
  }
  auto valueVec = resolveExprNode(value);
  ret.insert(ret.end(), valueVec.begin(), valueVec.end());
  ret.push_back(op);
  return ret;
}

bool ExprResolver::isAccessible(
    std::shared_ptr<parsetree::ast::Modifiers> mod,
    std::shared_ptr<parsetree::ast::CodeBody> parent) {
  // 6.6.1
  if (mod->isPublic())
    return true;
  // 6.6.2
  auto targetClass =
      std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(parent);
  if (auto curClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
          currentProgram->getBody())) {
    if (isSuperClass(targetClass, curClass))
      return true;
  }
  // same package
  if (auto otherProgram =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
              targetClass->getParent())) {
    if (currentProgram->getPackageName() == otherProgram->getPackageName())
      return true;
  }
  return false;
}

} // namespace static_check
