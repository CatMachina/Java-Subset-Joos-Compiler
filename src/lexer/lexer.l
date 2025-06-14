%option never-interactive
%option c++ yyclass="myFlexLexer"
%option noyywrap
%option yylineno

%{
#include <iostream>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include "parser.tab.h"
#include "parseTree/parseTree.hpp"
#include "lexer/myFlexLexer.hpp"

void print_token(const std::string& token_name, const std::string& token_value) {
    // uncomment to debug
    // std::cout << "Token: " << token_name << ", Value: \"" << token_value << "\"" << std::endl;
}

int yycolumn = 1; // Track column location

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

using literalType = parsetree::Literal::Type;
using operatorType = parsetree::Operator::Type;
using modifierType = parsetree::Modifier::Type;
using basicType = parsetree::BasicType::Type;

%}


%x COMMENT

%%
"//".*

"/*"                              { BEGIN(COMMENT); }
<COMMENT>"*/"                     { BEGIN(INITIAL); }
<COMMENT>.                        { }
<COMMENT>\n                       { yycolumn = 1; }

"abstract"                          { yylval = make_modifier(modifierType::Abstract); print_token("MODIFIER", yytext); return ABSTRACT ; }
"if"                                { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return IF ; }
"this"                              { yylval = make_identifier(yytext); print_token("KEYWORD", yytext); return THIS; }
"boolean"                           { yylval = make_basic_type(basicType::Boolean); print_token("KEYWORD", yytext); return BOOLEAN ; }
"implements"                        { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return IMPLEMENTS ; }
"protected"                         { yylval = make_modifier(modifierType::Protected); print_token("MODIFIER", yytext); return PROTECTED ; }
"import"                            { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return IMPORT ; }
"public"                            { yylval = make_modifier(modifierType::Public); print_token("MODIFIER", yytext); return PUBLIC ; }
"byte"                              { yylval = make_basic_type(basicType::Byte); print_token("KEYWORD", yytext); return BYTE ; }
"else"                              { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return ELSE ; }
"instanceof"                        { yylval = make_operator(operatorType::InstanceOf); print_token("KEYWORD", yytext); return INSTANCEOF ; }
"return"                            { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return RETURN ; }
"extends"                           { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return EXTENDS ; }
"int"                               { yylval = make_basic_type(basicType::Int); print_token("KEYWORD", yytext); return INT ; }
"short"                             { yylval = make_basic_type(basicType::Short); print_token("KEYWORD", yytext); return SHORT ; }
"final"                             { yylval = make_modifier(modifierType::Final); print_token("MODIFIER", yytext); return FINAL ; }
"interface"                         { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return INTERFACE ; }
"static"                            { yylval = make_modifier(modifierType::Static); print_token("MODIFIER", yytext); return STATIC ; }
"void"                              { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return VOID ; }
"char"                              { yylval = make_basic_type(basicType::Char); print_token("KEYWORD", yytext); return CHAR ; }
"class"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return CLASS ; }
"while"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return WHILE ; }
"const"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return CONST ; }
"for"                               { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return FOR ; }
"package"                           { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return PACKAGE ; }
"new"                               { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return NEW ; }
"native"                            { yylval = make_modifier(modifierType::Native); print_token("KEYWORD", yytext); return NATIVE ; }


"private"                           { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return PRIVATE ; }
"goto"                              { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return GOTO ; }
"do"                                { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return DO ; }
"try"                               { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return TRY ; }
"catch"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return CATCH ; }
"finally"                           { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return FINALLY ; }
"throw"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return THROW ; }
"throws"                            { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return THROWS ; }
"transient"                         { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return TRANSIENT ; }
"switch"                            { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return SWITCH ; }
"break"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return BREAK ; }
"case"                              { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return CASE ; }
"default"                           { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return DEFAULT ; }
"synchronized"                      { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return SYNCHRONIZED ; }
"volatile"                          { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return VOLATILE ; }
"continue"                          { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return CONTINUE ; }
"double"                            { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return DOUBLE ; }
"float"                             { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return FLOAT ; }
"long"                              { yylval = make_corrupted(yytext); print_token("KEYWORD", yytext); return LONG ; }

"true"                              { yylval = make_literal(literalType::Boolean, yytext); print_token("BOOLEAN_LITERAL", yytext); return LITERAL ; }
"false"                             { yylval = make_literal(literalType::Boolean, yytext); print_token("BOOLEAN_LITERAL", yytext); return LITERAL ; }

\'[^\'\\]\'                         { yylval = make_literal(literalType::Character, yytext); print_token("CHARACTER_LITERAL", yytext); return LITERAL ; }
\'(\\[btnfr\'\"\\]|\\([0-7]|[0-7][0-7]|[0-3][0-7][0-7]))\'                 { yylval = make_literal(literalType::Character, yytext); print_token("CHARACTER_LITERAL", yytext); return LITERAL ; }

\"([^\"\\]|\\[btnfr\'\"\\]|\\([0-7]|[0-7][0-7]|[0-3][0-7][0-7]))*\"      { yylval = make_literal(literalType::String, yytext); print_token("STRING_LITERAL", yytext); return LITERAL ; }

"null"                              { yylval = make_literal(literalType::Null, yytext); print_token("NULL_LITERAL", yytext); return LITERAL ; }

"("                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return LPAREN ; }
")"                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return RPAREN ; }
"{"                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return LBRACE ; }
"}"                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return RBRACE ; }
"["                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return LBRACK ; }
"]"                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return RBRACK ; }
";"                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return SEMI ; }
","                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return COMMA ; }
"."                                 { yylval = make_corrupted(yytext); print_token("SEPARATOR", yytext); return DOT ; }

"="                                 { yylval = make_operator(operatorType::Assign); print_token("OPERATOR", yytext); return BECOMES ; }
">"                                 { yylval = make_operator(operatorType::GreaterThan); print_token("OPERATOR", yytext); return GT ; }
"<"                                 { yylval = make_operator(operatorType::LessThan); print_token("OPERATOR", yytext); return LT ; }
"!"                                 { yylval = make_operator(operatorType::Not); print_token("OPERATOR", yytext); return NOT ; }
"=="                                { yylval = make_operator(operatorType::Equal); print_token("OPERATOR", yytext); return EQ ; }
"<="                                { yylval = make_operator(operatorType::LessThanOrEqual); print_token("OPERATOR", yytext); return LE ; }
">="                                { yylval = make_operator(operatorType::GreaterThanOrEqual); print_token("OPERATOR", yytext); return GE ; }
"!="                                { yylval = make_operator(operatorType::NotEqual); print_token("OPERATOR", yytext); return NE ; }
"&&"                                { yylval = make_operator(operatorType::And); print_token("OPERATOR", yytext); return AND ; }
"||"                                { yylval = make_operator(operatorType::Or); print_token("OPERATOR", yytext); return OR ; }
"+"                                 { yylval = make_operator(operatorType::Plus); print_token("OPERATOR", yytext); return PLUS ; }
"-"                                 { yylval = make_operator(operatorType::Minus); print_token("OPERATOR", yytext); return MINUS ; }
"*"                                 { yylval = make_operator(operatorType::Multiply); print_token("OPERATOR", yytext); return STAR ; }
"/"                                 { yylval = make_operator(operatorType::Divide); print_token("OPERATOR", yytext); return SLASH ; }
"&"                                 { yylval = make_operator(operatorType::BitwiseAnd); print_token("OPERATOR", yytext); return VERT ; }
"|"                                 { yylval = make_operator(operatorType::BitwiseOr); print_token("OPERATOR", yytext); return AMP ; }
"%"                                 { yylval = make_operator(operatorType::Modulo); print_token("OPERATOR", yytext); return PCT ; }

(0|[1-9][0-9]*)                     { yylval = make_literal(literalType::Integer, yytext);; print_token("INTEGER_LITERAL", yytext); return LITERAL ; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval = make_identifier(yytext);
    print_token("IDENTIFIER", yytext);
    return ID;
}

[ \t\r]+
[ \n]+                              { yycolumn = 1; }

.                                   { std::cout << "Unrecognized character: " << yytext << std::endl; return YYUNDEF; }

%%

int yylex(YYSTYPE* lvalp, YYLTYPE* loc, myFlexLexer& lexer) {
    return lexer.bison_lex(lvalp, loc);
}

int myFlexLexer::bison_lex(YYSTYPE *lvalp, YYLTYPE* loc) {
    auto res = yylex();
    *lvalp = std::move(yylval);
    *loc = yylloc;
    return res;
}

static void yyerror(const char* s) {
    std::cerr << "Lexing error! : " << s << std::endl;
}
