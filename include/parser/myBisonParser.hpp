#pragma once

#include "lexer/myFlexLexer.hpp"
#include <FlexLexer.h>

#include <iostream>
#include <memory>
#include <sstream>

class myBisonParser final {
public:
  explicit myBisonParser(const std::string &in) : lexer{}, buffer(nullptr) {
    auto iss = std::make_unique<std::istringstream>(in);
    lexer.yy_switch_to_buffer(lexer.yy_create_buffer(iss.get(), in.size()));
    input_stream = std::move(iss);
  }

  int yylex() { return lexer.yylex(); }

  int parse(std::shared_ptr<parsetree::Node> &ret) {
    ret = nullptr;
    return yyparse(&ret, lexer);
  }

  void setFileID(int id) { lexer.setFileID(id); }

private:
  myFlexLexer lexer;
  std::shared_ptr<yy_buffer_state> buffer;
  std::unique_ptr<std::istringstream> input_stream;
};
