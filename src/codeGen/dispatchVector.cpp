#include "codeGen/dispatchVector.hpp"

namespace codegen {

DispatchVector::DispatchVector(
    std::shared_ptr<parsetree::ast::ClassDecl> classDecl) {
  for (auto &superClass : classDecl->getSuperClasses()) {
    if (!superClass || !superClass->getResolvedDecl() ||
        !superClass->getResolvedDecl())
      continue;
    // Cast to class
    auto superClassDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        superClass->getResolvedDecl()->getAstNode());

    if (superClassDecl->getFullName() != "java.lang.Object") {
      auto parentDV = DispatchVectorBuilder::getDV(superClassDecl);
      fieldVector.insert(fieldVector.end(), parentDV->fieldVector.begin(),
                         parentDV->fieldVector.end());
    }
  }

  auto classFields = classDecl->getFields();
  fieldVector.insert(fieldVector.end(), classFields.begin(), classFields.end());

  int maxColour = 0;
  for (auto &method : classDecl->getMethods()) {
    if (method->isConstructor())
      continue;
    int colour = DispatchVectorBuilder::getAssignment(method);
    maxColour = std::max(colour, maxColour);
  }
  methodVector.resize(maxColour + 1, nullptr);
  for (auto &method : classDecl->getMethods()) {
    if (method->isConstructor())
      continue;
    int colour = DispatchVectorBuilder::getAssignment(method);
    methodVector[colour] = method;
  }

  className = classDecl->getName();
}

void DispatchVectorBuilder::addMethodsToGraph(
    std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> methods) {
  graph.minColours = std::max(graph.minColours, methods.size());

  for (auto &method : methods) {
    if (method->isConstructor())
      continue;

    // Add to overall method array
    graph.methods.push_back(method);

    // Add all other methods as neighbours
    for (auto &other : methods) {
      if (method->isConstructor())
        continue;
      if (method == other)
        continue;
      graph.neighbours[method].insert(other);
      graph.neighbours[other].insert(method);
    }
  }
}

} // namespace codegen
