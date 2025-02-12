#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast/ast.hpp"
#include "parseTree/parseTree.hpp"
#include "parseTree/parseTreeVisitor.hpp"
#include "parseTree/sourceNode.hpp"
#include "parser/myBisonParser.hpp"
#include "staticCheck/envManager.hpp"

#include <memory>

// hack
bool isLiteralTypeValid(const std::shared_ptr<parsetree::Node> &node) {
  if (!node)
    return true;

  if (node->get_node_type() == parsetree::Node::Type::Literal) {
    auto literalNode = std::dynamic_pointer_cast<parsetree::Literal>(node);
    return literalNode && literalNode->isValid();
  }

  return std::all_of(
      node->children().begin(), node->children().end(),
      [](const auto &child) { return isLiteralTypeValid(child); });
}

int main(int argc, char **argv) {
  try {
    if (argc == 1) {
      std::cerr << "Usage: " << argv[0] << " input-files... " << std::endl;
      return EXIT_FAILURE;
    }

    source::SourceManager sm = source::SourceManager();
    parsetree::ast::ASTManager astManager;

    for (int file_number = 1; file_number < argc; ++file_number) {
      // Extract file path and validate extension
      const std::string filePath = argv[file_number];
      const std::string fileName =
          std::filesystem::path(filePath).stem().string();
      if (!filePath.ends_with(".java")) {
        std::cerr << "Error: not a valid .java file" << std::endl;
        return 42;
      }

      // Track file
      sm.addFile(fileName);

      // Read file content
      std::ifstream inputFile(filePath, std::ios::binary);
      if (!inputFile.is_open()) {
        std::cerr << "Error! Could not open input file \"" << filePath << "\""
                  << std::endl;
        return EXIT_FAILURE;
      }
      const std::string fileContent((std::istreambuf_iterator<char>(inputFile)),
                                    std::istreambuf_iterator<char>());

      // Check for non-ASCII characters
      if (std::any_of(fileContent.begin(), fileContent.end(), [](char c) {
            return static_cast<unsigned char>(c) > 127;
          })) {
        std::cerr << "Parse error: non-ASCII character in input" << std::endl;
        return 42;
      }

      // Parse the input
      std::shared_ptr<parsetree::Node> parse_tree;
      myBisonParser parser{fileContent};
      int result = parser.parse(parse_tree);

      // Validate parse result
      if (!parse_tree || result) {
        return 42;
      }
      if (parse_tree->is_corrupted()) {
        std::cerr << "Parse error: parse tree is invalid" << std::endl;
        // comment out if not debugging
        // parse_tree->print(std::cerr);
        return 42;
      }

      // Validate literal types
      if (!isLiteralTypeValid(parse_tree)) {
        std::cerr << "Parse error: invalid literal type" << std::endl;
        return 42;
      }

      // Build AST from the parse tree
      std::shared_ptr<parsetree::ast::ProgramDecl> ast;
      static_check::EnvManager env;
      parsetree::ParseTreeVisitor visitor{env};
      try {
        if (parse_tree->is_corrupted())
          throw std::runtime_error("Parse tree is invalid");
        ast = visitor.visitProgramDecl(parse_tree);
      } catch (const std::exception &ex) {
        std::cerr << "Runtime error: " << ex.what() << std::endl;
        return 42;
      } catch (...) {
        std::cerr << "Unknown failure occurred." << std::endl;
        return EXIT_FAILURE;
      }

      if (!ast) {
        return 42;
      }

      // Validate class/interface name matches file name
      const auto cuBody =
          std::dynamic_pointer_cast<parsetree::ast::Decl>(ast->getBody());
      if (!cuBody || cuBody->getName() != fileName) {
        std::cerr
            << "Parse error: class/interface name does not match file name"
            << std::endl;
        std::cerr << "Class/interface name: "
                  << (cuBody ? cuBody->getName() : "<null>") << std::endl;
        std::cerr << "File name: " << fileName << std::endl;
        return 42;
      }

      astManager.addAST(ast);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception &ex) {
    std::cerr << "Unhandled exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error occurred." << std::endl;
    return EXIT_FAILURE;
  }
}
