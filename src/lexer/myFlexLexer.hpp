#pragma once

#include <vector>
#include <memory>

// #include "parsetree/parseTree.hpp"
#include "../src/parseTree/parseTree.hpp"

#include "parser.tab.h"

class myBisonParser;

class myFlexLexer : public yyFlexLexer {
    using Node = parsetree::Node;
    using Operator = parsetree::Operator;
    using Literal = parsetree::Literal;
    using Identifier = parsetree::Identifier;
    using Modifier = parsetree::Modifier;
    using BasicType = parsetree::BasicType;
    friend class myBisonParser;

public:
    // generated Flex lexer function
    int yylex();
    // bison-specific lexer function, implemented in the .l file
    int bison_lex(YYSTYPE* lvalp);

    template <typename... Args>
    std::shared_ptr<Node> make_node(Args&&... args) {
        auto nodePtr = std::make_shared<Node>(std::forward<Args>(args)...);
        nodes.push_back(nodePtr);
        return nodePtr;
    }

    std::shared_ptr<Node> make_corrupted();

    std::shared_ptr<Node> make_operator(Operator::Type type);

    std::shared_ptr<Node> make_literal(Literal::Type type, const char* value);

    std::shared_ptr<Node> make_identifier(const char* name);

    std::shared_ptr<Node> make_modifier(Modifier::Type type);

    std::shared_ptr<Node> make_basic_type(BasicType::Type type);

private:
    YYSTYPE yylval;
    std::vector<std::shared_ptr<Node>> nodes;
};
