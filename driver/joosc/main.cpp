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
#include "staticCheck/astValidator.hpp"
#include "staticCheck/cfgBuilder.hpp"
#include "staticCheck/envManager.hpp"
#include "staticCheck/exprResolver.hpp"
#include "staticCheck/forwardChecker.hpp"
#include "staticCheck/hierarchyCheck.hpp"
#include "staticCheck/liveVariableAnalysis.hpp"
#include "staticCheck/reachabilityAnalysis.hpp"
#include "staticCheck/typeLinker.hpp"
#include "staticCheck/typeResolver.hpp"

#include "codeGen/astVisitor.hpp"
#include "codeGen/codeGenLabels.hpp"
#include "tir/TIRBuilder.hpp"

#include <memory>

#define EXIT_ERROR 42
#define EXIT_WARNING 43

// void checkLinked(std::shared_ptr<parsetree::ast::AstNode> node) {
//   if (!node)
//     throw std::runtime_error("Node is null when resolving AST");

//   for (auto child : node->getChildren()) {
//     if (!child)
//       continue;

//     // Case: Type
//     if (auto type = std::dynamic_pointer_cast<parsetree::ast::Type>(child)) {
//       if (!(type->isResolved())) {
//         type->print(std::cout);
//         std::cout << " not resolved" << std::endl;
//         // throw std::runtime_error(" Type still not resolved after
//         typeLinking");
//       }
//     }
//     // Case: regular code
//     else {
//       checkLinked(child);
//     }
//   }
// }

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
  int retCode = EXIT_SUCCESS;
  try {
    if (argc == 1) {
      std::cerr << "Usage: " << argv[0] << " input-files... " << std::endl;
      return EXIT_FAILURE;
    }

    source::SourceManager sm = source::SourceManager();
    auto astManager = std::make_shared<parsetree::ast::ASTManager>();
    auto env = std::make_shared<static_check::EnvManager>();

    std::cout << "Starting compilation..." << std::endl;

    // First pass: AST construction
    for (int file_number = 1; file_number < argc; ++file_number) {
      // Extract file path and validate extension
      const std::string filePath = argv[file_number];
      const std::string fileName =
          std::filesystem::path(filePath).stem().string();
      if (!filePath.ends_with(".java")) {
        std::cerr << "Error: not a valid .java file" << std::endl;
        return EXIT_ERROR;
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
        return EXIT_ERROR;
      }

      // Parse the input
      std::shared_ptr<parsetree::Node> parse_tree;
      myBisonParser parser{fileContent};
      parser.setFileID(file_number);
      int result = parser.parse(parse_tree);

      // Validate parse result
      if (!parse_tree || result) {
        std::cerr << "Parse error: parse failed" << std::endl;
        return EXIT_ERROR;
      }
      if (parse_tree->is_corrupted()) {
        std::cerr << "Parse error: parse tree is invalid" << std::endl;
        // comment out if not debugging
        // parse_tree->print(std::cerr);
        return EXIT_ERROR;
      }

      // Validate literal types
      if (!isLiteralTypeValid(parse_tree)) {
        std::cerr << "Parse error: invalid literal type" << std::endl;
        return EXIT_ERROR;
      }

      // if (file_number == 1)
      //   parse_tree->print(std::cout);

      // Build AST from the parse tree
      std::shared_ptr<parsetree::ast::ProgramDecl> ast;
      parsetree::ParseTreeVisitor visitor{env};
      try {
        if (parse_tree->is_corrupted())
          throw std::runtime_error("Parse tree is invalid");
        ast = visitor.visitProgramDecl(parse_tree);
      } catch (const std::exception &ex) {
        std::cerr << "Runtime error: " << ex.what() << std::endl;
        return EXIT_ERROR;
      } catch (...) {
        std::cerr << "Unknown failure occurred." << std::endl;
        return EXIT_FAILURE;
      }

      if (!ast) {
        std::cerr << "Parse error: failed to build AST" << std::endl;
        return EXIT_ERROR;
      }

      // Validate class/interface name matches file name
      const auto body =
          std::dynamic_pointer_cast<parsetree::ast::Decl>(ast->getBody());
      if (!body || body->getName() != fileName) {
        std::cerr
            << "Parse error: class/interface name does not match file name"
            << std::endl;
        std::cerr << "Class/interface name: "
                  << (body ? body->getName() : "<null>") << std::endl;
        std::cerr << "File name: " << fileName << std::endl;
        return EXIT_ERROR;
      }
      astManager->addAST(ast);
      // std::cout << "Parsed " << fileName << std::endl;
    }

    std::cout << "Passed AST constructions\n";

    // environment (symbol table) building + type linking
    auto typeLinker =
        std::make_shared<static_check::TypeLinker>(astManager, env);
    std::shared_ptr<static_check::Package> rootPackage =
        typeLinker->getRootPackage();
    // rootPackage->printStructure();
    std::cout << "Starting type linking\n";
    typeLinker->resolve();
    std::cout << "Populating java.lang\n";
    typeLinker->populateJavaLang();
    std::cout << "Passed type linking\n";

    // hierarchy checking
    auto hierarchyChecker =
        std::make_shared<static_check::HierarchyCheck>(rootPackage);
    std::cout << "Starting hierarchy check" << std::endl;
    if (!hierarchyChecker->check()) {
      std::cout << "Did not pass hierarchy check\n";
      return EXIT_ERROR;
    }
    std::cout << "Passed hierarchy check\n";

    // for (auto &ast : astManager->getASTs()) {
    //   checkLinked(ast);
    // }

    // astManager->getASTs()[0]->print(std::cout);

    std::cout << "Starting name disambiguation and type checking...\n";

    auto typeResolver =
        std::make_shared<static_check::TypeResolver>(astManager, env);

    auto exprResolver = std::make_shared<static_check::ExprResolver>(
        astManager, hierarchyChecker, typeLinker, typeResolver);
    exprResolver->resolve();

    auto astValidator =
        std::make_shared<static_check::ASTValidator>(typeResolver);
    astValidator->validate(astManager);

    auto forwardChecker = std::make_shared<static_check::ForwardChecker>();
    forwardChecker->check(astManager);

    std::cout << "Name disambiguation and type checking passed\n";

    auto cfgBuilder = std::make_shared<static_check::CFGBuilder>();
    std::cout << "Start building CFGs....\n";
    for (auto ast : astManager->getASTs()) {
      for (auto decl : ast->getBody()->getDecls()) {
        if (auto method =
                std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(decl)) {
          std::cout << "=== Start building CFG for method " << method->getName()
                    << " ===" << std::endl;
          std::shared_ptr<CFG> cfg = cfgBuilder->buildCFG(method);
          std::cout << "=== Done building CFG for method " << method->getName()
                    << " ===" << std::endl;
          if (cfg) {
            // cfg->print(std::cout);
            if (!static_check::ReachabilityAnalysis::checkUnreachableStatements(
                    cfg)) {
              std::cerr << "Method " << method->getName()
                        << " has unreachable statements" << std::endl;
              return EXIT_ERROR;
            }
            if (!static_check::ReachabilityAnalysis::checkFiniteLengthReturn(
                    cfg, method)) {
              std::cerr
                  << "Method " << method->getName()
                  << " finite-length execution paths do not all end in return"
                  << std::endl;
              return EXIT_ERROR;
            }
            std::cout << "Check Dead Assignments\n";
            if (!static_check::LiveVariableAnalysis::checkDeadAssignments(
                    cfg)) {
              std::cerr << "Warning: Method " << method->getName()
                        << " has dead assignments" << std::endl;
              retCode = EXIT_WARNING;
            }
          } else {
            std::cout << "Method is null or has no body." << std::endl;
          }
        }
      }
    }
    std::cout << "Done building CFGs....\n";

    // code gen
    auto tirBuilder = std::make_shared<tir::TIRBuilder>(astManager);
    tirBuilder->run();
    // auto codeGenLabels = std::make_shared<codegen::CodeGenLabels>();
    // auto astVisitor =
    //     std::make_shared<codegen::ASTVisitor>(astManager, codeGenLabels);
    // astVisitor->visit();

    return retCode;
  } catch (const std::runtime_error &err) {
    std::cerr << "Runtime error: " << err.what() << std::endl;
    return EXIT_ERROR;
  } catch (const std::exception &ex) {
    std::cerr << "Unhandled exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error occurred." << std::endl;
    return EXIT_FAILURE;
  }
}
