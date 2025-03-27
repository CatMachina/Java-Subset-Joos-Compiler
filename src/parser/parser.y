%debug
%locations

%code top {
    #include <iostream>
    #include <memory>
    #include "parser.tab.h"
    #include "parseTree/parseTree.hpp"
    #include "parseTree/sourceNode.hpp"
    #include "parser/myBisonParser.hpp"

    extern int yylex(YYSTYPE*, YYLTYPE*, myFlexLexer&);
    static void yyerror(YYLTYPE*, YYSTYPE*, myFlexLexer&, const char*);
}

%code requires {
    #include "parseTree/parseTree.hpp"
    #include "parseTree/sourceNode.hpp"
    #include <memory>
    namespace pt = parsetree;
    using NodeType = pt::Node::Type;
    class myFlexLexer;
}

%define api.pure full
%define api.location.type { source::SourceRange }
%define api.value.type { std::shared_ptr<pt::Node> }
%parse-param { std::shared_ptr<pt::Node>* ret }
%param { myFlexLexer& lexer }


// Separators
%token COMMA SEMI

// Literals
%token LITERAL
%token ID
%token THIS

// Literals, Names and Operations (Lowest to Highest)
%left BECOMES
%left AND OR
%left AMP VERT
%nonassoc LT GT LE GE INSTANCEOF EQ NE
%left PLUS MINUS
%left STAR SLASH PCT
%left UMINUS NOT
%left DOT
%nonassoc NUM TRUE FALSE SINGLECHAR ESCAPESEQ LPAREN RPAREN LBRACK RBRACK SQUOTE DQUOTE

// Class Structure
%token NEW CLASS EXTENDS IMPLEMENTS STATIC IMPORT PACKAGE PUBLIC INTERFACE PROTECTED ABSTRACT FINAL LBRACE RBRACE CONST

// Method and Field Access
// %token LENGTH

// Modifiers (most covered in Class Structure)
%token NATIVE

// Method Structure
%token RETURN VOID

// Control Flow
%token IF ELSE WHILE FOR

// Types
%token BOOLEAN INT CHAR BYTE SHORT

// Unused
%token SUPER DOUBLE FLOAT LONG DO TRY CATCH FINALLY THROW THROWS TRANSIENT CASE
%token SYNCHRONIZED VOLATILE GOTO CONTINUE BREAK DEFAULT SWITCH PRIVATE

%initial-action {
    (void) yynerrs;
    *ret = nullptr;
}

%start program

%%

//////////////////// Class Structure (Program Structure) ////////////////////

// Program Structure
program:
    package_decls import_decls_opt type_decl {
        (void) yynerrs;
        *ret = lexer.make_node(@$, NodeType::ProgramDecl, std::move($1), std::move($2), std::move($3));
    }
;

// Package Declaration
// package my.package;
package_decls:
    %empty { $$ = nullptr; }
    | package_decl
;

package_decl: PACKAGE qualified_name SEMI    { $$ = lexer.make_node(@$, NodeType::PackageDecl, std::move($2)); } 
;

// Import Declarations (could be multiple)
// import java.util.Vector;
import_decls_opt:
    %empty { $$ = nullptr; }
    | import_decls
;

import_decls:
    import_decl { $$ = lexer.make_node(@$, NodeType::ImportDeclList, std::move($1)); }
    | import_decls import_decl { $$ = lexer.make_node(@$, NodeType::ImportDeclList, std::move($1), std::move($2)); }
;

import_decl:
    IMPORT qualified_name SEMI { $$ = lexer.make_node(@$, NodeType::SingleImportDecl, std::move($2)); }
    | IMPORT qualified_name DOT STAR SEMI { $$ = lexer.make_node(@$, NodeType::MultiImportDecl, std::move($2)); }
;

// Class and Interface Declarations
type_decl:
    %empty { $$ = nullptr; }
    | class_decl
    | interface_decl
;

// Class Declaration
// public final class B extends A implements InterfaceA, InterfaceB {}
class_decl:
    modifiers_opt CLASS ID superclass interfaces LBRACE class_body RBRACE {
        $$ = lexer.make_node(@$, NodeType::ClassDecl, std::move($1), std::move($3), std::move($4), std::move($5), std::move($7) );
    }
;

// Modifiers
modifiers_opt:
    %empty { $$ = nullptr; }
    | modifiers
;

modifiers:
    modifier { $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1)); }
    | modifiers modifier { $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1), std::move($2)); }
;

modifier:
    PUBLIC
    | ABSTRACT
    | FINAL
;

// Superclass
superclass:
    %empty { $$ = nullptr; }
    | EXTENDS qualified_name { $$ = lexer.make_node(@$, NodeType::SuperClass, std::move($2)); }
;

// Interfaces
interfaces:
    %empty { $$ = nullptr; }
    | IMPLEMENTS interface_list { $$ = std::move($2); }
;

interface_list:
    qualified_name { $$ = lexer.make_node(@$, NodeType::InterfaceTypeList, std::move($1)); }
    | interface_list COMMA qualified_name {
        $$ = lexer.make_node(@$, NodeType::InterfaceTypeList, std::move($1), std::move($3));
    }
;

// Interface Declaration
// public interface MyInterface {}
interface_decl: modifiers_opt INTERFACE ID extends_interfaces_opt LBRACE interface_body RBRACE
    { $$ = lexer.make_node(@$, NodeType::InterfaceDecl, std::move($1), std::move($3), std::move($4), std::move($6)); }
;

extends_interfaces_opt:
    %empty { $$ = nullptr; }
    | extends_interfaces
;

extends_interfaces:
    EXTENDS qualified_name { $$ = lexer.make_node(@$, NodeType::InterfaceTypeList, std::move($2)); }
    | extends_interfaces COMMA qualified_name {
        $$ = lexer.make_node(@$, NodeType::InterfaceTypeList, std::move($1), std::move($3));
    }
;

// Class Body
class_body:
    %empty { $$ = nullptr; }
    | class_body_decl_list
;

class_body_decl_list:
    class_body_decl { $$ = lexer.make_node(@$, NodeType::ClassBodyDeclList, std::move($1)); }
    | class_body_decl_list class_body_decl {
        $$ = lexer.make_node(@$, NodeType::ClassBodyDeclList, std::move($1), std::move($2));
    }

class_body_decl:
    class_member_decl
    | constructor_decl
;

class_member_decl:
    field_decl
    | method_decl
;

// Interface Body
interface_body:
    %empty { $$ = nullptr; }
    | interface_body_decl_list
;

//| method_decl interface_body

interface_body_decl_list:
    abstract_method_decl { $$ = lexer.make_node(@$, NodeType::InterfaceBodyDeclList, std::move($1)); }
    | interface_body_decl_list abstract_method_decl {
        $$ = lexer.make_node(@$, NodeType::InterfaceBodyDeclList, std::move($1), std::move($2));
    }
;

abstract_method_decl:
    abstract_modifiers_opt type ID LPAREN params RPAREN SEMI {
        $$ = lexer.make_node(@$, NodeType::AbstractMethodDecl, std::move($1), std::move($2), std::move($3), std::move($5));
    }
    | abstract_modifiers_opt VOID ID LPAREN params RPAREN SEMI {
        $$ = lexer.make_node(@$, NodeType::AbstractMethodDecl, std::move($1), std::move($3), std::move($5));
        }
;

abstract_modifiers_opt:
    %empty { $$ = nullptr; }
    | abstract_modifiers
;

abstract_modifiers:
    abstract_modifier { $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1)); }
    | abstract_modifiers abstract_modifier {
        $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1), std::move($2));
    }
;

abstract_modifier:
    PUBLIC
    | ABSTRACT
;

// Field Declarations
/*
public int x;
protected static int y = 42;
int z;
*/
field_decl:
    method_modifiers_opt type var_name SEMI {
        $$ = lexer.make_node(@$, NodeType::FieldDecl, std::move($1), std::move($2), std::move($3));
    }
;

//////////////////// Params ////////////////////

// method(int arg1, bool arg2, ...)
params:
    %empty { $$ = nullptr; }
    | param_list
;

param_list:
    param { $$ = lexer.make_node(@$, NodeType::ParameterList, std::move($1)); }
    | param_list COMMA param {
        $$ = lexer.make_node(@$, NodeType::ParameterList, std::move($1), std::move($3));
    }
;

param:
    type ID { $$ = lexer.make_node(@$, NodeType::Parameter, std::move($1), std::move($2)); }
;

// Method Declarations
/*
protected abstract void compute();
public static void main(String[] args) {}
*/
method_decl:
    method_modifiers_opt VOID ID LPAREN params RPAREN method_body {
        $$ = lexer.make_node(@$, NodeType::MethodDecl, std::move($1), std::move($3), std::move($5), std::move($7));
    }
    | method_modifiers_opt type ID LPAREN params RPAREN method_body {
        $$ = lexer.make_node(@$, NodeType::MethodDecl, std::move($1), std::move($2), std::move($3), std::move($5), std::move($7));
    }
;

method_modifiers_opt:
    %empty { $$ = nullptr; }
    | method_modifiers
;

method_modifiers:
    method_modifier { $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1)); }
    | method_modifiers method_modifier { $$ = lexer.make_node(@$, NodeType::ModifierList, std::move($1), std::move($2)); }
;

method_modifier:
    PUBLIC
    | PROTECTED
    | STATIC
    | ABSTRACT
    | FINAL
    | NATIVE
;

method_body:
    block { $$ = std::move($1); }
    | SEMI { $$ = nullptr; } // Abstract or native methods
;

// Constructor Declarations
/*
public A() {}
public A(int x) { this.x = x; }
*/
constructor_decl: method_modifiers_opt ID LPAREN params RPAREN block {
    $$ = lexer.make_node(@$, NodeType::ConstructorDecl, std::move($1), std::move($2), std::move($4), std::move($6));
}
;

//////////////////// Types ////////////////////

type:
    non_array_type { $$ = std::move($1); }
    | array_type { $$ = std::move($1); }

non_array_type:
    qualified_name { $$ = lexer.make_node(@$, NodeType::Type, std::move($1)); }
    | basic_type { $$ = lexer.make_node(@$, NodeType::Type, std::move($1)); }
;

array_type:
    qualified_name LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayType, std::move($1)); }
    | basic_type LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayType, std::move($1)); }
;

object_type:
    qualified_name { $$ = lexer.make_node(@$, NodeType::Type, std::move($1)); } // A obj;
    | qualified_name LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayType, std::move($1)); } // java.util.Vector[] v;
    | basic_type LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayType, std::move($1)); }
    ;

basic_type: BOOLEAN
    | INT
    | CHAR
    | BYTE
    | SHORT
;

qualified_name:
    ID                                                                { $$ = lexer.make_node(@$, NodeType::QualifiedName, std::move($1)); }
    | qualified_name DOT ID                                        { $$ = lexer.make_node(@$, NodeType::QualifiedName, std::move($1), std::move($3)); }
    ;

//////////////////// Block ////////////////////

// { ... }
block:
    LBRACE statements_opt RBRACE { $$ = lexer.make_node(@$, NodeType::Block, std::move($2)); }
;

//////////////////// Statements ////////////////////

statements_opt:
    %empty { $$ = nullptr; }
    | statements { $$ = std::move($1); }
;

statements:
    statement { $$ = lexer.make_node(@$, NodeType::StatementList, std::move($1)); }
    | statements statement { $$ = lexer.make_node(@$, NodeType::StatementList, std::move($1), std::move($2)); }
;

statement:
    local_decl_statement
    | statement_no_substatement
    | if_then_statement
    | if_then_else_statement
    | while_statement
    | for_statement
;

statement_no_substatement:
    block
    | empty_statement
    | expr_statement
    | return_statement
;

statement_no_short_if:
    statement_no_substatement
    | if_then_else_statement_no_short_if
    | while_statement_no_short_if
    | for_statement_no_short_if
;

expr_statement:
    statement_expr SEMI { $$ = lexer.make_node(@$, NodeType::ExprStatement, std::move($1)); }
;

return_statement:
    RETURN expr_opt SEMI { $$ = lexer.make_node(@$, NodeType::ReturnStatement, std::move($2)); }
;

statement_expr:
    assignment
    | method_invocation_expr
    | class_create_expr
;

empty_statement:
    SEMI { $$ = lexer.make_node(@$, NodeType::Statement); }
;


//////////////////// Control Flows ////////////////////

if_then_statement:
    IF LPAREN expr RPAREN statement { $$ = lexer.make_node(@$, NodeType::IfStatement, std::move($3), std::move($5)); }
;

if_then_else_statement:
    IF LPAREN expr RPAREN statement_no_short_if ELSE statement { $$ = lexer.make_node(@$, NodeType::IfStatement, std::move($3), std::move($5), std::move($7)); }
;

if_then_else_statement_no_short_if:
    IF LPAREN expr RPAREN statement_no_short_if ELSE statement_no_short_if { $$ = lexer.make_node(@$, NodeType::IfStatement, std::move($3), std::move($5), std::move($7)); }
;

while_statement:
    WHILE LPAREN expr RPAREN statement { $$ = lexer.make_node(@$, NodeType::WhileStatement, std::move($3), std::move($5)); }
;

while_statement_no_short_if:
    WHILE LPAREN expr RPAREN statement_no_short_if { $$ = lexer.make_node(@$, NodeType::WhileStatement, std::move($3), std::move($5)); }
;

for_statement:
    FOR LPAREN for_init_opt SEMI expr_opt SEMI for_update_opt RPAREN statement {
        $$ = lexer.make_node(@$, NodeType::ForStatement, std::move($3), std::move($5), std::move($7), std::move($9));
    }
;

for_statement_no_short_if:
    FOR LPAREN for_init_opt SEMI expr_opt SEMI for_update_opt RPAREN statement_no_short_if {
        $$ = lexer.make_node(@$, NodeType::ForStatement, std::move($3), std::move($5), std::move($7), std::move($9));
    }
;

for_init_opt:
    %empty { $$ = lexer.make_node(@$, NodeType::Statement); }
    | local_decl { $$ = lexer.make_node(@$, NodeType::LocalDeclStatement, std::move($1)); }
    | statement_expr { $$ = lexer.make_node(@$, NodeType::ExprStatement, std::move($1)); }
;

for_update_opt:
    %empty { $$ = lexer.make_node(@$, NodeType::Statement); }
    | statement_expr { $$ = lexer.make_node(@$, NodeType::ExprStatement, std::move($1)); }
;

//////////////////// Expressions ////////////////////
expr_opt:
    %empty { $$ = nullptr; }
    | expr { $$ = std::move($1); }
;

expr:
    assignment_expr
;

assignment_expr:
    assignment
    | conditional_or_expr
;

// Assignment (Lowest precedence)
assignment: lvalue BECOMES assignment_expr { $$ = lexer.make_node(@$, NodeType::Assignment, std::move($1), std::move($2), std::move($3)); }
;

lvalue: qualified_name
    | field_access_expr
    | array_access_expr
;


// Logical (lazy)
conditional_or_expr:
    conditional_and_expr
    | conditional_or_expr OR conditional_and_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

conditional_and_expr:
    eager_or_expr
    | conditional_and_expr AND eager_or_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Eager Boolean
eager_or_expr:
    eager_and_expr
    | eager_or_expr VERT eager_and_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

eager_and_expr:
    equality_expr
    | eager_and_expr AMP equality_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Equality
equality_expr:
    relational_expr
    | equality_expr EQ relational_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | equality_expr NE relational_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Relational
relational_expr:
    additive_expr
    | relational_expr LT additive_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | relational_expr GT additive_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }    
    | relational_expr LE additive_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | relational_expr GE additive_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | relational_expr INSTANCEOF object_type { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Additive
additive_expr:
    multiplicative_expr
    | additive_expr PLUS multiplicative_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | additive_expr MINUS multiplicative_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Multiplicative
multiplicative_expr:
    unary_expr_negative
    | multiplicative_expr STAR unary_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | multiplicative_expr SLASH unary_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
    | multiplicative_expr PCT unary_expr { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2), std::move($3)); }
;

// Unary operators
unary_expr_negative:
    MINUS unary_expr_negative {
        // if literal we need to set it to negative
        if ($2->get_node_type() == NodeType::Literal) {
            auto literal = std::dynamic_pointer_cast<pt::Literal>($2);
            literal->setNegative();
        }
        $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2));
    }
    | unary_expr
;

unary_expr:
    post_fix_expr
    | NOT unary_expr_negative { $$ = lexer.make_node(@$, NodeType::Expression, std::move($1), std::move($2)); }
    | cast_expr
;

post_fix_expr:
    primary
    | qualified_name
;

cast_expr:
    LPAREN cast_type RPAREN unary_expr_negative { $$ = lexer.make_node(@$, NodeType::Cast, std::move($2), std::move($4)); }
    | LPAREN expr RPAREN unary_expr {
        // Cast is valid iff
        // $2 is a qualified name or an array type
        bool isQI = $2->get_node_type() == NodeType::QualifiedName;
        bool isArr = $2->get_node_type() == NodeType::ArrayCastType;
        bool hasOneChild = $2->num_children() == 1;
        if (isQI || (isArr && hasOneChild)) {
            $$ = lexer.make_node(@$, NodeType::Cast, std::move($2), std::move($4));
        } else {
            std::cerr << "Cast expression is not valid" << std::endl;
            $$ = lexer.make_corrupted("some cast expression");
        }
    }
;

cast_type:
    basic_type { $$ = lexer.make_node(@$, NodeType::Type, std::move($1)); }
    | basic_type LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayType, std::move($1)); }
;

//////////////////// Primary Expressions ////////////////////
primary:
    primary_without_array
    | array_create
    | array_access_expr
    | array_cast
;

primary_without_array:
    LITERAL
    | THIS
    | LPAREN expr RPAREN { $$ = lexer.make_node(@$, NodeType::Expression, std::move($2)); }
    | class_create_expr
    | field_access_expr
    | method_invocation_expr
    ;

array_create:
    NEW non_array_type LBRACK expr RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayCreation, std::move($2), std::move($4)); }
;

array_access_expr:
    primary_without_array LBRACK expr RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayAccess, std::move($1), std::move($3)); }
    | qualified_name LBRACK expr RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayAccess, std::move($1), std::move($3)); }
;

array_cast:
    qualified_name LBRACK RBRACK { $$ = lexer.make_node(@$, NodeType::ArrayCastType, std::move($1)); }
;

class_create_expr:
    NEW qualified_name LPAREN arg_list_opt RPAREN { $$ = lexer.make_node(@$, NodeType::ClassCreation, std::move($2), std::move($4)); }
;

field_access_expr:
    primary DOT ID {
        bool isExpression = $1->get_node_type() == parsetree::Node::Type::Expression;
        int numChildren = $1->num_children();
        if(isExpression && numChildren == 1 && $1->child_at(0)->get_node_type() == parsetree::Node::Type::QualifiedName) {
            std::cerr << "Invalid expression for field access" << std::endl;
            $$ = lexer.make_corrupted("some field access expression");
        } else {
            $$ = lexer.make_node(@$, NodeType::FieldAccess, std::move($1), std::move($3));
        }
    }
;

method_invocation_expr:
    qualified_name LPAREN arg_list_opt RPAREN { $$ = lexer.make_node(@$, NodeType::MethodInvocation, std::move($1), std::move($3)); }
    | primary DOT ID LPAREN arg_list_opt RPAREN { $$ = lexer.make_node(@$, NodeType::MethodInvocation, std::move($1), std::move($3), std::move($5)); }
;


arg_list_opt:
    %empty { $$ = nullptr; }
    | arg_list
;

arg_list:
    expr { $$ = lexer.make_node(@$, NodeType::ArgumentList, std::move($1)); }
    | arg_list COMMA expr { $$ = lexer.make_node(@$, NodeType::ArgumentList, std::move($1), std::move($3)); }
;


//////////////////// Var and Arr ////////////////////
local_decl_statement:
    local_decl SEMI { $$ = lexer.make_node(@$, NodeType::LocalDeclStatement, std::move($1)); }
;

local_decl:
    type var_name { $$ = lexer.make_node(@$, NodeType::LocalDecl, std::move($1), std::move($2)); }
;

var_name:
    ID { $$ = lexer.make_node(@$, NodeType::Variable, std::move($1)); }
    | ID BECOMES expr { $$ = lexer.make_node(@$, NodeType::Variable, std::move($1), std::move($3)); }
;



%%

std::string parser_resolve_token(int yysymbol) {
    if(yysymbol <= 0) return "EOF";
    if(yysymbol >= 256) {
        yysymbol -= 255;
        // check if yysymbol is greater than the end of the array
        if((unsigned) yysymbol >= sizeof(yytname)/sizeof(yytname[0])) {
            return std::string{"<unknown>"};
        }
        return std::string{
            yysymbol_name(static_cast<yysymbol_kind_t>(yysymbol))
        };
    } else {
        return std::string{static_cast<char>(yysymbol)};
    }
}

static void yyerror(YYLTYPE* loc, YYSTYPE* ret, myFlexLexer& lexer, const char* s) {
    (void) ret;
    std::cerr << "Parsing error! : " << s << std::endl;
    std::cerr << loc->fileID << " " << loc->first_line << ":" << loc->first_column << " - " << loc->last_line << ":" << loc->last_column << "\n";
}
