#pragma once

#include <FlexLexer.h>
#include "lexer/myFlexLexer.hpp"

#include <iostream>
#include <sstream>
#include <memory>

#include "parsetree/ParseTree.h"

class myBisonParser final {
public:
    explicit myBisonParser(const std::string& in)
        : lexer{}, buffer(nullptr, [](yy_buffer_state* buf) { if (buf) yy_delete_buffer(buf); }) {
        auto iss = std::make_unique<std::istringstream>(in);
        buffer.reset(lexer.yy_create_buffer(iss.get(), in.size()));
        lexer.yy_switch_to_buffer(buffer.get());
        input_stream = std::move(iss);
    }

    int yylex() {
        return lexer.yylex();
    }

    int parse(parsetree::Node*& ret) {
        ret = nullptr;
        return yyparse(&ret, lexer);
    }

private:
    myFlexLexer lexer;
    std::unique_ptr<yy_buffer_state, void(*)(yy_buffer_state*)> buffer;
    std::unique_ptr<std::istringstream> input_stream;
};
