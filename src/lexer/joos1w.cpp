#include "../src/parseTree/parseTree.hpp"
#include "../src/parser/myBisonParser.hpp"

using Node = parsetree::Node;
using Operator = parsetree::Operator;
using Literal = parsetree::Literal;
using Identifier = parsetree::Identifier;
using Modifier = parsetree::Modifier;
using BasicType = parsetree::BasicType;
using Corrupted = parsetree::Corrupted;

std::shared_ptr<Node> myFlexLexer::make_corrupted(const char *name) {
  auto nodePtr = std::make_shared<Corrupted>(name);
  nodes.push_back(nodePtr);
  return nodePtr;
}

std::shared_ptr<Node> myFlexLexer::make_operator(Operator::Type type) {
  auto nodePtr = std::make_shared<Operator>(type);
  nodes.push_back(nodePtr);
  return nodePtr;
}

std::shared_ptr<Node> myFlexLexer::make_literal(Literal::Type type,
                                                const char *value) {
  auto nodePtr = std::make_shared<Literal>(type, value);
  nodes.push_back(nodePtr);
  return nodePtr;
}

std::shared_ptr<Node> myFlexLexer::make_identifier(const char *name) {
  auto nodePtr = std::make_shared<Identifier>(std::string(name));
  nodes.push_back(nodePtr);
  return nodePtr;
}

std::shared_ptr<Node> myFlexLexer::make_modifier(Modifier::Type type) {
  auto nodePtr = std::make_shared<Modifier>(type);
  nodes.push_back(nodePtr);
  return nodePtr;
}

std::shared_ptr<Node> myFlexLexer::make_basic_type(BasicType::Type type) {
  auto nodePtr = std::make_shared<BasicType>(type);
  nodes.push_back(nodePtr);
  return nodePtr;
}
