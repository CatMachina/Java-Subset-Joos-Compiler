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

bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> super,
                  std::shared_ptr<parsetree::ast::AstNode> child) {
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
    auto program = std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(ast);
    if (!program)
      throw std::runtime_error("Not AST");
    typeLinker->setCurrentProgram(program);
    resolveAST(ast);
    std::cout << "-------- an AST resolved\n";
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

  std::cout << "-------- start type resolution\n";
  // FIXME: rm this when type linker works
  for (auto &node : resolved) {
    if (auto typeNode =
            std::dynamic_pointer_cast<parsetree::ast::TypeNode>(node)) {
      if (!(typeNode->getType()->isResolved())) {
        auto type = typeNode->getType();
        if (!type)
          throw std::runtime_error("TypeNode Type cannot be null");
        if (auto array =
                std::dynamic_pointer_cast<parsetree::ast::ArrayType>(type)) {
          typeLinker->resolveType(array->getElementType());
        } else {
          typeLinker->resolveType(type);
        }
        if (!(type->isResolved())) {
          type->print(std::cout);
          throw std::runtime_error("Type still not resolved after resolveType "
                                   "in ExprResolver::evaluate");
        }
      }
    }
  }
  expr->setExprNodes(resolved);
  typeResolver->EvalList(resolved);
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
  std::cout << "resolving expr node" << std::endl;
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(node)) {
    std::cout << "resolving expr node vector" << std::endl;
    return std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
        node);
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 node)) {
    // ExprNode
    std::cout << "resolving expr node expr node" << std::endl;
    auto expr = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(node);
    auto name = std::dynamic_pointer_cast<parsetree::ast::MemberName>(expr);
    auto thisNode = std::dynamic_pointer_cast<parsetree::ast::ThisNode>(expr);

    auto currentDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(
        currentProgram->getBody());
    if (!currentDecl) {
      throw std::runtime_error("current program body not Decl");
    }

    if (thisNode) {
      if (thisNode->isTypeResolved()) {
        thisNode->setResolvedDecl(currentDecl);
      } else {
        auto refType =
            std::make_shared<parsetree::ast::ReferenceType>(currentDecl);
        refType->setResolvedDecl(std::make_shared<Decl>(currentDecl));
        if (!refType->getResolvedDecl())
          throw std::runtime_error("resolved decl not sets");
        thisNode->resolveDeclAndType(currentDecl, refType);
      }
    }

    if (!name)
      return {expr};

    std::cout << "MemberName ";
    name->print(std::cout);

    auto childLinked = resolveName(name);

    return recursiveReduce(childLinked);
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(node)) {
    std::cout << "resolving expr node expr name linked" << std::endl;
    auto expr = std::get<std::shared_ptr<ExprNameLinked>>(node);
    return recursiveReduce(expr);
  } else {
    throw std::runtime_error("should not happen");
  }
}

std::shared_ptr<parsetree::ast::Decl>
ExprResolver::lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                              std::string name) {
  auto condition = [name, this](std::shared_ptr<parsetree::ast::Decl> decl) {
    if (auto typedDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl);
        typedDecl) {
      bool sameName = decl->getName() == name;
      bool sameContext = decl->getParent() == currentContext;
      bool checkScope =
          !(std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) &&
          currentScope;
      bool scopeVisible = true;
      if (sameContext && checkScope)
        scopeVisible = currentScope->canView(typedDecl->getScope());
      bool canAccess = true;
      if (auto fieldDecl =
              std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
        canAccess =
            isAccessible(fieldDecl->getModifiers(), fieldDecl->getParent());
      }
      std::cout << "name: " << name << " got name: " << decl->getName()
                << " scopeVisible: " << scopeVisible
                << " canAccess: " << canAccess << std::endl;
      return sameName && scopeVisible && canAccess;
    }
    return false;
  };

  std::cout << "lookupNamedDecl " << name << std::endl;
  auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctx);
  if (!classDecl)
    std::cout << "No current class?" << std::endl;
  auto methodDecl = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(ctx);
  if (methodDecl)
    std::cout << "We are in a method" << std::endl;

  if (classDecl && (ctx != astManager->java_lang.Array)) {
    // // Search in the declared set
    std::cout << "classDecl " << classDecl->getName()
              << " checking declared fields" << std::endl;
    auto declaredFields = classDecl->getFields();
    for (const auto decl : declaredFields) {
      if (condition(decl)) {
        std::cout << "found declared field " << decl->getName() << std::endl;
        return decl;
      }
    }

    // Search in the inherit set
    std::cout << "classDecl " << classDecl->getName()
              << " checking inherited fields" << std::endl;
    std::shared_ptr<parsetree::ast::Decl> result = nullptr;
    // TODO: this recomputes inherited field every time...
    auto inheritedFields = hierarchyChecker->getInheritedFields(classDecl);
    for (const auto decl : inheritedFields) {
      std::cout << "inherited field " << decl.second->getName() << std::endl;
      if (condition(decl.second)) {
        std::cout << "found inherited field " << decl.second->getName()
                  << std::endl;
        if (result)
          return nullptr; // Ambiguous case
        result = decl.second;
      }
    }
    if (result) {
      return result;
    }
  } else {
    std::cout << "not in a class, search for decls in context:" << std::endl;
    ctx->print(std::cout);
    std::cout << std::endl;
    // Search for the unique local variable
    for (auto decl : ctx->getDecls()) {
      std::cout << "checking decl " << decl->getName() << std::endl;
      if (condition(decl))
        return decl;
    }
  }
  return nullptr;
}

std::shared_ptr<parsetree::ast::Decl>
ExprResolver::reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                             std::shared_ptr<ExprNameLinked> node) {
  std::cout << "reclassifyDecl" << std::endl;
  if (auto decl = lookupNamedDecl(ctx, node->getNode()->getName())) {
    if (auto fieldDecl =
            std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, fieldDecl);
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(fieldDecl, fieldDecl->getType());
      return fieldDecl;
    } else if (auto varDecl =
                   std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      // data->reclassify(ExprName::Type::ExpressionName, varDecl);
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(varDecl, varDecl->getType());
      return varDecl;
    }
  }
  if (auto ctxDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)) {
    if (auto parentCtx = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(
            ctxDecl->getParent())) {
      std::cout << "reclassifyDecl goto codebody parent" << std::endl;
      return reclassifyDecl(parentCtx, node);
    }
  }
  return nullptr;
}

std::shared_ptr<ExprNameLinked>
ExprResolver::resolveMemberName(std::shared_ptr<ExprNameLinked> expr) {
  std::cout << "resolveMemberName" << std::endl;
  if (expr->getNode()->isTypeResolved() && expr->getNode()->isDeclResolved()) {
    if (std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
            expr->getNode()->getResolvedDecl())) {
      expr->setValueType(ExprNameLinked::ValueType::ExpressionName);
      return expr;
    }
  }
  auto decl = reclassifyDecl(currentContext, expr);
  std::cout << "resolveMemberName reclassfy decl done" << std::endl;
  if (decl != nullptr) {
    std::cout << "resolveMemberName found decl" << std::endl;
    expr->getNode()->setResolvedDecl(decl);
    return expr;
  }

  std::cout << "resolveMemberName resolve import for "
            << expr->getNode()->getName() << std::endl;
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
    expr->setPackage(*pkg);
    // if (!expr->isDeclResolved())
    //   expr->setResolvedDecl(pkg);
  }
  return expr;
}

std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::recursiveReduce(std::shared_ptr<ExprNameLinked> node) {
  std::cout << "recursiveReduce" << std::endl;
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
  std::cout << "exprResolver mapValue" << std::endl;
  return node;
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
    std::cout << "field access on vector, with: " << std::endl;
    // FIXME: hack for failed type linker
    for (auto node :
         std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
             lhs)) {
      if (auto typeNode =
              std::dynamic_pointer_cast<parsetree::ast::TypeNode>(node)) {
        if (auto rType =
                std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                    typeNode->getType())) {
          // FIXME: rm when type linker is fixed
          if (!(rType->isResolved())) {
            std::cout << "temp hack to resolve type again" << std::endl;
            typeLinker->resolveType(rType, currentProgram);
          }
        }
      }
      node->print(std::cout);
    }
    std::cout << std::endl;
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 lhs)) {
    auto node = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(lhs);
    if (auto memberName =
            std::dynamic_pointer_cast<parsetree::ast::MemberName>(node)) {
      std::cout << "field access on member name" << std::endl;
      prev = resolveName(memberName);
    } else if (auto thisNode =
                   std::dynamic_pointer_cast<parsetree::ast::ThisNode>(node)) {
      std::cout << "field access on this node" << std::endl;
      prev = resolveExprNode(thisNode);
    } else if (auto literal =
                   std::dynamic_pointer_cast<parsetree::ast::Literal>(node)) {
      std::cout << "field access on literal" << std::endl;
      if (!literal->isString())
        throw std::runtime_error("accessing field on non string literal");
      prev = ExprNodeList{literal};
    } else {
      throw std::runtime_error(
          "cannot resolve field access due to bad grammar");
    }
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(lhs)) {
    std::cout << "field access on expr name linked" << std::endl;
    prev = std::get<std::shared_ptr<ExprNameLinked>>(lhs);
  } else {
    throw std::runtime_error("should not reach here");
  }

  auto exprNodePtr = std::get_if<std::shared_ptr<ExprNameLinked>>(&prev);
  if (exprNodePtr) {
    auto node = *exprNodePtr;
    std::cout << "prev is expr name linked: " << node->getNode()->getName()
              << " is ";
    node->getNode()->print(std::cout);
    std::cout << " with type "
              << std::string(magic_enum::enum_name(node->getValueType()))
              << std::endl;
    if (node->getValueType() != ExprNameLinked::ValueType::ExpressionName &&
        node->getValueType() != ExprNameLinked::ValueType::TypeName &&
        node->getValueType() != ExprNameLinked::ValueType::PackageName) {
      throw std::runtime_error("cannot resolve field access due to bad "
                               "grammar, not expr type or package namew");
    }
  }

  auto exprNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(id);
  //  // Special case: If "Id" in Q . Id is a method name, then defer resolution
  if (auto methodNode =
          std::dynamic_pointer_cast<parsetree::ast::MethodName>(exprNode)) {
    auto newPrev = std::make_shared<ExprNameLinked>(
        ExprNameLinked::ValueType::MethodName, methodNode, op);
    newPrev->setPrev(prev);
    return newPrev;
  }

  // Now grab the id and cast it to the appropriate type
  std::cout << "grabbing id" << std::endl;
  auto fieldNode =
      std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode);
  if (!fieldNode)
    throw std::runtime_error("Bad node. Expected MemberName here.");
  // Allocate a new node as the member access to represent "Id" in Lhs . Id
  auto newPrev = std::make_shared<ExprNameLinked>(
      ExprNameLinked::ValueType::SingleAmbiguousName, fieldNode, op);
  newPrev->setPrev(prev);

  std::cout << "newPrev " << newPrev->getNode()->getName() << " is ";
  newPrev->getNode()->print(std::cout);
  std::cout << std::endl;

  std::cout << "building reduced expression" << std::endl;
  // And we can build the reduced expression now
  // 1. If the previous node is a wrapper, then newPrev can be anything
  // 2. If the previous is a list, then newPrev must be ExpressionName
  //    FIXME: Is this true? What about (Class).Field?
  if (exprNodePtr) {
    switch ((*exprNodePtr)->getValueType()) {
    case ExprNameLinked::ValueType::ExpressionName:
      resolveFieldAccess(newPrev);
      break;
    case ExprNameLinked::ValueType::TypeName:
      resolveTypeAccess(newPrev);
      break;
    case ExprNameLinked::ValueType::PackageName:
      resolvePackageAccess(newPrev);
      break;
    default:
      throw std::runtime_error("should not reach here");
    }
  } else {
    resolveFieldAccess(newPrev);
  }
  return newPrev;
}

exprResolveType ExprResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const exprResolveType method, const op_array &args) {
  std::cout << "evaluating for method invocation" << std::endl;
  // incomplete resolved method name
  std::shared_ptr<ExprNameLinked> unresolved = nullptr;
  // Single name method can never be static, and static methods can
  // never be single name methods!
  bool isSingleNameMethod = false;

  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
          method)) {
    auto expr = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(method);
    auto name = std::dynamic_pointer_cast<parsetree::ast::MethodName>(expr);
    if (!name)
      throw std::runtime_error("Bad node. Expected MethodName here.");
    unresolved = resolveName(name);
    isSingleNameMethod = true;
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(method)) {
    unresolved = std::get<std::shared_ptr<ExprNameLinked>>(method);
  } else {
    throw std::runtime_error("Bad Method Call Expression");
  }

  // Resolve the array of arguments
  std::cout << "resolving arguments" << std::endl;
  std::vector<std::shared_ptr<parsetree::ast::Type>> argTypes;
  ExprNodeList arglist;
  for (auto it = args.rbegin(); it != args.rend(); ++it) {
    auto &arg = *it;
    auto tmplist = resolveExprNode(arg);
    // FIXME: rm this when type linker works
    for (auto &node : tmplist) {
      if (auto typeNode =
              std::dynamic_pointer_cast<parsetree::ast::TypeNode>(node)) {
        if (auto refType = std::dynamic_pointer_cast<parsetree::ast::Type>(
                typeNode->getType())) {
          if (!(refType->isResolved())) {
            if (auto arrayType =
                    std::dynamic_pointer_cast<parsetree::ast::ArrayType>(
                        refType)) {
              typeLinker->resolveType(arrayType->getElementType());
            }
            typeLinker->resolveType(refType);
          }
        }
      }
    }
    argTypes.push_back(typeResolver->EvalList(tmplist));
    arglist.insert(arglist.end(), tmplist.begin(), tmplist.end());
  }

  // Begin resolution of the method call
  std::cout << "resolving method call" << std::endl;
  auto ctx = getMethodParent(unresolved);
  auto methodDecl = resolveMethodOverload(ctx, unresolved->getNode()->getName(),
                                          argTypes, false);

  // Check if the method call is legal
  std::cout << "checking method call" << std::endl;
  if (!isAccessible(methodDecl->getModifiers(), methodDecl->getParent())) {
    throw std::runtime_error("method call to non-accessible method: " +
                             methodDecl->getName());
  }

  // static method call check
  std::cout << "checking static method call" << std::endl;
  if (isSingleNameMethod && methodDecl->getModifiers()->isStatic()) {
    throw std::runtime_error(
        "attempted to call static method using single name: " +
        methodDecl->getName());
  }

  // check if the previous a type name
  std::cout << "check if the previous a type name" << std::endl;
  if (unresolved->getPrev().has_value() && unresolved->prevAsLinked() &&
      unresolved->prevAsLinked()->getValueType() ==
          ExprNameLinked::ValueType::TypeName) {
    // we are calling a static method, check
    if (!methodDecl->getModifiers()->isStatic()) {
      throw std::runtime_error("attempted to call non-static method: " +
                               methodDecl->getName());
    }
  }

  // Reclassify
  unresolved->setValueType(ExprNameLinked::ValueType::ExpressionName);
  unresolved->getNode()->setResolvedDecl(methodDecl);

  // Once unresolved has been resolved, we can build the expression list
  ExprNodeList list;
  auto reduced = recursiveReduce(unresolved);
  list.insert(list.end(), reduced.begin(), reduced.end());
  list.insert(list.end(), arglist.begin(), arglist.end());
  list.push_back(op);
  return list;
}

exprResolveType
ExprResolver::evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                            const exprResolveType object,
                            const op_array &args) {
  std::cout << "evaluating for new object" << std::endl;
  // Get the type we are instantiating
  std::shared_ptr<parsetree::ast::TypeNode> expr = nullptr;
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
          object)) {
    std::cout << "new object object is ExprNode" << std::endl;
    expr = std::dynamic_pointer_cast<parsetree::ast::TypeNode>(
        std::get<std::shared_ptr<parsetree::ast::ExprNode>>(object));
    if (!expr)
      throw std::runtime_error("Bad node. Expected TypeNode here.");
  } else {
    throw std::runtime_error("Bad grammar, object must be an atomic ExprNode*");
  }

  // Resolve the array of arguments
  std::vector<std::shared_ptr<parsetree::ast::Type>> argType;
  ExprNodeList arglist;
  for (auto it = args.rbegin(); it != args.rend(); ++it) {
    auto arg = *it;
    auto tmplist = resolveExprNode(arg);
    std::cout << "tmplist size: " << tmplist.size() << std::endl;
    // FIXME: rm this when type linker works
    for (auto &node : tmplist) {
      if (auto typeNode =
              std::dynamic_pointer_cast<parsetree::ast::TypeNode>(node)) {
        if (auto refType = std::dynamic_pointer_cast<parsetree::ast::Type>(
                typeNode->getType())) {
          if (!(refType->isResolved())) {
            if (auto arrayType =
                    std::dynamic_pointer_cast<parsetree::ast::ArrayType>(
                        refType)) {
              typeLinker->resolveType(arrayType->getElementType());
            }
            typeLinker->resolveType(refType);
          }
        }
      }
    }

    std::cout << "tmplist: ";
    for (auto &node : tmplist) {
      node->print(std::cout);
    }
    std::cout << std::endl;
    argType.push_back(typeResolver->EvalList(tmplist));
    std::cout << "EvalList on tmplist finished" << std::endl;
    arglist.insert(arglist.end(), tmplist.begin(), tmplist.end());
  }

  // Check if the type is abstract
  std::cout << "checking if type is abstract" << std::endl;
  auto rType =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(expr->getType());
  if (!rType) {
    throw std::runtime_error("New object not reference type");
  }
  if (std::dynamic_pointer_cast<parsetree::ast::UnresolvedType>(rType)) {
    std::cout << "rType is unresolved type" << std::endl;
  }
  // FIXME: rm when type linker is working
  if (!rType->isResolved()) {
    if (auto array =
            std::dynamic_pointer_cast<parsetree::ast::ArrayType>(rType)) {
      typeLinker->resolveType(array->getElementType());
    } else {
      typeLinker->resolveType(rType, currentProgram);
    }
  }
  std::cout << "rType: " << rType->toString()
            << " is resolved? :  " << rType->isResolved() << std::endl;
  auto typeAsDecl = rType->getAsDecl();
  if (!typeAsDecl)
    typeAsDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(
        rType->getResolvedDecl()->getAstNode());
  auto ctx = typeAsDecl->asCodeBody();
  if (auto classDecl =
          std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctx)) {
    if (classDecl->getModifiers()->isAbstract()) {
      throw std::runtime_error("attempted to instantiate abstract class: " +
                               classDecl->getName());
    }
  }

  // Begin resolution of the method call
  std::cout << "begin resolution of method call" << std::endl;
  auto methodDecl = resolveMethodOverload(ctx, "", argType, true);
  // override
  expr->setResolvedDecl(methodDecl);

  // Once op has been resolved, we can build the expression list
  ExprNodeList list;
  list.push_back(expr);
  list.insert(list.end(), arglist.begin(), arglist.end());
  list.push_back(op);

  std::cout << "list: ";
  for (auto &arg : list) {
    arg->print(std::cout);
  }
  return list;
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

exprResolveType
ExprResolver::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                             const exprResolveType lhs,
                             const exprResolveType rhs) {
  std::cout << "evaluating for assignment" << std::endl;
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;
  auto lhsVec = resolveExprNode(lhs);
  auto rhsVec = resolveExprNode(rhs);
  ret.reserve(lhsVec.size() + rhsVec.size() + 1);
  ret.insert(ret.end(), lhsVec.begin(), lhsVec.end());
  ret.insert(ret.end(), rhsVec.begin(), rhsVec.end());
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

//// More Helper

std::shared_ptr<parsetree::ast::Decl> ExprNameLinked::prevAsDecl(
    std::shared_ptr<TypeResolver> TR,
    std::shared_ptr<parsetree::ast::ASTManager> astManager,
    std::shared_ptr<TypeLinker> typeLinker) const {
  std::cout << "prevAsDecl" << std::endl;
  if (auto p = prevAsLinked())
    return p->getNode()->getResolvedDecl();
  std::cout << "prevAsDecl complex case" << std::endl;
  // Complex case, the previous node is an expression list
  if (!prev_.has_value())
    throw std::runtime_error("No previous value");
  if (!std::holds_alternative<ExprNodeList>(prev_.value()))
    throw std::runtime_error("Previous Not an expression list");
  ExprNodeList prevList = std::get<ExprNodeList>(prev_.value());
  std::cout << "EvalList started in prevAsDecl" << std::endl;
  // FIXME: rm this when type linker works
  for (auto &node : prevList) {
    if (auto typeNode =
            std::dynamic_pointer_cast<parsetree::ast::TypeNode>(node)) {
      if (auto refType = std::dynamic_pointer_cast<parsetree::ast::Type>(
              typeNode->getType())) {
        if (!(refType->isResolved())) {
          std::cout << "resolving unresolved type";
          refType->print(std::cout);
          std::cout << std::endl;
          if (auto arrayType =
                  std::dynamic_pointer_cast<parsetree::ast::ArrayType>(
                      refType)) {
            typeLinker->resolveType(arrayType->getElementType());
          }
          typeLinker->resolveType(refType);
        }
      }
    }
  }
  auto type = TR->EvalList(prevList);
  std::cout << "EvalList finished in prevAsDecl" << std::endl;
  return std::dynamic_pointer_cast<parsetree::ast::Decl>(
      GetTypeAsDecl(type, astManager));
}

void ExprResolver::resolveFieldAccess(std::shared_ptr<ExprNameLinked> access) {
  std::cout << "resolveFieldAccess" << std::endl;
  // First, verify invariants if access is a field access
  if (access->getValueType() != ExprNameLinked::ValueType::SingleAmbiguousName)
    throw std::runtime_error("Not a ambiguous name, Not a field access");
  if (auto p = access->prevAsLinked()) {
    if (p->getValueType() != ExprNameLinked::ValueType::ExpressionName)
      throw std::runtime_error("Not an expression name, Not a field access");
  }

  // Next, fetch the type or declaration
  std::cout << "fetch the type or declaration" << std::endl;
  auto name = access->getNode()->getName();
  auto typeOrDecl = access->prevAsDecl(typeResolver, astManager, typeLinker);
  std::shared_ptr<parsetree::ast::CodeBody> refType = nullptr;
  if (access->prevAsLinked()) {
    std::cout << "prev a wrapper, resolve type" << std::endl;
    // If the previous node is a wrapper, then we resolve the type
    auto decl = typeOrDecl;
    auto typeddecl = std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl);
    if (!typeddecl) {
      throw std::runtime_error("field access to non-typed declaration" +
                               decl->getName());
    }
    auto type = typeddecl->getType();
    if (!type) {
      throw std::runtime_error("field access to void-typed declaration: " +
                               decl->getName());
    }
    auto typeAsDecl = GetTypeAsDecl(type, astManager);
    refType = typeAsDecl->asCodeBody();
    if (!refType) {
      throw std::runtime_error("field access to void-typed declaration: " +
                               decl->getName());
    }
  } else {
    if (!typeOrDecl)
      throw std::runtime_error("typeOrDecl is null");
    auto typeOrDeclDecl =
        std::dynamic_pointer_cast<parsetree::ast::Decl>(typeOrDecl);
    if (!typeOrDeclDecl) {
      throw std::runtime_error("Expected decl here");
    }
    refType = typeOrDeclDecl->asCodeBody();
    if (!refType) {
      throw std::runtime_error("Expected non-null type here");
    }
  }
  // Now we check if "name" is a field of "decl"
  std::cout << "checking if name is in decl" << std::endl;
  auto field = lookupNamedDecl(refType, name);
  if (!field) {
    throw std::runtime_error("field access failed for " + name);
  }
  if (!std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field) &&
      !std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(field)) {
    throw std::runtime_error(
        "field shouldn't be anything other than field or method");
  }

  std::shared_ptr<parsetree::ast::Type> fieldType = nullptr;
  if (auto x = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field))
    fieldType = x->getType();
  access->setValueType(ExprNameLinked::ValueType::ExpressionName);
  access->getNode()->resolveDeclAndType(field, fieldType);
}

void ExprResolver::resolveTypeAccess(std::shared_ptr<ExprNameLinked> access) {
  std::cout << "resolveTypeAccess" << std::endl;
  // First, verify invariants if access is a type access
  if (access->getValueType() !=
      ExprNameLinked::ValueType::SingleAmbiguousName) {
    throw std::runtime_error("Not a ambiguous name, Not a type access");
  }
  if (auto p = access->prevAsLinked()) {
    if (p->getValueType() != ExprNameLinked::ValueType::TypeName)
      throw std::runtime_error("Not a type name, Not a type access");
  }
  // Next, fetch the type or declaration
  auto name = access->getNode()->getName();
  auto typeOrDecl = access->prevAsDecl(typeResolver, astManager, typeLinker);
  // We note this must be a class type or we have a type error
  auto type = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(typeOrDecl);
  if (!type) {
    throw std::runtime_error("static member access to non-class type for " +
                             name);
  }
  // Now we check if "name" is a field of "decl".
  std::cout << "checking if name is in decl" << std::endl;
  auto field = lookupNamedDecl(type, name);
  if (!field) {
    throw std::runtime_error("type access failed for " + name);
  }
  // With the additional constraint that the field must be static
  std::cout << "checking if field is static" << std::endl;
  std::shared_ptr<parsetree::ast::Modifiers> mods;
  if (auto fieldDecl =
          std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field)) {
    mods = fieldDecl->getModifiers();
  } else if (auto methodDecl =
                 std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(field)) {
    mods = methodDecl->getModifiers();
  } else {
    throw std::runtime_error(
        "field shouldn't be anything other than field or method");
  }
  if (!mods->isStatic()) {
    throw std::runtime_error("field attempt to access non-static member: " +
                             field->getName());
  }
  // field access is valid
  std::shared_ptr<parsetree::ast::Type> fieldType = nullptr;
  if (auto x = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field))
    fieldType = x->getType();
  access->setValueType(ExprNameLinked::ValueType::ExpressionName);
  access->getNode()->resolveDeclAndType(field, fieldType);
  access->setPrev(std::nullopt);
}

void ExprResolver::resolvePackageAccess(
    std::shared_ptr<ExprNameLinked> access) const {
  std::cout << "resolvePackageAccess" << std::endl;
  // First, verify invariants if access is a package access
  auto prev = access->prevAsLinked();
  if (!prev)
    throw std::runtime_error("Expected non-null previous node");
  if (access->getValueType() !=
      ExprNameLinked::ValueType::SingleAmbiguousName) {
    throw std::runtime_error("Not a ambiguous name, Not a package access");
  }
  if (prev->getValueType() != ExprNameLinked::ValueType::PackageName)
    throw std::runtime_error("Not a package name, Not a package access");
  // Now we can get the "pkg" and "name" to resolve against
  auto pkg = prev->getPackage();
  auto name = access->getNode()->getName();
  if (!pkg)
    throw std::runtime_error("Expected non-null package here");
  // Now we check if "name" is a package of "pkg"
  auto subpkg = pkg->getChild(name);
  if (std::holds_alternative<nullptr_t>(subpkg)) {
    throw std::runtime_error("package access to unknown member: " + name);
  }
  // reclassify
  if (std::holds_alternative<std::shared_ptr<Decl>>(subpkg)) {
    auto pkgdecl = std::get<std::shared_ptr<Decl>>(subpkg);
    access->setValueType(ExprNameLinked::ValueType::TypeName);
    auto pkgType =
        std::make_shared<parsetree::ast::ReferenceType>(pkgdecl->getAstNode());
    pkgType->setResolvedDecl(pkgdecl);
    access->getNode()->resolveDeclAndType(pkgdecl->getAstNode(), pkgType);
  } else {
    access->setValueType(ExprNameLinked::ValueType::PackageName);
    access->setPackage(std::get<std::shared_ptr<Package>>(subpkg));
  }
  access->setPrev(std::nullopt);
}

std::shared_ptr<parsetree::ast::CodeBody>
ExprResolver::getMethodParent(std::shared_ptr<ExprNameLinked> method) const {
  std::cout << "getMethodParent" << std::endl;
  if (method->getValueType() != ExprNameLinked::ValueType::MethodName) {
    throw std::runtime_error("Not a method name, cannot get parent");
  }
  // If there's no previous, use the current context
  if (!method->getPrev().has_value())
    return currentProgram->getBody();
  auto declOrType = method->prevAsDecl(typeResolver, astManager, typeLinker);
  // auto ty = std::dynamic_pointer_cast<parsetree::ast::CodeBody>(declOrType);
  auto ty = declOrType->asCodeBody();
  if (method->prevAsLinked() && !ty) {
    auto typedDecl =
        std::dynamic_pointer_cast<parsetree::ast::VarDecl>(declOrType);
    if (!typedDecl) {
      throw std::runtime_error("method call to non-typed declaration: " +
                               declOrType->getName());
    }
    auto type = typedDecl->getType();
    if (!type) {
      throw std::runtime_error("method call to non-typed declaration: " +
                               declOrType->getName());
    }
    auto typeAsDecl = GetTypeAsDecl(type, astManager);
    ty = typeAsDecl->asCodeBody();
    if (!ty) {
      throw std::runtime_error("method call to non-reference type: " +
                               declOrType->getName());
    }
  }
  if (!ty)
    throw std::runtime_error("Expected non-null type");
  return ty;
}

std::shared_ptr<parsetree::ast::MethodDecl> ExprResolver::resolveMethodOverload(
    std::shared_ptr<parsetree::ast::CodeBody> ctx, std::string name,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &argTypes,
    bool isConstructor) {
  std::cout << "resolveMethodOverload" << std::endl;
  // Set the name to the constructor name if isConstructor is true
  if (isConstructor)
    name = ctx->asDecl()->getName();
  // 15.12.2.1
  std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> candidates;
  if (isConstructor) {
    auto program = std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(
        std::dynamic_pointer_cast<parsetree::ast::Decl>(ctx)->getParent());
    // Only grab the constructors of this type
    for (auto decl : ctx->getDecls()) {
      auto ctor = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(decl);
      if (!ctor)
        continue;
      if (!ctor->isConstructor())
        continue;
      if (ctor->getParams().size() != argTypes.size())
        continue;
      std::cout << "ctor: " << ctor->getName() << std::endl;
      if (!isAccessible(ctor->getModifiers(), ctor->getParent()))
        continue;
      // If the ctor is protected, it must be in the same PACKAGE
      if (ctor->getModifiers()->isProtected()) {
        if (program->getPackageName() != currentProgram->getPackageName()) {
          throw std::runtime_error(
              "attempted to access protected constructor from different "
              "package");
        }
      }
      if (areParameterTypesApplicable(ctor, argTypes))
        candidates.push_back(ctor);
    }
  } else {
    // Search the current class and all superclasses
    // 1. Parameters number match
    // 2. Parameters are convertible
    // 3. Method is accessible
    auto ctxDecl = ctx->asDecl();
    auto result = hierarchyChecker->getAllInheritedMethods(ctxDecl);
    if (!result.success)
      throw std::runtime_error("Failed to get inherited methods");

    // should we also loop through abstract methods?
    std::unordered_map<std::string, std::shared_ptr<parsetree::ast::MethodDecl>>
        allMethods = result.methods;
    allMethods.insert(result.abstractMethods.begin(),
                      result.abstractMethods.end());
    for (auto pair : allMethods) {
      auto decl = pair.second;
      if (!decl)
        continue;
      if (decl->getParams().size() != argTypes.size())
        continue;
      if (decl->getName() != name)
        continue;
      if (!areParameterTypesApplicable(decl, argTypes))
        continue;
      if (!isAccessible(decl->getModifiers(), decl->getParent()))
        continue;
      candidates.push_back(decl);
    }
  }
  if (candidates.size() == 0) {
    if (isConstructor) {
      throw std::runtime_error("no constructor found for " + std::string(name));
    } else {
      throw std::runtime_error("no method found for " + std::string(name));
    }
  }
  if (candidates.size() == 1)
    return candidates[0];

  // 15.12.2.2 Choose the Most Specific Method
  std::shared_ptr<parsetree::ast::MethodDecl> mostSpecific = nullptr;
  std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> maxSpecific;
  // Grab the (not necessarily unique) minimum
  for (auto cur : candidates) {
    if (!mostSpecific) {
      mostSpecific = cur;
      continue;
    }
    // cur < minimum?
    if (isMethodMoreSpecific(cur, mostSpecific)) {
      mostSpecific = cur;
    }
  }
  // Now grab all the maximally specific methods
  for (auto cur : candidates) {
    if (cur == mostSpecific) {
      maxSpecific.push_back(cur);
    } else if (isMethodMoreSpecific(cur, mostSpecific) &&
               isMethodMoreSpecific(mostSpecific, cur)) {
      maxSpecific.push_back(cur);
    }
  }
  // If there's only one maximally specific method, return it
  if (maxSpecific.size() == 1)
    return maxSpecific[0];
  // There are more conditions i.e., abstract...
  // Otherwise, we have an ambiguity error
  throw std::runtime_error("ambiguous method found for " + std::string(name));
}

bool ExprResolver::areParameterTypesApplicable(
    std::shared_ptr<parsetree::ast::MethodDecl> decl,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &argTypes) const {
  std::cout << "areParameterTypesApplicable" << std::endl;
  bool valid = true;
  for (size_t i = 0; i < argTypes.size(); i++) {
    auto ty1 = argTypes[i];
    auto ty2 = decl->getParams()[i]->getType();
    std::cout << "ty1: ";
    ty1->print(std::cout);
    std::cout << ", ty2: ";
    ty2->print(std::cout);
    std::cout << std::endl;
    valid &= typeResolver->isAssignableTo(ty1, ty2);
  }
  std::cout << "areParameterTypesApplicable: " << valid << std::endl;
  return valid;
}

bool ExprResolver::isMethodMoreSpecific(
    std::shared_ptr<parsetree::ast::MethodDecl> a,
    std::shared_ptr<parsetree::ast::MethodDecl> b) const {
  std::cout << "isMethodMoreSpecific" << std::endl;
  // Let a be declared in T with parameter types Ti
  // Let b be declared in U with parameter types Ui
  // Then a > b when for all i, Ti > Ui and T > U
  // Where two types A > B when A converts to B
  auto aDecl = std::make_shared<Decl>(
      std::dynamic_pointer_cast<parsetree::ast::Decl>(a->getParent()));
  auto T = std::make_shared<parsetree::ast::ReferenceType>(aDecl->getAstNode());
  T->setResolvedDecl(aDecl);

  auto bDecl = std::make_shared<Decl>(
      std::dynamic_pointer_cast<parsetree::ast::Decl>(b->getParent()));
  auto U = std::make_shared<parsetree::ast::ReferenceType>(bDecl->getAstNode());
  U->setResolvedDecl(bDecl);

  if (!typeResolver->isAssignableTo(T, U))
    return false;
  assert(a->getParams().size() == b->getParams().size());
  for (size_t i = 0; i < a->getParams().size(); i++) {
    auto Ti = a->getParams()[i]->getType();
    auto Ui = b->getParams()[i]->getType();
    if (!typeResolver->isAssignableTo(Ti, Ui))
      return false;
  }
  return true;
}

} // namespace static_check
