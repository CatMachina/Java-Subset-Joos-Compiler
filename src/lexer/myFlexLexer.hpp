#pragma once

#include <vector>

#include "parseTree/parseTree.hpp"

// Only include FlexLexer.h if it hasn't been already included
#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

class myFlexLexer : public yyFlexLexer {
    using Node = parsetree::Node;
    using Operator = parsetree::Operator;
    using Literal = parsetree::Literal;
    using Identifier = parsetree::Identifier;
    using Modifier = parsetree::Modifier;
    using BasicType = parsetree::BasicType;

public:
    // generated Flex lexer function
    int yylex();
    // bison-specific lexer function, implemented in the .l file
    int bison_lex(YYSTYPE* lvalp);

    template <typename... Args>
    std::unique_ptr<Node> make_node(Args&&... args) {
        auto nodePtr = std::make_unique<Node>(std::forward<Args>(args)...);
        nodes.push_back(nodePtr.get());
        return nodePtr.release();
    }

    std::unique_ptr<Node> make_corrupted() {
        auto nodePtr = std::make_unique<Node>(Node::Type::Corrupted);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

    std::unique_ptr<Node> make_operator(Operator::Type type) {
        auto nodePtr = std::make_unique<Operator>(type);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

    std::unique_ptr<Node> make_literal(Literal::Type type, const char* value) {
        auto nodePtr = std::make_unique<Literal>(type, value);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

    std::unique_ptr<Node> make_identifier(const char* name) {
        auto nodePtr = std::make_unique<Identifier>(name);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

    std::unique_ptr<Node> make_modifier(Modifier::Type type) {
        auto nodePtr = std::make_unique<Modifier>(type);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

    std::unique_ptr<Node> make_basic_type(BasicType::Type type) {
        auto nodePtr = std::make_unique<BasicType>(type);
        nodes.push_back(nodePtr.get());
        return nodePtr;
    }

private:
    YYSTYPE yylval;
    std::vector<Node*> nodes;
};
