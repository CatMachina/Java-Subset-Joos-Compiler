%{
    // TODO: C declarations
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
    | param_list
;

param_list: param
    | param COMMA param_list
;

param: type ID
;

//////////////////// Class Structure (Program Structure) ////////////////////

// Program Structure
program: package_decl import_decls type_decl
;

// Package Declaration
// package my.package;
package_decl: 
    | PACKAGE qualified_name SEMI
;

// Import Declarations (could be multiple)
// import java.util.Vector;
import_decls: 
    | import_decl import_decls
;

import_decl: IMPORT qualified_name SEMI
;

qualified_name: ID
    | qualified_name DOT ID
    | qualified_name DOT STAR
;

// Class and Interface Declarations
type_decl: class_decl
    | interface_decl
;

// Class Declaration
// public final class B extends A implements InterfaceA, InterfaceB {}
class_decl: modifiers CLASS ID superclass interfaces LBRACE class_body RBRACE;

// Modifiers
modifiers:
    | PUBLIC modifier_list
;

modifier_list: 
    | ABSTRACT
    | FINAL
;

// Superclass
superclass:
    | EXTENDS qualified_name
;

// Interfaces
interfaces:
    | IMPLEMENTS interface_list
;

interface_list: qualified_name
    | interface_list COMMA qualified_name
;

// Interface Declaration
// public interface MyInterface {}
interface_decl: PUBLIC INTERFACE ID LBRACE interface_body RBRACE
;

// Class Body
class_body: 
    | class_body_decl class_body
;

class_body_decl: field_decl
    | method_decl
    | constructor_decl
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
field_decl: field_modifiers type ID field_initializer SEMI
;

field_modifiers:
    | PUBLIC
    | PROTECTED
    | STATIC
    | PUBLIC STATIC
    | PROTECTED STATIC
;

field_initializer:
    | BECOMES expr
;

// Method Declarations
/*
protected abstract void compute();
public static void main(String[] args) {}
*/
method_decl: method_modifiers type ID LPAREN params RPAREN method_body
;

// did i miss anything?
method_modifiers:
    | PUBLIC
    | PROTECTED 
    | PUBLIC STATIC
    | PROTECTED STATIC
    | PUBLIC ABSTRACT
    | PROTECTED ABSTRACT
    | PUBLIC FINAL
    | PROTECTED FINAL
;

method_body: LBRACE statements RBRACE
    | SEMI
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
block: LBRACE statements RBRACE
;

//////////////////// Statements ////////////////////

statements:
    | statement statements
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
