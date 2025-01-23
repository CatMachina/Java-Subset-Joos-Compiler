%{
    // TODO: C++ declarations
    #include "ast.hpp"
    #include <memory>
    #include <vector>
    using namespace std;

    // Root of the AST
    unique_ptr<ClassDecl> root;
}

// AST node types
%union {
    int ival;
    char* sval;
    unique_ptr<Expr> expr;
    unique_ptr<Stmt> stmt;
    vector<unique_ptr<Stmt>>* stmts;
    unique_ptr<ClassDecl> class_decl;
    unique_ptr<FieldDecl> field_decl;
    unique_ptr<MethodDecl> method_decl;
    vector<unique_ptr<FieldDecl>>* fields;
    vector<unique_ptr<MethodDecl>>* methods;
}

// TODO: Bison declarations


// TODO: Token declarations

// Separators
%token COMMA SEMI

// Literals, Names and Operations (Lowest to Highest)
%left BECOMES
%left AND OR
%left AMP VERT
%left EQ NE
%left LT GT LE GE INSTANCEOF
%left PLUS MINUS
%left STAR SLASH PCT
%left UMINUS NOT
%precedence ID NUM TRUE FALSE NULL THIS SINGLECHAR ESCAPESEQ DOT LPAREN RPAREN LBRACK RBRACK

// Class Structure
%token NEW CLASS EXTENDS IMPLEMENTS STATIC IMPORT PACKAGE PUBLIC INTEFACE PROTECTED ABSTRACT FINAL

// Method and Field Access
%token LENGTH

// Modifiers (most covered in Class Structure)
%token NATIVE

// Method Structure
%token RETURN VOID

// Control Flow
%token IF ELSE WHILE FOR

// Types
%token BOOLEAN INT CHAR BYTE SHORT

// TODO: Grammar rules

%%

//////////////////// Params ////////////////////

// method(int arg1, bool arg2, ...)
params:
    param_list { $$ = std::move($1); }
    | /* Empty */ { $$ = std::vector<std::pair<std::string, std::string>>{}; }
;

param_list:
    param { $$ = std::vector<std::pair<std::string, std::string>>{$1}; }
    | param_list COMMA param {
        $1.push_back($3);
        $$ = std::move($1);
    }
;

param:
    type ID { $$ = std::make_pair($1, $2); }
;

//////////////////// Class Structure (Program Structure) ////////////////////

// Program Structure
program:
    package_decl import_decls type_decl {
        root = std::unique_ptr<ClassDecl>($3);
    }
;

// Package Declaration
// package my.package;
package_decl:
    PACKAGE qualified_name SEMI
    | /* Empty */
;

// Import Declarations (could be multiple)
// import java.util.Vector;
import_decls:
    import_decl import_decls
    | /* Empty */
;

import_decl:
    IMPORT qualified_name SEMI
;

qualified_name:
    ID { $$ = new std::string($1); }
    | qualified_name DOT ID { $$ = new std::string(*$1 + "." + $3); delete $1; }
;

// Class and Interface Declarations
type_decl:
    class_decl { $$ = std::move($1); }
    | interface_decl { $$ = std::move($1); }
;

// Class Declaration
// public final class B extends A implements InterfaceA, InterfaceB {}
class_decl:
    modifiers CLASS ID superclass interfaces LBRACE class_body RBRACE {
        $$ = std::make_unique<ClassDecl>(
            $3, // Class name
            $4 ? *$4 : "", // Superclass
            $5 ? *$5 : std::vector<std::string>{}, // Interfaces
            std::move($7.fields), // Fields
            std::move($7.methods) // Methods
        );
    }
;

// Modifiers
modifiers:
    PUBLIC { $$ = new std::vector<std::string>{"public"}; }
    | ABSTRACT { $$ = new std::vector<std::string>{"abstract"}; }
    | FINAL { $$ = new std::vector<std::string>{"final"}; }
    | PUBLIC ABSTRACT { $$ = new std::vector<std::string>{"public", "abstract"}; }
;

modifier_list: 
    ABSTRACT
    | FINAL
;

// Superclass
superclass:
    EXTENDS qualified_name { $$ = $2; }
    | /* Empty */ { $$ = nullptr; }
;

// Interfaces
interfaces:
    IMPLEMENTS interface_list { $$ = $2; }
    | /* Empty */ { $$ = nullptr; }
;

interface_list:
    qualified_name { $$ = new std::vector<std::string>{*$1}; delete $1; }
    | interface_list COMMA qualified_name {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
;

// Interface Declaration
// public interface MyInterface {}
interface_decl: PUBLIC INTERFACE ID LBRACE interface_body RBRACE
;

// Class Body
class_body:
    | class_body_decl class_body {
        $$ = std::move($2);
        $$->fields.insert($$->fields.end(), std::move($1.fields.begin()), std::move($1.fields.end()));
        $$->methods.insert($$->methods.end(), std::move($1.methods.begin()), std::move($1.methods.end()));
    }
;

class_body_decl:
    field_decl { $$ = {std::move($1), nullptr}; }
    | method_decl { $$ = {nullptr, std::move($1)}; }
;

// Interface Body
interface_body:
    | method_decl interface_body
;


// Field Declarations
/*
public int x;
protected static int y = 42;
int z;
*/
field_decl:
    field_modifiers type ID field_initializer SEMI {
        $$ = std::make_unique<FieldDecl>($3, $2, std::move($4));
    }
;

field_modifiers:
    PUBLIC
    | PROTECTED
    | STATIC
    | PUBLIC STATIC
    | PROTECTED STATIC
;

field_initializer:
    BECOMES expr { $$ = std::move($2); }
    | /* Empty */ { $$ = nullptr; }
;

// Method Declarations
/*
protected abstract void compute();
public static void main(String[] args) {}
*/
method_decl:
    method_modifiers type ID LPAREN params RPAREN method_body {
        $$ = std::make_unique<MethodDecl>(
            $3, // Method name
            $2, // Return type
            std::move($5), // Parameters
            std::move($7) // Body
        );
    }
;

// did i miss anything?
method_modifiers:
    PUBLIC
    | PROTECTED 
    | PUBLIC STATIC
    | PROTECTED STATIC
    | PUBLIC ABSTRACT
    | PROTECTED ABSTRACT
    | PUBLIC FINAL
    | PROTECTED FINAL
;

method_body:
    LBRACE statements RBRACE { $$ = std::make_unique<BlockStmt>(std::move($2)); }
    | SEMI { $$ = nullptr; } // Abstract or native methods
;

// Constructor Declarations
/*
public A() {}
public A(int x) { this.x = x; }
*/
constructor_decl: constructor_modifiers ID LPAREN params RPAREN LBRACE statements RBRACE
;

constructor_modifiers: PUBLIC
    | PROTECTED
;

//////////////////// Types ////////////////////

type: BOOLEAN
    | INT
    | CHAR
    | BYTE
    | SHORT
;

// Single-dimensional arrays
type: BOOLEAN LBRACK RBRACK
    | INT LBRACK RBRACK
    | CHAR LBRACK RBRACK
    | BYTE LBRACK RBRACK
    | SHORT LBRACK RBRACK
;

// Reference types (custom classes/interfaces)
type: ID                  // A obj;
    | ID LBRACK RBRACK    // java.util.Vector[] v;
;

// Void type for method return types
void_type: VOID
;

//////////////////// Literals ////////////////////

literal: NUM
	| TRUE
	| FALSE
	| char
	| SQUOTE char SQUOTE
	| DQUOTE chars DQUOTE
	| NULL
;

char: SINGLECHAR
    | ESCAPESEQ
;

chars:
    | chars char
;

//////////////////// Block ////////////////////

// { ... }
block:
    LBRACE statements RBRACE { $$ = std::make_unique<BlockStmt>(std::move($2)); }
;
;

//////////////////// Statements ////////////////////

statements:
    | statement statements { $2->push_back(std::move($1)); $$ = $2; }
    | /* Empty */ { $$ = new std::vector<std::unique_ptr<Stmt>>(); }
;

statement: block
    | SEMI
    | RETURN SEMI
    | RETURN expr SEMI
    | expr
    | IF LPARAN test RPARAN statement
	| IF LPARAN test RPARAN statement_no_short_if ELSE statement
	| WHILE LPARAN test RPARAN statement
	| WHILE LPARAN test RPARAN statement_no_short_if
	| FOR LPAREN for_init SEMI test SEMI for_update RPAREN statement
;

statement_no_short_if: block
    | SEMI
    | RETURN SEMI
    | RETURN expr SEMI
    | expr
    | IF LPARAN test RPARAN statement_no_short_if ELSE statement_no_short_if
	| WHILE LPARAN test RPARAN statement_no_short_if
    | FOR LPAREN for_init SEMI test SEMI for_update RPAREN statement_no_short_if
;

for_init: 
    | decl
    | expr
;

for_update:
    | expr
;

test: expr LT expr             // <
	| expr GT expr             // >
	| expr LE expr             // <=
	| expr GE expr             // >=
    | expr INSTANCEOF type     // instanceof
    | expr EQ expr             // ==
    | expr NE expr             // !=
    | expr && expr             // &&
    | expr || expr             // ||
;

//////////////////// Expressions ////////////////////
// Primary expr (Highest precedence)
expr: ID
    | NUM
    | TRUE
    | FALSE
    | NULL
    | THIS
    | expr DOT ID                       // Field access
    | expr DOT ID LPAREN args RPAREN    // Method call
    | expr LBRACK expr RBRACK           // Array access
    | NEW ID LPAREN args RPAREN         // Object creation
    | NEW type LBRACK expr RBRACK       // Array creation
    | LPAREN expr RPAREN                // Parentheses
;

// Unary operators
expr: '-' expr %prec UMINUS     // -x
    | NOT expr                  // !x
    | LPAREN type RPAREN expr   // (type) cast
;

// Multiplicative
expr: expr STAR expr           // *
    | expr SLASH expr          // /
    | expr PCT expr            // %
;

// Additive
expr: expr PLUS expr            // +
    | expr MINUS expr           // -
;

// Relational
expr: expr LT expr             // <
    | expr GT expr             // >
    | expr LE expr             // <=
    | expr GE expr             // >=
    | expr INSTANCEOF type     // instanceof
;

// Equalit
expr: expr EQ expr             // ==
    | expr NE expr             // !=
;

// Bitwise operators
expr: expr AMP expr             // &
    | expr VERT expr            // |
;

// Logical (lazy)
expr: expr AND expr             // &&
    | expr OR expr              // ||
;

// Assignment (Lowest precedence)
expr: lvalue BECOMES expr       // =
;

%%
