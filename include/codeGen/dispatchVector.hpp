#include "ast/ast.hpp"

#include <set>

namespace codegen {

class DispatchVector {
public:
  std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> methodVector;
  std::vector<std::shared_ptr<parsetree::ast::FieldDecl>> fieldVector;

  DispatchVector(std::shared_ptr<parsetree::ast::ClassDecl> classDecl);

  int getFieldOffset(std::shared_ptr<parsetree::ast::FieldDecl> field) const {
    auto it = std::find(fieldVector.begin(), fieldVector.end(), field);
    if (it == fieldVector.end()) {
      throw std::runtime_error("Field not found in dispatch vector");
    }
    return std::distance(fieldVector.begin(), it);
  }
};

class DispatchVectorBuilder {

  static inline struct Graph {
    size_t minColours;
    std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> methods;
    std::unordered_map<std::shared_ptr<parsetree::ast::MethodDecl>,
                       std::set<std::shared_ptr<parsetree::ast::MethodDecl>>>
        neighbours;
    std::unordered_map<std::shared_ptr<parsetree::ast::MethodDecl>, int> colour;
  } graph;
  static inline std::unordered_map<std::shared_ptr<parsetree::ast::ClassDecl>,
                                   std::shared_ptr<DispatchVector>>
      classDVs;

  void addMethodsToGraph(
      std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> methods);

  void visitAST(std::shared_ptr<parsetree::ast::AstNode> node) {
    if (!node)
      throw std::runtime_error("Node is null when resolving AST");

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
      addMethodsToGraph(classDecl->getMethods());
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       node)) {
      addMethodsToGraph(interfaceDecl->getMethods());
    }

    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      visitAST(child);
    }
  }

public:
  static void assignColours() {
    auto firstVertex = graph.neighbours.begin()->first;
    graph.colour[firstVertex] = 1;

    std::unordered_set<int> availableColors;

    for (auto &vertex : graph.neighbours) {
      if (vertex.first == firstVertex)
        continue;

      availableColors.clear();

      for (auto &adjacent : graph.neighbours[vertex.first]) {
        if (graph.colour[adjacent] != 0) {
          availableColors.insert(graph.colour[adjacent]);
        }
      }

      int currentColor = 1;
      while (availableColors.find(currentColor) != availableColors.end()) {
        currentColor++;
      }

      graph.colour[vertex.first] = currentColor;
    }
  }

  static void resetColours() {
    for (auto node : graph.methods) {
      graph.colour[node] = 0;
    }
  }

  static void verifyColoured() {
    // verify all methods coloured and not share a colour with neighbours
    for (auto node : graph.methods) {
      int colour = graph.colour[node];
      if (colour <= 0) {
        throw std::runtime_error("Method has not been assigned a colour");
      }
      for (auto neighbour : graph.neighbours[node]) {
        if (colour == graph.colour[neighbour]) {
          throw std::runtime_error("Method shares colour with neighbour");
        }
      }
    }
  }

  static int getAssignment(std::shared_ptr<parsetree::ast::MethodDecl> method) {
    if (std::find(graph.methods.begin(), graph.methods.end(), method) ==
        graph.methods.end()) {
      throw std::runtime_error(
          "Method does not exist in graph when getting assignment");
    }
    int colour = graph.colour[method];
    if (colour <= 0) {
      throw std::runtime_error("Method has not been assigned a colour");
    }
    return colour;
  }

  static std::shared_ptr<DispatchVector>
  getDV(std::shared_ptr<parsetree::ast::ClassDecl> classDecl) {
    if (classDVs.find(classDecl) == classDVs.end()) {
      classDVs.insert({classDecl, std::make_shared<DispatchVector>(classDecl)});
    }
    return classDVs[classDecl];
  }

  void visit(std::shared_ptr<parsetree::ast::ASTManager> astManager) {
    for (auto ast : astManager->getASTs()) {
      visitAST(ast);
    }
  }
};

} // namespace codegen
