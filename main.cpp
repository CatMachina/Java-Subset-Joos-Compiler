#include <iostream>
#include "ast/ast.hpp"
#include "weeder/weeder.hpp"

extern std::unique_ptr<ClassDecl> root;
extern int yyparse();

int main() {
    if (yyparse() == 0) {
        std::cout << "Parsing completed successfully!" << std::endl;

        try {
            Weeder weeder;
            weeder.weed(*root);

            std::cout << "Weeding completed successfully!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Weeder error: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Parsing failed." << std::endl;
        return 1;
    }

    return 0;
}
