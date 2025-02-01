#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>

#include "../src/parser/myBisonParser.hpp"
#include "../src/parseTree/parseTree.hpp"

extern std::string parser_resolve_token(int yysymbol);
extern int yydebug;

int main()
{
  // Determine if input is piped
  const bool is_piped = !isatty(STDIN_FILENO);

  if (!is_piped)
  {
    std::cout
        << "This is the Scanner. Enter a string to be lexed followed by enter."
        << std::endl;
  }

  do
  {
    // Prompt for input if not piped
    if (!is_piped)
    {
      std::cout << "> ";
      std::cout.flush();
    }

    // Read input
    std::string input;
    if (!is_piped)
    {
      if (!std::getline(std::cin, input))
      {
        break; // Exit on EOF
      }
    }
    else
    {
      input.assign(std::istreambuf_iterator<char>(std::cin),
                   std::istreambuf_iterator<char>());
      if (input.empty())
      {
        break; // Exit on EOF
      }
    }

    // Initialize the parser with the input
    myBisonParser parser{input};

    // Parse
    const char *env_var = std::getenv("YYDEBUG"); // Replace with your environment variable name
    if (env_var) {
      yydebug = 1;
      std::cout << "====== DEBUG MODE ======\n";
    }
    std::shared_ptr<parsetree::Node> root = nullptr;
    parser.parse(root);

    std::cout << "====== Resulting Tree ======\n";
    root->print(std::cout);

  } while (!is_piped);

  return 0;
}
