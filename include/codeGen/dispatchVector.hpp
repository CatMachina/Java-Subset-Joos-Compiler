#include "ast/ast.hpp"

#include <set>

namespace codegen {

class DispatchVector {
public:
  std::vector<std::shared_ptr<parsetree::ast::MethodDecl>> methodVector;
  std::vector<std::shared_ptr<parsetree::ast::FieldDecl>> fieldVector;
  std::string className;

  DispatchVector(std::shared_ptr<parsetree::ast::ClassDecl> classDecl);

  int getFieldOffset(std::shared_ptr<parsetree::ast::FieldDecl> field) const {
    auto it = std::find(fieldVector.begin(), fieldVector.end(), field);
    if (it == fieldVector.end()) {
      throw std::runtime_error("Field " + field->getName() +
                               " not found in dispatch vector of " + className);
    }
    return std::distance(fieldVector.begin(), it);
  }

  std::ostream &printIndent(std::ostream &os, int indent = 0) const {
    for (int i = 0; i < indent; ++i) {
      os << "  ";
    }
    return os;
  }

  std::ostream &print(std::ostream &os, int indent = 0) const {
    printIndent(os, indent);
    os << "(DispatchVector: " << className << "\n";
    printIndent(os, indent + 1);
    os << "Methods:\n";
    for (size_t i = 0; i < methodVector.size(); ++i) {
      auto &method = methodVector[i];
      if (!method)
        continue;
      printIndent(os, indent + 2);
      os << method->getName() << ", colour: " << i << "\n";
    }
    printIndent(os, indent + 1);
    os << "Fields:\n";
    for (const auto &field : fieldVector) {
      printIndent(os, indent + 2);
      os << field->getName() << "\n";
    }
    printIndent(os, indent);
    os << ")\n";
    return os;
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
      std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>> methods);

  void visitAST(std::shared_ptr<parsetree::ast::AstNode> node) {
    if (!node)
      throw std::runtime_error("Node is null when resolving AST");

    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
      // std::cout << "addMethodsToGraph for class: " << classDecl->getFullName()
      //           << std::endl;
      if (classDecl->getAllMethods().size() == 0) {
        throw std::runtime_error("Class has no methods");
      }
      addMethodsToGraph(classDecl->getAllMethods());
    } else if (auto interfaceDecl =
                   std::dynamic_pointer_cast<parsetree::ast::InterfaceDecl>(
                       node)) {
      // std::cout << "addMethodsToGraph for interface: "
      //           << interfaceDecl->getFullName() << std::endl;
      std::unordered_set<std::shared_ptr<parsetree::ast::MethodDecl>> methods;
      for (auto method : interfaceDecl->getMethods()) {
        methods.insert(method);
      }
      addMethodsToGraph(methods);
    }

    for (const auto &child : node->getChildren()) {
      if (!child)
        continue;
      visitAST(child);
    }
  }

public:
  static void assignColours() {
    if (graph.neighbours.size() == 0) {
      return;
    }
    auto firstVertex = graph.neighbours.begin()->first;
    graph.colour[firstVertex] = 1;
    // std::cout << "assign first color to " << firstVertex->getFullName()
    //           << std::endl;

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
      if (std::find(graph.methods.begin(), graph.methods.end(), node) ==
          graph.methods.end()) {
        throw std::runtime_error("In verifyColoured: Method " +
                                 node->getFullName() +
                                 " does not exist in graph");
      }
      int colour = graph.colour[node];
      // if (colour <= 0) {
      //   throw std::runtime_error("In verifyColoured: Method " +
      //   node->getFullName() + " has not been "
      //                            "assigned a colour with colour " +
      //                            std::to_string(colour));
      // }
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
      // std::cout << "graph methods has " << graph.methods.size()
      //           << " elements\n";
      throw std::runtime_error(
          "Method " + method->getFullName() +
          " does not exist in graph when getting assignment");
    }
    int colour = graph.colour[method];
    // if (colour <= 0) {
    //   throw std::runtime_error("In getAssignment: Method has not been
    //   assigned "
    //                            "a colour with colour " +
    //                            std::to_string(colour));
    // }
    return colour;
  }

  static std::shared_ptr<DispatchVector>
  getDV(std::shared_ptr<parsetree::ast::ClassDecl> classDecl) {
    if (classDVs.find(classDecl) == classDVs.end()) {
      auto classDV = std::make_shared<DispatchVector>(classDecl);
      // std::cout << "created classDV: \n";
      // classDV->print(std::cout);
      classDVs.insert({classDecl, classDV});
    }
    if (classDVs.find(classDecl) == classDVs.end()) {
      throw std::runtime_error("Could not create or find classDV");
    }
    return classDVs[classDecl];
  }

  void visit(std::shared_ptr<parsetree::ast::ASTManager> astManager) {
    for (auto ast : astManager->getASTs()) {
      visitAST(ast);
    }
  }

  void
  printClassDV(std::shared_ptr<parsetree::ast::ClassDecl> classDecl) const {
    auto classDV = getDV(classDecl);
    if (!classDV) {
      throw std::runtime_error("Could not create classDV");
    }
    std::cout << "==== " << classDecl->getFullName() << " ====\n";
    int i = -1;
    for (auto &method : classDV->methodVector) {
      i++;
      if (!method)
        continue;
      std::cout << "Method " << getAssignment(method) << ": "
                << method->getFullName() << "\n";
    }
    std::cout << std::endl;
  }

  void printItself() const {
    for (auto &classDV : classDVs) {
      std::cout << "==== " << classDV.first->getFullName() << " ====\n";
      int i = -1;
      for (auto &method : classDV.second->methodVector) {
        i++;
        if (!method)
          continue;
        std::cout << "Method " << getAssignment(method) << ": "
                  << method->getFullName() << "\n";
      }
      std::cout << std::endl;
    }
  }

  void print(std::shared_ptr<parsetree::ast::AstNode> node) const {
    if (!node)
      throw std::runtime_error("Node is null when printing AST");
    if (auto classDecl =
            std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(node)) {
      printClassDV(classDecl);
    } else {
      for (const auto &child : node->getChildren()) {
        if (!child)
          continue;
        print(child);
      }
    }
  }
};

} // namespace codegen
