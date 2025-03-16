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
    return false;
  }
  if (!isClass(child)) {
    return false;
  }
  if (!isClass(super)) {
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

    if (isSuperClass(super, std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                                superClassDecl)))
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
    // std::cout << "-------- an AST resolved\n";
  }
}

void ExprResolver::resolveAST(std::shared_ptr<parsetree::ast::AstNode> node) {
  if (!node)
    throw std::runtime_error("Node is null when resolving AST");

  if (auto codeBody =
          std::dynamic_pointer_cast<parsetree::ast::CodeBody>(node)) {
    BeginContext(codeBody);
  }

  staticState.isInstFieldInitializer = false;
  staticState.fieldScope = nullptr;

  if (auto programDecl =
          std::dynamic_pointer_cast<parsetree::ast::ProgramDecl>(node)) {
    BeginProgram(programDecl);
  } else if (auto classDecl =
                 std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
    staticState.currentClass = classDecl;
    currentClass = classDecl;
    currentInterface = nullptr;
  } else if (auto interfaceDecl =
                 std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                     node)) {
    currentInterface = interfaceDecl;
    currentClass = nullptr;
  } else if (auto field =
                 std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(node)) {
    staticState.isStaticContext = field->isStatic();
    if (field->hasInit()) {
      staticState.isInstFieldInitializer = !(field->isStatic());
      staticState.fieldScope = field->getInitializer()->getScope();
    }
  } else if (auto method =
                 std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(node)) {
    staticState.isStaticContext = method->isStatic();
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
  staticResolver->evaluate(expr, staticState);
}

exprResolveType ExprResolver::evaluateList(
    std::vector<std::shared_ptr<parsetree::ast::ExprNode>> &list) {
  return Evaluator<exprResolveType>::evaluateList(list);
}

/**
 * Recursively reduce a parsed expression tree to the simplest possible
 * parsed expression tree. This is done by resolving names and member names
 * to their corresponding decls and types.
 *
 * @param node the input parsed expression tree
 * @return a vector of parsed expression nodes, which is the simplest possible
 * parsed expression tree
 */
std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::resolveExprNode(const exprResolveType node) {

  // If the node is already a vector of expression nodes, return it directly.
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(node)) {
    return std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(
        node);
    // If the node is a single expression node, process it.
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 node)) {
    // ExprNode
    auto expr = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(node);
    auto name = std::dynamic_pointer_cast<parsetree::ast::MemberName>(expr);
    auto thisNode = std::dynamic_pointer_cast<parsetree::ast::ThisNode>(expr);

    auto currentDecl = std::dynamic_pointer_cast<parsetree::ast::Decl>(
        currentProgram->getBody());
    if (!currentDecl) {
      throw std::runtime_error("current program body not Decl");
    }

    // 'this'
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

    auto childLinked = resolveName(name);

    // Recursively reduce the linked name to a list of expression nodes.
    return recursiveReduce(childLinked);
    // If the node is an instance of ExprNameLinked, resolve and reduce it.
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(node)) {
    auto expr = std::get<std::shared_ptr<ExprNameLinked>>(node);
    return recursiveReduce(expr);
  } else {
    throw std::runtime_error("should not happen");
  }
}

/**
 * Search for a named declaration in the given context.
 *
 * @param ctx the context in which to search
 * @param name the name to search for
 * @param loc the source location of the name
 * @return the unique declaration named `name` in `ctx`, or nullptr if
 *         no such declaration exists
 *
 * The search is performed in the following order:
 * 1. The declared set of `ctx`, if `ctx` is a class
 * 2. The inherit set of `ctx`, if `ctx` is a class
 * 3. The local variable set of `ctx`, otherwise
 *
 * After searching in the declared and inherit sets, the first matching
 * declaration is returned. If no matching declaration is found in the
 * declared and inherit sets, the search continues in the local variable
 * set, and the first matching declaration is returned. If no matching
 * declaration is found in the local variable set, nullptr is returned.
 *
 * Note that for protected fields, the search is only performed in the
 * declared set of `ctx`, and no matching declaration is returned if the
 * field is protected and not accessible.
 */
std::shared_ptr<parsetree::ast::Decl>
ExprResolver::lookupNamedDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                              std::string name, const source::SourceRange loc) {
  auto condition = [loc, name,
                    this](std::shared_ptr<parsetree::ast::Decl> decl) {
    if (auto typedDecl =
            std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl);
        typedDecl) {
      bool sameName = decl->getName() == name;
      bool sameContext = decl->getParent() == currentContext;
      bool checkScope =
          (!(std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) &&
           currentScope);
      bool scopeVisible = true;
      if (sameContext && checkScope)
        scopeVisible = currentScope->canView(typedDecl->getScope());
      bool canAccess = true;
      if (auto fieldDecl =
              std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
        canAccess =
            isAccessible(fieldDecl->getModifiers(), fieldDecl->getParent());
        if (!canAccess && fieldDecl->getModifiers()->isProtected())
          throw std::runtime_error("Cannot access protect field " +
                                   fieldDecl->getName());
      }
      // std::cout << "name: " << name << " got name: " << decl->getName()
      //           << " scopeVisible: " << scopeVisible
      //           << " canAccess: " << canAccess << std::endl;
      return sameName && scopeVisible && canAccess;
    }
    return false;
  };

  auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctx);
  if (classDecl && (ctx != astManager->java_lang.Array)) {
    // Search in the declared set
    auto declaredFields = classDecl->getFields();
    for (const auto decl : declaredFields) {
      if (condition(decl)) {
        return decl;
      }
    }

    // Search in the inherit set
    std::shared_ptr<parsetree::ast::Decl> result = nullptr;
    // TODO: this recomputes inherited field every time...
    auto inheritedFields = hierarchyChecker->getInheritedFields(classDecl);
    for (const auto decl : inheritedFields) {
      if (condition(decl.second)) {
        if (result)
          return nullptr; // Ambiguous case
        result = decl.second;
      }
    }
    if (result) {
      return result;
    }
  } else {
    // Search for the unique local variable
    for (auto decl : ctx->getDecls()) {
      if (condition(decl)) {
        return decl;
      }
    }
  }
  return nullptr;
}

/**
 * Reclassifies an expression node to resolve its declaration and type within
 * the given code body context.
 *
 * This function attempts to find and reclassify the declaration associated with
 * the specified expression node by searching within the provided context. It
 * first looks for a named declaration in the current context and reclassifies
 * the node based on whether it is a field or variable declaration. If the
 * declaration is not found, it recursively searches in the parent context.
 *
 * @param ctx The code body context in which to search for the declaration.
 * @param node The expression node to be reclassified.
 * @return A shared pointer to the resolved declaration, or nullptr if not
 * found.
 */

std::shared_ptr<parsetree::ast::Decl>
ExprResolver::reclassifyDecl(std::shared_ptr<parsetree::ast::CodeBody> ctx,
                             std::shared_ptr<ExprNameLinked> node) {
  auto astNode = node->getNode();
  if (auto decl = lookupNamedDecl(ctx, astNode->getName(), astNode->getLoc())) {
    if (auto fieldDecl =
            std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(decl)) {
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(fieldDecl, fieldDecl->getType());
      return fieldDecl;
    } else if (auto varDecl =
                   std::dynamic_pointer_cast<parsetree::ast::VarDecl>(decl)) {
      node->setValueType(ExprNameLinked::ValueType::ExpressionName);
      node->getNode()->resolveDeclAndType(varDecl, varDecl->getType());
      return varDecl;
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

/**
 * Resolves the member name of the given expression within the current context.
 *
 * This function attempts to resolve the declaration and type of the member
 * name represented by the provided expression node. It first checks if the
 * node's type and declaration are already resolved. If not, it reclassifies
 * the declaration within the current context. If the declaration is not found
 * in the current context, it searches for the declaration in the context of
 * imported names. If the member name is resolved as a variable or field
 * declaration, it updates the expression node with the appropriate type.
 * If it is resolved as a package name, it updates the expression node with
 * the package information.
 *
 * @param expr The expression node representing the member name to be resolved.
 * @return A shared pointer to the resolved expression node.
 * @throws std::runtime_error if the member name cannot be resolved or there
 *         is an import-on-demand conflict.
 */
std::shared_ptr<ExprNameLinked>
ExprResolver::resolveMemberName(std::shared_ptr<ExprNameLinked> expr) {
  // If the expression node already has both type and declaration resolved,
  // check if it is a variable declaration and classify it accordingly.
  if (expr->getNode()->isTypeResolved() && expr->getNode()->isDeclResolved()) {
    if (std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
            expr->getNode()->getResolvedDecl())) {
      expr->setValueType(ExprNameLinked::ValueType::ExpressionName);
      return expr;
    }
  }

  // Attempt to reclassify the declaration within the current package.
  auto decl = reclassifyDecl(currentContext, expr);
  if (decl != nullptr) {
    expr->getNode()->setResolvedDecl(decl);
    return expr;
  }

  // Try to find an import matching the expression node's name
  auto &context = typeLinker->getContext(currentProgram);
  auto import = context.find(expr->getNode()->getName()) != context.end()
                    ? context[expr->getNode()->getName()]
                    : nullptr;

  // not found
  if (std::holds_alternative<nullptr_t>(import)) {
    for (auto pair : context) {
      std::cout << pair.first << " ";
    }
    std::cout << std::endl;
    throw std::runtime_error("No import for " + expr->getNode()->getName());
  }

  // If the import is a declaration, classify it as a type and resolve it
  // accordingly.
  if (auto decl = std::get_if<std::shared_ptr<Decl>>(&import)) {
    if (!decl) {
      throw std::runtime_error("Ambiguous import-on-demand conflict");
    }
    auto declNode = (*decl)->getAstNode();
    expr->setValueType(ExprNameLinked::ValueType::TypeName);

    // Resolve the declaration as a reference type if not already resolved.
    if (!expr->getNode()->isTypeResolved()) {
      auto refType = std::make_shared<parsetree::ast::ReferenceType>(declNode);
      refType->setResolvedDecl(std::make_shared<Decl>(declNode));
      expr->getNode()->resolveDeclAndType(declNode, refType);
    } else if (!expr->getNode()->isDeclResolved()) {
      expr->getNode()->setResolvedDecl(declNode);
    };

    // If the import is a package, classify it as a package name and store it
  } else if (auto pkg = std::get_if<std::shared_ptr<Package>>(&import)) {
    expr->setValueType(ExprNameLinked::ValueType::PackageName);
    expr->setPackage(*pkg);
  }
  return expr;
}

/**
 * Recursively reduces a linked expression node to a list of the simplest
 * possible expression nodes, by traversing the linked structure starting
 * from the given node and collecting nodes in order.
 *
 * @param node A shared pointer to an `ExprNameLinked`, representing the
 * starting point of the reduction. The node is expected to have a value
 * type of `ExpressionName`.
 * @return A vector of shared pointers to `parsetree::ast::ExprNode`,
 * representing the reduced list of expression nodes.
 * @throws std::runtime_error If the node does not have the expected value
 * type or if required components are missing during traversal.
 */
std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
ExprResolver::recursiveReduce(std::shared_ptr<ExprNameLinked> node) {
  if (node->getValueType() != ExprNameLinked::ValueType::ExpressionName) {
    throw std::runtime_error("expected an expression name here");
  }

  // Base case: If there is no previous node or if the previous node is not an
  // expression name, return the current node as a single-element list.
  if (!node->getPrev().has_value() ||
      (node->prevAsLinked() && node->prevAsLinked()->getValueType() !=
                                   ExprNameLinked::ValueType::ExpressionName)) {
    return std::vector<std::shared_ptr<parsetree::ast::ExprNode>>{
        node->getNode()};
  }

  // Recursively reduce the previous node to obtain its expression node list.
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
  return node;
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

/**
 * Resolve field access.
 *
 * Given the left hand side of the expression Lhs and the identifier Id in Lhs .
 * Id, resolve the field access to a method or field name. This function is
 * complicated by the fact that the grammar is ambiguous, and we have to
 * distinguish between the two cases. If the LHS is a qualified name, then we
 * need to defer resolution of the qualified name until we see the RHS of the
 * expression (if there is one).
 *
 * If the LHS is a qualified name, then we need to distinguish between the two
 * cases. If the qualified name is a method name, then defer resolution of the
 * qualified name until we see the RHS of the expression (if there is one). If
 * the qualified name is a field name, then resolve the qualified name to a
 * single ambiguous name.
 *
 * If the LHS is not a qualified name, then we can resolve the field access to a
 * single ambiguous name. If the LHS is a this node, then set the field access
 * to be accessed by this.
 *
 * @param op The field access operator.
 * @param lhs The left hand side of the expression.
 * @param id The identifier on the right hand side of the expression.
 * @return A vector of expression nodes that represent the reduced expression.
 */
exprResolveType
ExprResolver::evalFieldAccess(std::shared_ptr<parsetree::ast::FieldAccess> &op,
                              const exprResolveType lhs,
                              const exprResolveType id) {
  previousType prev;
  // Check if lhs is a vector of expression nodes and assign it to prev.
  if (std::holds_alternative<
          std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs)) {
    prev =
        std::get<std::vector<std::shared_ptr<parsetree::ast::ExprNode>>>(lhs);
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
            typeLinker->resolveType(rType, currentProgram);
          }
        }
      }
    }

    // If lhs is a single expression node, resolve it accordingly.
  } else if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
                 lhs)) {
    auto node = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(lhs);

    // If lhs is a MemberName, resolve it.
    if (auto memberName =
            std::dynamic_pointer_cast<parsetree::ast::MemberName>(node)) {
      prev = resolveName(memberName);

      // If lhs is 'this', resolve it as an expression node.
    } else if (auto thisNode =
                   std::dynamic_pointer_cast<parsetree::ast::ThisNode>(node)) {
      prev = resolveExprNode(thisNode);

      // If lhs is a string literal, allow field access only if it's a string.
    } else if (auto literal =
                   std::dynamic_pointer_cast<parsetree::ast::Literal>(node)) {
      if (!literal->isString())
        throw std::runtime_error("accessing field on non string literal");
      prev = ExprNodeList{literal};
    } else {
      throw std::runtime_error(
          "cannot resolve field access due to bad grammar");
    }
    // If lhs is an ExprNameLinked, assign it directly.
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(lhs)) {
    prev = std::get<std::shared_ptr<ExprNameLinked>>(lhs);
  } else {
    throw std::runtime_error("should not reach here");
  }

  // Ensure that previous node is an expression, type, or package.
  auto exprNodePtr = std::get_if<std::shared_ptr<ExprNameLinked>>(&prev);
  if (exprNodePtr) {
    auto node = *exprNodePtr;
    // std::cout << "prev is expr name linked: " << node->getNode()->getName()
    //           << " is ";
    // node->getNode()->print(std::cout);
    // std::cout << " with type "
    //           << std::string(magic_enum::enum_name(node->getValueType()))
    //           << std::endl;
    if (node->getValueType() != ExprNameLinked::ValueType::ExpressionName &&
        node->getValueType() != ExprNameLinked::ValueType::TypeName &&
        node->getValueType() != ExprNameLinked::ValueType::PackageName) {
      throw std::runtime_error("cannot resolve field access due to bad "
                               "grammar, not expr type or package namew");
    }
  }

  // Retrieve the identifier (id) as an expression node.
  auto exprNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(id);
  // Special case: If "Id" in lhs . Id is a method name, defer resolution.
  if (auto methodNode =
          std::dynamic_pointer_cast<parsetree::ast::MethodName>(exprNode)) {
    auto newPrev = std::make_shared<ExprNameLinked>(
        ExprNameLinked::ValueType::MethodName, methodNode, op);
    newPrev->setPrev(prev);
    return newPrev;
  }

  // Cast the identifier to a MemberName (field access).
  auto fieldNode =
      std::dynamic_pointer_cast<parsetree::ast::MemberName>(exprNode);
  if (!fieldNode)
    throw std::runtime_error("Bad node. Expected MemberName here.");

  // Check if lhs was 'this' and mark the field as accessed by 'this'.
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(lhs)) {
    auto lhsNode = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(lhs);
    if (std::dynamic_pointer_cast<parsetree::ast::ThisNode>(lhsNode)) {
      fieldNode->setAccessedByThis();
    } else if (auto memberName =
                   std::dynamic_pointer_cast<parsetree::ast::MemberName>(
                       lhsNode)) {
      if (memberName->getName() == currentClass->getName()) {
        fieldNode->setAccessedByThis();
      }
    }
  }
  // Mark the field as not a base. public static int x = a.b.c -> a is base
  fieldNode->setNotAsBase();

  // Create a new node representing "Id" in lhs . Id
  auto newPrev = std::make_shared<ExprNameLinked>(
      ExprNameLinked::ValueType::SingleAmbiguousName, fieldNode, op);
  newPrev->setPrev(prev);

  // Resolve the field access based on the previous node's type.
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

/**
 * Evaluates a method invocation expression by resolving the method being
 * called and its arguments, and then constructing a list of expression nodes
 * that represent the invocation.
 *
 * @param op A shared pointer to a MethodInvocation object representing the
 * method invocation operation.
 * @param method The method to be invoked, represented as an exprResolveType
 * which can be an ExprNode or an ExprNameLinked.
 * @param args A constant reference to an array of arguments for the method
 * call.
 * @return A list of expression nodes representing the resolved method
 * invocation.
 * @throws std::runtime_error If the method node is not a MethodName, if the
 * method call expression is invalid, if any argument type cannot be resolved,
 * or if the method call is to a non-accessible or non-static method where
 * required.
 */
exprResolveType ExprResolver::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const exprResolveType method, const op_array &args) {
  // incomplete resolved method name
  std::shared_ptr<ExprNameLinked> unresolved = nullptr;

  // Flag indicating whether the method call is a single name method call.
  // Single name methods cannot be static, and static methods cannot be single
  // name methods.
  bool isSingleNameMethod = false;

  // If the method is represented as a single expression node, resolve it.
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
          method)) {
    auto expr = std::get<std::shared_ptr<parsetree::ast::ExprNode>>(method);
    auto name = std::dynamic_pointer_cast<parsetree::ast::MethodName>(expr);
    if (!name)
      throw std::runtime_error("Bad node. Expected MethodName here.");
    unresolved = resolveName(name);
    isSingleNameMethod = true;
    // If the method is already an unresolved name linked structure, retrieve
    // it.
  } else if (std::holds_alternative<std::shared_ptr<ExprNameLinked>>(method)) {
    unresolved = std::get<std::shared_ptr<ExprNameLinked>>(method);
  } else {
    throw std::runtime_error("Bad Method Call Expression");
  }

  // Resolve the argument expressions and their types.
  std::vector<std::shared_ptr<parsetree::ast::Type>> argTypes;
  ExprNodeList arglist;
  // Iterate through arguments in reverse order (due to RPN evaluation
  // mechanics).
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
  auto ctx = getMethodParent(unresolved);
  auto methodDecl =
      resolveMethodOverload(ctx, unresolved->getNode()->getName(), argTypes,
                            unresolved->getNode()->getLoc(), false);

  // Ensure the method is accessible from the current context.
  if (!isAccessible(methodDecl->getModifiers(), methodDecl->getParent())) {
    throw std::runtime_error("method call to non-accessible method: " +
                             methodDecl->getName());
  }

  // Ensure single name methods are not static.
  if (isSingleNameMethod && methodDecl->getModifiers()->isStatic()) {
    throw std::runtime_error(
        "attempted to call static method using single name: " +
        methodDecl->getName());
  }

  // If the method is being accessed through a type name, ensure it is static.
  if (unresolved->getPrev().has_value() && unresolved->prevAsLinked() &&
      unresolved->prevAsLinked()->getValueType() ==
          ExprNameLinked::ValueType::TypeName) {
    // we are calling a static method, check
    if (!methodDecl->getModifiers()->isStatic()) {
      throw std::runtime_error("attempted to call non-static method: " +
                               methodDecl->getName());
    }
  }

  // Reclassify the unresolved name as an expression name (since it has been
  // resolved).
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

/**
 * Constructs a new object by resolving the class type and its constructor
 * arguments, and returns a list of expression nodes representing the object
 * creation.
 *
 * @param op A shared pointer to a ClassCreation object representing the
 * object creation operation.
 * @param object The object type to be instantiated, represented as an
 * exprResolveType which should be an ExprNode.
 * @param args A constant reference to an array of constructor arguments.
 * @return A list of expression nodes representing the resolved object
 * creation.
 * @throws std::runtime_error If the object node is not a TypeNode, if the
 * class type is not a reference type, if the class is abstract, or if the
 * constructor cannot be resolved.
 */
exprResolveType
ExprResolver::evalNewObject(std::shared_ptr<parsetree::ast::ClassCreation> &op,
                            const exprResolveType object,
                            const op_array &args) {
  // Get the type we are instantiating
  std::shared_ptr<parsetree::ast::TypeNode> expr = nullptr;
  // Ensure that the object is a valid TypeNode.
  if (std::holds_alternative<std::shared_ptr<parsetree::ast::ExprNode>>(
          object)) {
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
  // Process arguments in reverse order to match expected evaluation order.
  for (auto it = args.rbegin(); it != args.rend(); ++it) {
    auto arg = *it;
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

    argType.push_back(typeResolver->EvalList(tmplist));
    arglist.insert(arglist.end(), tmplist.begin(), tmplist.end());
  }

  // Check if the type being instantiated is a reference type.
  auto rType =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(expr->getType());
  if (!rType) {
    throw std::runtime_error("New object not reference type");
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

  // Check if the class being instantiated is abstract and throw an error if so
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
  auto loc = source::SourceRange();
  loc.fileID = INT_MAX;
  auto methodDecl = resolveMethodOverload(ctx, "", argType, loc, true);
  // override
  expr->setResolvedDecl(methodDecl);

  // Once op has been resolved, we can build the expression list
  ExprNodeList list;
  list.push_back(expr);
  list.insert(list.end(), arglist.begin(), arglist.end());
  list.push_back(op);

  return list;
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

/**
 * Resolves an assignment expression by processing the left-hand side (lhs)
 * and right-hand side (rhs) of the assignment, and constructs a list of
 * expression nodes representing the assignment operation.
 *
 * This function handles the initialization status of member names involved in
 * the assignment. It marks them as initialized within the expression if they
 * are part of the left-hand side.
 *
 * @param op A shared pointer to an Assignment object representing the
 * assignment operation.
 * @param lhs The left-hand side of the assignment, represented as an
 * exprResolveType which can be an ExprNode or an ExprNameLinked.
 * @param rhs The right-hand side of the assignment, represented as an
 * exprResolveType which can be an ExprNode or an ExprNameLinked.
 * @return A list of expression nodes representing the resolved assignment
 * operation.
 */
exprResolveType
ExprResolver::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                             const exprResolveType lhs,
                             const exprResolveType rhs) {
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> ret;

  // FIXME: temp workaround
  auto lhsVec = resolveExprNode(lhs);
  if (lhsVec.size() == 1) {
    if (auto memberName =
            std::dynamic_pointer_cast<parsetree::ast::MemberName>(lhsVec[0])) {
      memberName->setinitializedInExpr();
    }
  } else if (lhsVec.size() == 3) {
    if (std::dynamic_pointer_cast<parsetree::ast::FieldAccess>(lhsVec[2])) {
      if (auto memberName =
              std::dynamic_pointer_cast<parsetree::ast::MemberName>(
                  lhsVec[1])) {
        memberName->setinitializedInExpr();
      }
    }
  }

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

/**
 * Return the previous node as a Decl.
 *
 * The previous node can either be an ExprNameLinked or an expression list.
 * If the previous node is an ExprNameLinked, then the method returns the
 * resolved Decl of that node. If the previous node is an expression list,
 * the method resolves the expression list to a type and then returns the
 * corresponding Decl of that type.
 *
 * @param TR The type resolver.
 * @param astManager The AST Manager.
 * @param typeLinker The type linker.
 * @return The previous node as a Decl.
 */
std::shared_ptr<parsetree::ast::Decl> ExprNameLinked::prevAsDecl(
    std::shared_ptr<TypeResolver> TR,
    std::shared_ptr<parsetree::ast::ASTManager> astManager,
    std::shared_ptr<TypeLinker> typeLinker) const {
  if (auto p = prevAsLinked()) {
    return p->getNode()->getResolvedDecl();
  }
  // Complex case, the previous node is an expression list
  if (!prev_.has_value())
    throw std::runtime_error("No previous value");
  if (!std::holds_alternative<ExprNodeList>(prev_.value()))
    throw std::runtime_error("Previous Not an expression list");
  ExprNodeList prevList = std::get<ExprNodeList>(prev_.value());

  // FIXME: rm this when type linker works
  for (auto &node : prevList) {
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

  auto type = TR->EvalList(prevList);
  return std::dynamic_pointer_cast<parsetree::ast::Decl>(
      GetTypeAsDecl(type, astManager));
}

/**
 * Resolves a field access node by first verifying the invariants and then
 * fetching the type or declaration. If the previous node is a wrapper, then
 * we resolve the type. Then we check if the name is a field of the
 * declaration. If it is, we set the value type of the ExprNameLinked to
 * ExpressionName and resolve the decl and type of the node.
 *
 * @param access The ExprNameLinked to resolve.
 */
void ExprResolver::resolveFieldAccess(std::shared_ptr<ExprNameLinked> access) {
  // Ensure the accessed name is an ambiguous single name (which can be a
  // field).
  if (access->getValueType() != ExprNameLinked::ValueType::SingleAmbiguousName)
    throw std::runtime_error("Not a ambiguous name, Not a field access");

  // Ensure the previous node is an expression name before treating it as a
  // field access.
  if (auto p = access->prevAsLinked()) {
    if (p->getValueType() != ExprNameLinked::ValueType::ExpressionName)
      throw std::runtime_error("Not an expression name, Not a field access");
  }

  // Next, fetch the type or declaration
  auto name = access->getNode()->getName();
  auto typeOrDecl = access->prevAsDecl(typeResolver, astManager, typeLinker);
  std::shared_ptr<parsetree::ast::CodeBody> refType = nullptr;
  // If the previous node is linked, retrieve its type information.
  if (access->prevAsLinked()) {
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

    // Retrieve the declaration of the type being accessed.
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
  auto loc = source::SourceRange();
  auto field = lookupNamedDecl(refType, name, loc);
  if (!field) {
    throw std::runtime_error("field access failed for " + name);
  }
  // Ensure the resolved field is either a FieldDecl or MethodDecl
  if (!std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field) &&
      !std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(field)) {
    throw std::runtime_error(
        "field shouldn't be anything other than field or method");
  }

  // If the field is a FieldDecl, retrieve its type as type
  std::shared_ptr<parsetree::ast::Type> fieldType = nullptr;
  if (auto x = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field))
    fieldType = x->getType();
  access->setValueType(ExprNameLinked::ValueType::ExpressionName);
  access->getNode()->resolveDeclAndType(field, fieldType);
}

/**
 * Resolve a type access by verifying invariants and then fetching the type or
 * declaration. We check if the name is a field of the declaration. If it is,
 * we set the value type of the ExprNameLinked to ExpressionName and resolve
 * the decl and type of the node.
 *
 * @param access The ExprNameLinked to resolve.
 */
void ExprResolver::resolveTypeAccess(std::shared_ptr<ExprNameLinked> access) {
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
  auto field = lookupNamedDecl(type, name, access->getNode()->getLoc());
  if (!field) {
    throw std::runtime_error("type access failed for " + name);
  }
  // With the additional constraint that the field must be static
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

  // Handle protected access: ensure that the current class has access to the
  // protected field
  if (mods->isProtected()) {
    if (auto fieldClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            field->getParent())) {
      if (currentClass) {
        if (fieldClass != currentClass &&
            !isSuperClass(fieldClass, currentClass)) {
          throw std::runtime_error("cannot access protected field " +
                                   field->getName() + " from class " +
                                   currentClass->getName());
        }
      } else if (currentInterface) {

      } else {
        throw std::runtime_error(
            "No current class or interface, should not happen");
      }
    }
  }
  // field access is valid
  std::shared_ptr<parsetree::ast::Type> fieldType = nullptr;
  if (auto x = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(field))
    fieldType = x->getType();
  access->setValueType(ExprNameLinked::ValueType::ExpressionName);
  access->getNode()->resolveDeclAndType(field, fieldType);
  access->setPrev(std::nullopt);
}

/**
 * Resolve a package access by verifying invariants and then fetching the
 * package or type. We check if the name is a package of the previous package.
 * If it is, we set the value type of the ExprNameLinked to TypeName and
 * resolve the decl and type of the node.
 *
 * @param access The ExprNameLinked to resolve.
 */
void ExprResolver::resolvePackageAccess(
    std::shared_ptr<ExprNameLinked> access) const {
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

/**
 * Get the code body of the method declaration given an ExprNameLinked
 * representing a method name. If the method name has a previous node, we get
 * the declaration or type of the previous node. If the previous node is a
 * declaration, we return the code body of the declaration. If the previous
 * node is a type, we get the declaration of the type and return its code body.
 * If there is no previous node, we return the code body of the current program.
 *
 * @param method The ExprNameLinked to get the parent of.
 * @return The code body of the method declaration.
 */
std::shared_ptr<parsetree::ast::CodeBody>
ExprResolver::getMethodParent(std::shared_ptr<ExprNameLinked> method) const {
  if (method->getValueType() != ExprNameLinked::ValueType::MethodName) {
    throw std::runtime_error("Not a method name, cannot get parent");
  }
  // If there's no previous, use the current context
  if (!method->getPrev().has_value())
    return currentProgram->getBody();
  auto declOrType = method->prevAsDecl(typeResolver, astManager, typeLinker);
  auto ty = declOrType->asCodeBody();

  // If the previous node is a linked expression and not yet resolved to a
  // CodeBody, resolve it further.
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
    if (!typeAsDecl)
      throw std::runtime_error("method call to non-reference type");
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

static void removeDuplicates(
    std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> &maxSpecific) {
  std::unordered_set<parsetree::ast::MethodDecl *> seen;

  auto newEnd = std::remove_if(
      maxSpecific.begin(), maxSpecific.end(),
      [&seen](const std::shared_ptr<parsetree::ast::MethodDecl> &ptr) {
        return !seen.insert(ptr.get())
                    .second; // Returns true if already seen (to be removed)
      });

  maxSpecific.erase(newEnd, maxSpecific.end());
}

/**
 * Resolves an overloaded method by determining which method is most specific
 * for the given argument types. If there are multiple methods with the same
 * level of specificity, an ambiguity error is thrown.
 *
 * @param ctx The context in which the method is to be resolved. This is either
 *            a class or an interface.
 * @param name The name of the method to resolve.
 * @param argTypes The types of the arguments to the method.
 * @param loc The source location of the method call.
 * @param isConstructor Whether or not the method is a constructor.
 *
 * @return The most specific method that matches the given arguments.
 *
 * @throws std::runtime_error if no matching method is found, or if there is an
 *         ambiguity error.
 */
std::shared_ptr<parsetree::ast::MethodDecl> ExprResolver::resolveMethodOverload(
    std::shared_ptr<parsetree::ast::CodeBody> ctx, std::string name,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &argTypes,
    const source::SourceRange loc, bool isConstructor) {
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
      if (areParameterTypesApplicable(ctor, argTypes)) {
        candidates.push_back(ctor);
      }
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
    std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> allMethodsVec;
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(ctxDecl)) {
      allMethodsVec = classDecl->getMethods();
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       ctxDecl)) {
      allMethodsVec = interfaceDecl->getMethods();
    }
    for (auto pair : allMethods) {
      allMethodsVec.push_back(pair.second);
    }
    for (auto decl : allMethodsVec) {
      if (!decl)
        continue;
      if (decl->getParams().size() != argTypes.size())
        continue;
      if (decl->getName() != name)
        continue;
      if (!areParameterTypesApplicable(decl, argTypes))
        continue;
      if (!(decl->getParent() == ctx) &&
          !isAccessible(decl->getModifiers(), decl->getParent()))
        continue;
      if (decl->isConstructor())
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
  removeDuplicates(maxSpecific);
  // If there's only one maximally specific method, return it
  if (maxSpecific.size() == 1) {
    return maxSpecific[0];
  }
  // There are more conditions i.e., abstract...
  // Otherwise, we have an ambiguity error
  std::cout << "ambiguous method found for " << name << std::endl;
  for (auto cur : maxSpecific) {
    std::cout << ",  ";
    cur->print(std::cout);
    std::cout << std::endl;
  }
  throw std::runtime_error("ambiguous method found for " + std::string(name));
}

bool ExprResolver::areParameterTypesApplicable(
    std::shared_ptr<parsetree::ast::MethodDecl> decl,
    const std::vector<std::shared_ptr<parsetree::ast::Type>> &argTypes) const {
  bool valid = true;
  for (size_t i = 0; i < argTypes.size(); i++) {
    auto ty1 = argTypes[i];
    auto ty2 = decl->getParams()[i]->getType();
    // std::cout << "ty1: ";
    // ty1->print(std::cout);
    // std::cout << ", ty2: ";
    // ty2->print(std::cout);
    // std::cout << std::endl;
    valid &= typeResolver->isAssignableTo(ty1, ty2);
  }
  return valid;
}

/**
 * Check if a is more specific than b.
 *
 * A method a is more specific than another method b if a's parameter types
 * are all more specific than b's and a's declaring type is more specific than
 * b's.
 *
 * Two types A > B when A converts to B.
 *
 * @param a the first method declaration
 * @param b the second method declaration
 * @return true if a is more specific than b, false otherwise
 */
bool ExprResolver::isMethodMoreSpecific(
    std::shared_ptr<parsetree::ast::MethodDecl> a,
    std::shared_ptr<parsetree::ast::MethodDecl> b) const {
  auto aDecl = std::make_shared<Decl>(
      std::dynamic_pointer_cast<parsetree::ast::Decl>(a->getParent()));
  auto T = std::make_shared<parsetree::ast::ReferenceType>(aDecl->getAstNode());
  T->setResolvedDecl(aDecl);

  auto bDecl = std::make_shared<Decl>(
      std::dynamic_pointer_cast<parsetree::ast::Decl>(b->getParent()));
  auto U = std::make_shared<parsetree::ast::ReferenceType>(bDecl->getAstNode());
  U->setResolvedDecl(bDecl);

  if (!typeResolver->isAssignableTo(U, T))
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
