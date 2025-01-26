%{
#include <iostream>
#include <string>

void process_token(const std::string& token_name, const std::string& token_value) {
    std::cout << "Token: " << token_name << ", Value: \"" << token_value << "\"" << std::endl;
}

%}

%option noyywrap

%x COMMENT

%%
"//".*

"/*"                                { BEGIN(COMMENT); }
<COMMENT>"*/"                       { BEGIN(INITIAL); }
<COMMENT>[^*]+                      { }
<COMMENT>"*"[^/]                    { }
<COMMENT>"*"                        { }

"abstract"                          { process_token("KEYWORD", yytext); }
"if"                                { process_token("KEYWORD", yytext); }
"this"                              { process_token("KEYWORD", yytext); }
"boolean"                           { process_token("KEYWORD", yytext); }
"do"                                { process_token("KEYWORD", yytext); }
"implements"                        { process_token("KEYWORD", yytext); }
"protected"                         { process_token("KEYWORD", yytext); }
"import"                            { process_token("KEYWORD", yytext); }
"public"                            { process_token("KEYWORD", yytext); }
"byte"                              { process_token("KEYWORD", yytext); }
"else"                              { process_token("KEYWORD", yytext); }
"instanceof"                        { process_token("KEYWORD", yytext); }
"return"                            { process_token("KEYWORD", yytext); }
"extends"                           { process_token("KEYWORD", yytext); }
"int"                               { process_token("KEYWORD", yytext); }
"short"                             { process_token("KEYWORD", yytext); }
"final"                             { process_token("KEYWORD", yytext); }
"interface"                         { process_token("KEYWORD", yytext); }
"static"                            { process_token("KEYWORD", yytext); }
"void"                              { process_token("KEYWORD", yytext); }
"char"                              { process_token("KEYWORD", yytext); }
"class"                             { process_token("KEYWORD", yytext); }
"native"                            { process_token("KEYWORD", yytext); }
"while"                             { process_token("KEYWORD", yytext); }
"const"                             { process_token("KEYWORD", yytext); }
"for"                               { process_token("KEYWORD", yytext); }
"package"                           { process_token("KEYWORD", yytext); }

"true"                              { process_token("BOOLEAN_LITERAL", yytext); }
"false"                             { process_token("BOOLEAN_LITERAL", yytext); }

\'[^\'\\]\'                         { process_token("CHARACTER_LITERAL", yytext); }
\\[btnfr\'\"\\]                     { process_token("CHARACTER_LITERAL", yytext); }

\"([^\"\\]|\\[btnfr\'\"\\])*\"      { process_token("STRING_LITERAL", yytext); }

"null"                              { process_token("NULL_LITERAL", yytext); }

"("                                 { process_token("SEPARATOR", yytext); }
")"                                 { process_token("SEPARATOR", yytext); }
"{"                                 { process_token("SEPARATOR", yytext); }
"}"                                 { process_token("SEPARATOR", yytext); }
"["                                 { process_token("SEPARATOR", yytext); }
"]"                                 { process_token("SEPARATOR", yytext); }
";"                                 { process_token("SEPARATOR", yytext); }
","                                 { process_token("SEPARATOR", yytext); }
"."                                 { process_token("SEPARATOR", yytext); }

"="                                 { process_token("OPERATOR", yytext); }
">"                                 { process_token("OPERATOR", yytext); }
"<"                                 { process_token("OPERATOR", yytext); }
"!"                                 { process_token("OPERATOR", yytext); }
"~"                                 { process_token("OPERATOR", yytext); }
"?"                                 { process_token("OPERATOR", yytext); }
":"                                 { process_token("OPERATOR", yytext); }
"=="                                { process_token("OPERATOR", yytext); }
"<="                                { process_token("OPERATOR", yytext); }
">="                                { process_token("OPERATOR", yytext); }
"!="                                { process_token("OPERATOR", yytext); }
"&&"                                { process_token("OPERATOR", yytext); }
"||"                                { process_token("OPERATOR", yytext); }
"++"                                { process_token("OPERATOR", yytext); }
"--"                                { process_token("OPERATOR", yytext); }
"+"                                 { process_token("OPERATOR", yytext); }
"-"                                 { process_token("OPERATOR", yytext); }
"*"                                 { process_token("OPERATOR", yytext); }
"/"                                 { process_token("OPERATOR", yytext); }
"&"                                 { process_token("OPERATOR", yytext); }
"|"                                 { process_token("OPERATOR", yytext); }
"^"                                 { process_token("OPERATOR", yytext); }
"%"                                 { process_token("OPERATOR", yytext); }
"<<"                                { process_token("OPERATOR", yytext); }
">>"                                { process_token("OPERATOR", yytext); }
">>>"                               { process_token("OPERATOR", yytext); }
"+="                                { process_token("OPERATOR", yytext); }
"-="                                { process_token("OPERATOR", yytext); }
"*="                                { process_token("OPERATOR", yytext); }
"/="                                { process_token("OPERATOR", yytext); }
"&="                                { process_token("OPERATOR", yytext); }
"|="                                { process_token("OPERATOR", yytext); }
"^="                                { process_token("OPERATOR", yytext); }
"%="                                { process_token("OPERATOR", yytext); }
"<<="                               { process_token("OPERATOR", yytext); }
">>="                               { process_token("OPERATOR", yytext); }
">>>="                              { process_token("OPERATOR", yytext); }

[a-zA-Z_][a-zA-Z0-9_]* {
    process_token("IDENTIFIER", yytext);
}

[ \t\n\r]+

.                                   { std::cout << "Unrecognized character: " << yytext << std::endl; }

%%

int main() {
    yylex();
    return 0;
}