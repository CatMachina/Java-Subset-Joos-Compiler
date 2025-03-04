#include "staticCheck/exprResolver.hpp"
#include <string>

namespace static_check {

using ExprNodeList = std::vector<std::shared_ptr<parsetree::ast::ExprNode>>;
using exprResolveType =
    std::variant<std::shared_ptr<ExprNameLinked>,
                 std::shared_ptr<parsetree::ast::ExprNode>, ExprNodeList>;
using previousType =
    std::variant<std::shared_ptr<ExprNameLinked>, ExprNodeList>;

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
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 node)) {
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
          auto currentDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(
              currentProgram->getBody());
          if (!currentDecl) {
            throw std::runtime_error("current program body not Decl");
          }
          auto refType =
              std::make_shared<parsetree::ast::ReferenceType>(currentDecl);
          refType->setResolvedDecl(std::make_shared<Decl>(currentDecl));
          child->resolveDeclAndType(currentDecl, refType);
        }

        if (!(child->isDeclResolved() && child->isTypeResolved())) {
          throw std::runtime_error("ThisNode not resolved in resolveExprNode");
        }
        ret.push_back(child);
        continue;
      }

      auto childLinked = resolveSimpleName(std::make_shared<ExprNameLinked>(
          ExprNameLinked::ValueType::SingleAmbiguousName, child, nullptr));
      if (child->isDeclResolved() && child->isTypeResolved()) {
        auto resolved = recursiveReduce(childLinked);
        ret.insert(ret.end(), resolved.begin(), resolved.end());
      } else {
        auto resolved = recursiveReduce(childLinked);
        ret.insert(ret.end(), resolved.begin(), resolved.end());
      }
    }
    return ret;
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(node)) {
    auto expr = std::get<std::shared_ptr<ExprNameLinked>>(node);
    return recursiveReduce(expr);
  } else {
    throw std::runtime_error("should not happen");
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
      std::cout << "name: " << name << " got name: " << decl->getName()
                << " scopeVisible: " << scopeVisible
                << " canAccess: " << canAccess << std::endl;
      return decl->getName() == name && scopeVisible && canAccess;
    }
    return false;
  };

  std::cout << "lookupNamedDecl " << name << std::endl;
  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctx);
      classDecl) {
    std::cout << "classDecl " << classDecl->getName()
              << " checking inherited fields" << std::endl;
    std::shared_ptr<parsetree::ast::Decl> result = nullptr;
    for (const auto decl : hierarchyChecker->getInheritedFields(classDecl)) {
      std::cout << "inherited field " << decl.second->getName() << std::endl;
      if (condition(decl.second)) {
        std::cout << "found inherited field " << decl.second->getName()
                  << std::endl;
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
                             std::shared_ptr<ExprNameLinked> node) {
  std::cout << "reclassifyDecl" << std::endl;
  if (auto decl = lookupNamedDecl(currentContext, node->getNode()->getName())) {
    if (auto varDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, varDecl);
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(varDecl, varDecl->getType());
      return varDecl;
    } else if (auto fieldDecl =
                   std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(fieldDecl, fieldDecl->getType());
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

std::shared_ptr<ExprNameLinked>
ExprResolver::resolveSimpleName(std::shared_ptr<ExprNameLinked> expr) {
  if (expr->getNode()->isTypeResolved() && expr->getNode()->isDeclResolved()) {
    if (std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
            expr->getNode()->getResolvedDecl())) {
      expr->setValueType(ExprNameLinked::ValueType::ExpressionName);
      return expr;
    }
  }
  auto decl = reclassifyDecl(currentContext, expr);
  if (decl != nullptr) {
    expr->getNode()->setResolvedDecl(decl);
    return expr;
  }

  auto &context = typeLinker->getContext(currentProgram);
  auto import = context.find(expr->getNode()->getName()) != context.end()
                    ? context[expr->getNode()->getName()]
                    : nullptr;
  if (std::holds_alternative<nullptr_t>(import)) {
    for (auto pair : context) {
      std::cout << pair.first << " ";
    }
    std::cout << std::endl;
    throw std::runtime_error("No import for " + expr->getNode()->getName());
  }
  if (auto decl = std::get_if<std::shared_ptr<Decl>>(&import)) {
    if (!decl) {
      throw std::runtime_error("Ambiguous import-on-demand conflict");
    }
    auto declNode = (*decl)->getAstNode();
    // data->reclassify(ExprName::Type::TypeName, decl);
    expr->setValueType(ExprNameLinked::ValueType::TypeName);
    if (!expr->getNode()->isTypeResolved()) {
      auto refType = std::make_shared<parsetree::ast::ReferenceType>(declNode);
      refType->setResolvedDecl(std::make_shared<Decl>(declNode));
      expr->getNode()->resolveDeclAndType(declNode, refType);
    } else if (!expr->getNode()->isDeclResolved()) {
      expr->getNode()->setResolvedDecl(declNode);
    };

  } else if (auto pkg = std::get_if<std::shared_ptr<Package>>(&import)) {
    expr->setValueType(ExprNameLinked::ValueType::PackageName);
    // if (!expr->isDeclResolved())
    //   expr->setResolvedDecl(pkg);
  }
  return expr;
}

std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::recursiveReduce(std::shared_ptr<ExprNameLinked> node) {
  if (node->getValueType() != ExprNameLinked::ValueType::ExpressionName) {
    throw std::runtime_error("expected an expression name here");
  }

  // Base case
  if (!node->getPrev().has_value() ||
      (node->prevAsLinked() && node->prevAsLinked()->getValueType() !=
                                   ExprNameLinked::ValueType::ExpressionName)) {
    return std::vector<std::shared_ptr<parsetree::ast::ExprNode>>{
        node->getNode()};
  }

  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> list;
  if (auto prev = node->prevAsLinked()) {
    list = recursiveReduce(prev);
  } else {
    list = std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
        node->getPrev().value());
  }
  list.push_back(node->getNode());
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
  std::cout << "mapping value" << std::endl;
  return node;
}

exprResolveType ExprResolver::evalQualifiedName(
    std::shared_ptr<parsetree::ast::QualifiedName> &node) {
  std::cout << "evaluating qualified name" << std::endl;
  // todo
  return resolveExprNode(node);
}

exprResolveType
ExprResolver::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                        const exprResolveType lhs, const exprResolveType rhs) {
  std::cout << "evaluating binop" << std::endl;
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
  std::cout << "evaluating unop" << std::endl;
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
  std::cout << "evaluating field access" << std::endl;
  previousType prev;
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs)) {
    prev =
        std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs);
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 lhs)) {
    auto node = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(lhs);
    if (std::dynamic_pointer_cast<parsetree::ast::QualifiedName>(node)) {
      throw std::runtime_error("breakpoint for QualifiedName in field access");
    } else if (std::dynamic_pointer_cast<parsetree::ast::SimpleName>(node)) {
      throw std::runtime_error("breakpoint for SimpleName in field access");
    } else if (auto literal =
                   std::dynamic_pointer_cast<parsetree::ast::Literal>(node)) {
      if (!literal->isString())
        throw std::runtime_error("accessing field on non string literal");
      prev = ExprNodeList{literal};
    } else {
      throw std::runtime_error(
          "cannot resolve field access due to bad grammar");
    }
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(lhs)) {
    prev = std::get<std::shared_ptr<ExprNameLinked>>(lhs);
  } else {
    throw std::runtime_error("should not reach here");
  }

  auto exprNodePtr = std::get_if<std::shared_ptr<ExprNameLinked>>(&prev);
  if (exprNodePtr) {
    auto node = *exprNodePtr;
    if (node->getValueType() != ExprNameLinked::ValueType::ExpressionName &&
        node->getValueType() != ExprNameLinked::ValueType::TypeName &&
        node->getValueType() != ExprNameLinked::ValueType::PackageName) {
      throw std::runtime_error("cannot resolve field access due to bad "
                               "grammar, not expr type or package namew");
    }
  }

  auto exprNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(id);
  //  // Special case: If "Id" in Q . Id is a method name, then defer resolution

  // Now grab the id and cast it to the appropriate type
  auto fieldNode =
      std::dynamic_pointer_cast<parsetree::ast::SimpleName>(exprNode);
  if (!fieldNode)
    throw std::runtime_error("Malformed node. Expected SimpleName here.");
  // Allocate a new node as the member access to represent "Id" in Lhs . Id
  auto newPrev = std::make_shared<ExprNameLinked>(
      ExprNameLinked::ValueType::SingleAmbiguousName, fieldNode, op);
  newPrev->setPrev(prev);
  // And we can build the reduced expression now
  // 1. If the previous node is a wrapper, then newPrev can be anything
  // 2. If the previous is a list, then newPrev must be ExpressionName
  //    FIXME: Is this true? What about (Class).Field?
  if (exprNodePtr) {
    switch ((*exprNodePtr)->getValueType()) {
    case ExprNameLinked::ValueType::ExpressionName:
      // resolveFieldAccess(newPrev);
      break;
    case ExprNameLinked::ValueType::TypeName:
      // resolveTypeAccess(newPrev);
      break;
    case ExprNameLinked::ValueType::PackageName:
      // resolvePackageAccess(newPrev);
      break;
    default:
      throw std::runtime_error("should not reach here");
    }
  } else {
    // resolveFieldAccess(newPrev);
  }
  return newPrev;
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
  std::cout << "evaluating for new array" << std::endl;
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
  std::cout << "evaluating for array access" << std::endl;
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
  std::cout << "evaluating for cast" << std::endl;
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
  std::cout << "checking accessibility" << std::endl;
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
