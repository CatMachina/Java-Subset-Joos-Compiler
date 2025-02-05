#include <iostream>
#include <sstream>
#include <unistd.h>

#include "parser/myBisonParser.hpp"

extern std::string parser_resolve_token(int yysymbol);

int main() {
  // Determine if input is piped
  const bool is_piped = !isatty(STDIN_FILENO);

  if (!is_piped) {
    std::cout
        << "This is the Scanner. Enter a string to be lexed followed by enter."
        << std::endl;
  }

  do {
    // Prompt for input if not piped
    if (!is_piped) {
      std::cout << "> ";
      std::cout.flush();
    }

    // Read input
    std::string input;
    if (!is_piped) {
      if (!std::getline(std::cin, input)) {
        break; // Exit on EOF
      }
    } else {
      input.assign(std::istreambuf_iterator<char>(std::cin),
                   std::istreambuf_iterator<char>());
      if (input.empty()) {
        break; // Exit on EOF
      }
    }

    // Initialize the parser with the input
    myBisonParser parser{input};

    // Lex and print tokens
    while (int token = parser.yylex()) {
      std::cout << token << ": " << parser_resolve_token(token) << std::endl;
    }

  } while (!is_piped);

  return 0;
}
