/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* "%code top" blocks.  */
#line 1 "parser.y"

    #include <iostream>
    #include "parser.tab.hpp"
    #include "parsetree/parseTree.hpp"
    #include "parser/myBisonParser.hpp"

    extern int yylex(YYSTYPE*, myFlexLexer&);
    static void yyerror(YYLTYPE*, myFlexLexer&, const char*);

#line 78 "parser.tab.c"




# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 11 "parser.y"

    #include "parsetree/parseTree.h"
    namespace pt = parseTree;
    using NodeType = parsetree::Node::Type;
    class myFlexLexer;

#line 120 "parser.tab.c"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    COMMA = 258,                   /* COMMA  */
    SEMI = 259,                    /* SEMI  */
    LITERAL = 260,                 /* LITERAL  */
    ID = 261,                      /* ID  */
    THIS = 262,                    /* THIS  */
    BECOMES = 263,                 /* BECOMES  */
    AND = 264,                     /* AND  */
    OR = 265,                      /* OR  */
    AMP = 266,                     /* AMP  */
    VERT = 267,                    /* VERT  */
    LT = 268,                      /* LT  */
    GT = 269,                      /* GT  */
    LE = 270,                      /* LE  */
    GE = 271,                      /* GE  */
    INSTANCEOF = 272,              /* INSTANCEOF  */
    EQ = 273,                      /* EQ  */
    NE = 274,                      /* NE  */
    PLUS = 275,                    /* PLUS  */
    MINUS = 276,                   /* MINUS  */
    STAR = 277,                    /* STAR  */
    SLASH = 278,                   /* SLASH  */
    PCT = 279,                     /* PCT  */
    UMINUS = 280,                  /* UMINUS  */
    NOT = 281,                     /* NOT  */
    DOT = 282,                     /* DOT  */
    NUM = 283,                     /* NUM  */
    TRUE = 284,                    /* TRUE  */
    FALSE = 285,                   /* FALSE  */
    NULL = 286,                    /* NULL  */
    SINGLECHAR = 287,              /* SINGLECHAR  */
    ESCAPESEQ = 288,               /* ESCAPESEQ  */
    LPAREN = 289,                  /* LPAREN  */
    RPAREN = 290,                  /* RPAREN  */
    LBRACK = 291,                  /* LBRACK  */
    RBRACK = 292,                  /* RBRACK  */
    SQUOTE = 293,                  /* SQUOTE  */
    DQUOTE = 294,                  /* DQUOTE  */
    NEW = 295,                     /* NEW  */
    CLASS = 296,                   /* CLASS  */
    EXTENDS = 297,                 /* EXTENDS  */
    IMPLEMENTS = 298,              /* IMPLEMENTS  */
    STATIC = 299,                  /* STATIC  */
    IMPORT = 300,                  /* IMPORT  */
    PACKAGE = 301,                 /* PACKAGE  */
    PUBLIC = 302,                  /* PUBLIC  */
    INTERFACE = 303,               /* INTERFACE  */
    PROTECTED = 304,               /* PROTECTED  */
    ABSTRACT = 305,                /* ABSTRACT  */
    FINAL = 306,                   /* FINAL  */
    LBRACE = 307,                  /* LBRACE  */
    RBRACE = 308,                  /* RBRACE  */
    LENGTH = 309,                  /* LENGTH  */
    NATIVE = 310,                  /* NATIVE  */
    RETURN = 311,                  /* RETURN  */
    VOID = 312,                    /* VOID  */
    IF = 313,                      /* IF  */
    ELSE = 314,                    /* ELSE  */
    WHILE = 315,                   /* WHILE  */
    FOR = 316,                     /* FOR  */
    BOOLEAN = 317,                 /* BOOLEAN  */
    INT = 318,                     /* INT  */
    CHAR = 319,                    /* CHAR  */
    BYTE = 320,                    /* BYTE  */
    SHORT = 321                    /* SHORT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef  pt::Node*  YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (pt::Node** res, myFlexLexer& lexer);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_COMMA = 3,                      /* COMMA  */
  YYSYMBOL_SEMI = 4,                       /* SEMI  */
  YYSYMBOL_LITERAL = 5,                    /* LITERAL  */
  YYSYMBOL_ID = 6,                         /* ID  */
  YYSYMBOL_THIS = 7,                       /* THIS  */
  YYSYMBOL_BECOMES = 8,                    /* BECOMES  */
  YYSYMBOL_AND = 9,                        /* AND  */
  YYSYMBOL_OR = 10,                        /* OR  */
  YYSYMBOL_AMP = 11,                       /* AMP  */
  YYSYMBOL_VERT = 12,                      /* VERT  */
  YYSYMBOL_LT = 13,                        /* LT  */
  YYSYMBOL_GT = 14,                        /* GT  */
  YYSYMBOL_LE = 15,                        /* LE  */
  YYSYMBOL_GE = 16,                        /* GE  */
  YYSYMBOL_INSTANCEOF = 17,                /* INSTANCEOF  */
  YYSYMBOL_EQ = 18,                        /* EQ  */
  YYSYMBOL_NE = 19,                        /* NE  */
  YYSYMBOL_PLUS = 20,                      /* PLUS  */
  YYSYMBOL_MINUS = 21,                     /* MINUS  */
  YYSYMBOL_STAR = 22,                      /* STAR  */
  YYSYMBOL_SLASH = 23,                     /* SLASH  */
  YYSYMBOL_PCT = 24,                       /* PCT  */
  YYSYMBOL_UMINUS = 25,                    /* UMINUS  */
  YYSYMBOL_NOT = 26,                       /* NOT  */
  YYSYMBOL_DOT = 27,                       /* DOT  */
  YYSYMBOL_NUM = 28,                       /* NUM  */
  YYSYMBOL_TRUE = 29,                      /* TRUE  */
  YYSYMBOL_FALSE = 30,                     /* FALSE  */
  YYSYMBOL_NULL = 31,                      /* NULL  */
  YYSYMBOL_SINGLECHAR = 32,                /* SINGLECHAR  */
  YYSYMBOL_ESCAPESEQ = 33,                 /* ESCAPESEQ  */
  YYSYMBOL_LPAREN = 34,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 35,                    /* RPAREN  */
  YYSYMBOL_LBRACK = 36,                    /* LBRACK  */
  YYSYMBOL_RBRACK = 37,                    /* RBRACK  */
  YYSYMBOL_SQUOTE = 38,                    /* SQUOTE  */
  YYSYMBOL_DQUOTE = 39,                    /* DQUOTE  */
  YYSYMBOL_NEW = 40,                       /* NEW  */
  YYSYMBOL_CLASS = 41,                     /* CLASS  */
  YYSYMBOL_EXTENDS = 42,                   /* EXTENDS  */
  YYSYMBOL_IMPLEMENTS = 43,                /* IMPLEMENTS  */
  YYSYMBOL_STATIC = 44,                    /* STATIC  */
  YYSYMBOL_IMPORT = 45,                    /* IMPORT  */
  YYSYMBOL_PACKAGE = 46,                   /* PACKAGE  */
  YYSYMBOL_PUBLIC = 47,                    /* PUBLIC  */
  YYSYMBOL_INTERFACE = 48,                 /* INTERFACE  */
  YYSYMBOL_PROTECTED = 49,                 /* PROTECTED  */
  YYSYMBOL_ABSTRACT = 50,                  /* ABSTRACT  */
  YYSYMBOL_FINAL = 51,                     /* FINAL  */
  YYSYMBOL_LBRACE = 52,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 53,                    /* RBRACE  */
  YYSYMBOL_LENGTH = 54,                    /* LENGTH  */
  YYSYMBOL_NATIVE = 55,                    /* NATIVE  */
  YYSYMBOL_RETURN = 56,                    /* RETURN  */
  YYSYMBOL_VOID = 57,                      /* VOID  */
  YYSYMBOL_IF = 58,                        /* IF  */
  YYSYMBOL_ELSE = 59,                      /* ELSE  */
  YYSYMBOL_WHILE = 60,                     /* WHILE  */
  YYSYMBOL_FOR = 61,                       /* FOR  */
  YYSYMBOL_BOOLEAN = 62,                   /* BOOLEAN  */
  YYSYMBOL_INT = 63,                       /* INT  */
  YYSYMBOL_CHAR = 64,                      /* CHAR  */
  YYSYMBOL_BYTE = 65,                      /* BYTE  */
  YYSYMBOL_SHORT = 66,                     /* SHORT  */
  YYSYMBOL_67_ = 67,                       /* '.'  */
  YYSYMBOL_68_ = 68,                       /* ';'  */
  YYSYMBOL_YYACCEPT = 69,                  /* $accept  */
  YYSYMBOL_program = 70,                   /* program  */
  YYSYMBOL_package_decls = 71,             /* package_decls  */
  YYSYMBOL_package_decl = 72,              /* package_decl  */
  YYSYMBOL_import_decls_opt = 73,          /* import_decls_opt  */
  YYSYMBOL_import_decls = 74,              /* import_decls  */
  YYSYMBOL_import_decl = 75,               /* import_decl  */
  YYSYMBOL_type_decl = 76,                 /* type_decl  */
  YYSYMBOL_class_decl = 77,                /* class_decl  */
  YYSYMBOL_modifiers_opt = 78,             /* modifiers_opt  */
  YYSYMBOL_modifiers = 79,                 /* modifiers  */
  YYSYMBOL_modifier = 80,                  /* modifier  */
  YYSYMBOL_superclass = 81,                /* superclass  */
  YYSYMBOL_interfaces = 82,                /* interfaces  */
  YYSYMBOL_interface_list = 83,            /* interface_list  */
  YYSYMBOL_interface_decl = 84,            /* interface_decl  */
  YYSYMBOL_extends_interfaces_opt = 85,    /* extends_interfaces_opt  */
  YYSYMBOL_extends_interfaces = 86,        /* extends_interfaces  */
  YYSYMBOL_class_body = 87,                /* class_body  */
  YYSYMBOL_class_body_decl_list = 88,      /* class_body_decl_list  */
  YYSYMBOL_class_body_decl = 89,           /* class_body_decl  */
  YYSYMBOL_class_member_decl = 90,         /* class_member_decl  */
  YYSYMBOL_interface_body = 91,            /* interface_body  */
  YYSYMBOL_interface_body_decl_list = 92,  /* interface_body_decl_list  */
  YYSYMBOL_abstract_method_decl = 93,      /* abstract_method_decl  */
  YYSYMBOL_abstract_modifiers_opt = 94,    /* abstract_modifiers_opt  */
  YYSYMBOL_abstract_modifiers = 95,        /* abstract_modifiers  */
  YYSYMBOL_abstract_modifier = 96,         /* abstract_modifier  */
  YYSYMBOL_field_decl = 97,                /* field_decl  */
  YYSYMBOL_params = 98,                    /* params  */
  YYSYMBOL_param_list = 99,                /* param_list  */
  YYSYMBOL_param = 100,                    /* param  */
  YYSYMBOL_method_decl = 101,              /* method_decl  */
  YYSYMBOL_method_modifiers_opt = 102,     /* method_modifiers_opt  */
  YYSYMBOL_method_modifiers = 103,         /* method_modifiers  */
  YYSYMBOL_method_modifier = 104,          /* method_modifier  */
  YYSYMBOL_method_body = 105,              /* method_body  */
  YYSYMBOL_constructor_decl = 106,         /* constructor_decl  */
  YYSYMBOL_type = 107,                     /* type  */
  YYSYMBOL_object_type = 108,              /* object_type  */
  YYSYMBOL_basic_type = 109,               /* basic_type  */
  YYSYMBOL_qualified_name = 110,           /* qualified_name  */
  YYSYMBOL_block = 111,                    /* block  */
  YYSYMBOL_statements_opt = 112,           /* statements_opt  */
  YYSYMBOL_statements = 113,               /* statements  */
  YYSYMBOL_statement = 114,                /* statement  */
  YYSYMBOL_statement_no_substatement = 115, /* statement_no_substatement  */
  YYSYMBOL_statement_no_short_if = 116,    /* statement_no_short_if  */
  YYSYMBOL_expr_statement = 117,           /* expr_statement  */
  YYSYMBOL_return_statement = 118,         /* return_statement  */
  YYSYMBOL_statement_expr = 119,           /* statement_expr  */
  YYSYMBOL_empty_statement = 120,          /* empty_statement  */
  YYSYMBOL_if_then_statement = 121,        /* if_then_statement  */
  YYSYMBOL_if_then_else_statement = 122,   /* if_then_else_statement  */
  YYSYMBOL_if_then_else_statement_no_short_if = 123, /* if_then_else_statement_no_short_if  */
  YYSYMBOL_while_statement = 124,          /* while_statement  */
  YYSYMBOL_while_statement_no_short_if = 125, /* while_statement_no_short_if  */
  YYSYMBOL_for_statement = 126,            /* for_statement  */
  YYSYMBOL_for_statement_no_short_if = 127, /* for_statement_no_short_if  */
  YYSYMBOL_for_init_opt = 128,             /* for_init_opt  */
  YYSYMBOL_for_update_opt = 129,           /* for_update_opt  */
  YYSYMBOL_expr_opt = 130,                 /* expr_opt  */
  YYSYMBOL_expr = 131,                     /* expr  */
  YYSYMBOL_assignment_expr = 132,          /* assignment_expr  */
  YYSYMBOL_assignment = 133,               /* assignment  */
  YYSYMBOL_lvalue = 134,                   /* lvalue  */
  YYSYMBOL_conditional_or_expr = 135,      /* conditional_or_expr  */
  YYSYMBOL_conditional_and_expr = 136,     /* conditional_and_expr  */
  YYSYMBOL_eager_or_expr = 137,            /* eager_or_expr  */
  YYSYMBOL_eager_and_expr = 138,           /* eager_and_expr  */
  YYSYMBOL_equality_expr = 139,            /* equality_expr  */
  YYSYMBOL_relational_expr = 140,          /* relational_expr  */
  YYSYMBOL_additive_expr = 141,            /* additive_expr  */
  YYSYMBOL_multiplicative_expr = 142,      /* multiplicative_expr  */
  YYSYMBOL_unary_expr_negative = 143,      /* unary_expr_negative  */
  YYSYMBOL_unary_expr = 144,               /* unary_expr  */
  YYSYMBOL_post_fix_expr = 145,            /* post_fix_expr  */
  YYSYMBOL_cast_expr = 146,                /* cast_expr  */
  YYSYMBOL_cast_type = 147,                /* cast_type  */
  YYSYMBOL_primary = 148,                  /* primary  */
  YYSYMBOL_primary_without_array = 149,    /* primary_without_array  */
  YYSYMBOL_array_create = 150,             /* array_create  */
  YYSYMBOL_array_access_expr = 151,        /* array_access_expr  */
  YYSYMBOL_152_1 = 152,                    /* @1  */
  YYSYMBOL_array_cast = 153,               /* array_cast  */
  YYSYMBOL_class_create_expr = 154,        /* class_create_expr  */
  YYSYMBOL_field_access_expr = 155,        /* field_access_expr  */
  YYSYMBOL_method_invocation_expr = 156,   /* method_invocation_expr  */
  YYSYMBOL_arg_list_opt = 157,             /* arg_list_opt  */
  YYSYMBOL_arg_list = 158,                 /* arg_list  */
  YYSYMBOL_local_decl_statement = 159,     /* local_decl_statement  */
  YYSYMBOL_local_decl = 160,               /* local_decl  */
  YYSYMBOL_var_name = 161                  /* var_name  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  7
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   498

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  93
/* YYNRULES -- Number of rules.  */
#define YYNRULES  193
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  332

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   321


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    67,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    68,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    74,    74,    83,    84,    87,    93,    94,    98,    99,
     103,   104,   109,   110,   111,   117,   124,   125,   129,   130,
     134,   135,   136,   141,   142,   147,   148,   152,   153,   160,
     164,   165,   169,   170,   177,   178,   182,   183,   188,   189,
     193,   194,   199,   200,   206,   207,   213,   216,   222,   223,
     227,   228,   234,   235,   245,   254,   255,   259,   260,   266,
     275,   278,   284,   285,   289,   290,   294,   295,   296,   297,
     298,   302,   303,   311,   319,   320,   321,   322,   326,   327,
     328,   331,   332,   333,   334,   335,   339,   340,   347,   353,
     354,   358,   359,   363,   364,   365,   366,   367,   368,   372,
     373,   374,   375,   379,   380,   381,   382,   386,   390,   394,
     395,   396,   400,   407,   411,   415,   419,   423,   427,   433,
     439,   440,   441,   445,   446,   451,   452,   456,   460,   461,
     465,   468,   469,   470,   476,   477,   481,   482,   487,   488,
     492,   493,   498,   499,   500,   505,   506,   507,   508,   509,
     510,   515,   516,   517,   522,   523,   524,   525,   530,   531,
     535,   536,   537,   541,   542,   546,   550,   551,   556,   557,
     558,   559,   563,   564,   565,   566,   567,   568,   572,   576,
     576,   581,   585,   589,   593,   594,   599,   600,   604,   605,
     611,   615,   619,   620
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "COMMA", "SEMI",
  "LITERAL", "ID", "THIS", "BECOMES", "AND", "OR", "AMP", "VERT", "LT",
  "GT", "LE", "GE", "INSTANCEOF", "EQ", "NE", "PLUS", "MINUS", "STAR",
  "SLASH", "PCT", "UMINUS", "NOT", "DOT", "NUM", "TRUE", "FALSE", "NULL",
  "SINGLECHAR", "ESCAPESEQ", "LPAREN", "RPAREN", "LBRACK", "RBRACK",
  "SQUOTE", "DQUOTE", "NEW", "CLASS", "EXTENDS", "IMPLEMENTS", "STATIC",
  "IMPORT", "PACKAGE", "PUBLIC", "INTERFACE", "PROTECTED", "ABSTRACT",
  "FINAL", "LBRACE", "RBRACE", "LENGTH", "NATIVE", "RETURN", "VOID", "IF",
  "ELSE", "WHILE", "FOR", "BOOLEAN", "INT", "CHAR", "BYTE", "SHORT", "'.'",
  "';'", "$accept", "program", "package_decls", "package_decl",
  "import_decls_opt", "import_decls", "import_decl", "type_decl",
  "class_decl", "modifiers_opt", "modifiers", "modifier", "superclass",
  "interfaces", "interface_list", "interface_decl",
  "extends_interfaces_opt", "extends_interfaces", "class_body",
  "class_body_decl_list", "class_body_decl", "class_member_decl",
  "interface_body", "interface_body_decl_list", "abstract_method_decl",
  "abstract_modifiers_opt", "abstract_modifiers", "abstract_modifier",
  "field_decl", "params", "param_list", "param", "method_decl",
  "method_modifiers_opt", "method_modifiers", "method_modifier",
  "method_body", "constructor_decl", "type", "object_type", "basic_type",
  "qualified_name", "block", "statements_opt", "statements", "statement",
  "statement_no_substatement", "statement_no_short_if", "expr_statement",
  "return_statement", "statement_expr", "empty_statement",
  "if_then_statement", "if_then_else_statement",
  "if_then_else_statement_no_short_if", "while_statement",
  "while_statement_no_short_if", "for_statement",
  "for_statement_no_short_if", "for_init_opt", "for_update_opt",
  "expr_opt", "expr", "assignment_expr", "assignment", "lvalue",
  "conditional_or_expr", "conditional_and_expr", "eager_or_expr",
  "eager_and_expr", "equality_expr", "relational_expr", "additive_expr",
  "multiplicative_expr", "unary_expr_negative", "unary_expr",
  "post_fix_expr", "cast_expr", "cast_type", "primary",
  "primary_without_array", "array_create", "array_access_expr", "@1",
  "array_cast", "class_create_expr", "field_access_expr",
  "method_invocation_expr", "arg_list_opt", "arg_list",
  "local_decl_statement", "local_decl", "var_name", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-298)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-182)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -41,    14,    23,   -14,  -298,  -298,    28,  -298,    14,     1,
     -14,  -298,  -298,   116,    31,  -298,  -298,  -298,  -298,  -298,
     101,   183,  -298,  -298,  -298,  -298,  -298,     3,   125,   144,
    -298,    86,   117,   149,  -298,    14,   159,    14,   156,   207,
     153,    14,   177,   153,   261,    14,   239,   153,   244,  -298,
    -298,   190,   262,  -298,   141,   -32,  -298,   153,    14,  -298,
    -298,  -298,  -298,  -298,   192,   388,  -298,  -298,  -298,  -298,
     152,   252,  -298,  -298,  -298,  -298,   259,  -298,  -298,  -298,
    -298,  -298,   263,   235,    63,  -298,   153,  -298,  -298,   238,
     283,   286,  -298,   266,   270,   282,   284,   213,   279,    -4,
     318,   213,   213,  -298,  -298,   288,   321,  -298,   327,   213,
     111,   213,  -298,   300,   313,   304,   213,  -298,   322,  -298,
    -298,   234,   111,   410,    14,   121,  -298,  -298,  -298,   350,
     353,   356,   359,   363,   108,   330,   123,   240,  -298,  -298,
    -298,  -298,   345,   339,  -298,   368,  -298,  -298,   369,  -298,
     346,   379,   380,   276,  -298,  -298,   110,   -20,  -298,  -298,
    -298,  -298,   349,   351,   352,   -28,   111,   355,   111,   111,
     111,   111,   111,   111,   111,   111,   111,   111,   111,   213,
     111,   111,   234,   234,   234,   383,   111,   110,  -298,  -298,
    -298,   111,   111,   360,   362,   371,   391,   127,  -298,   354,
    -298,   276,  -298,  -298,  -298,   399,  -298,  -298,  -298,  -298,
    -298,  -298,   345,   -17,   -10,  -298,   414,  -298,  -298,  -298,
     382,  -298,   111,   111,   111,  -298,   387,   425,  -298,  -298,
     356,   359,   363,   108,   330,   330,   123,   123,   123,   123,
    -298,   393,    73,   240,   240,  -298,  -298,  -298,   396,   397,
    -298,   429,  -298,   111,   111,   361,   434,  -298,   406,  -298,
    -298,  -298,  -298,  -298,  -298,   408,   394,  -298,   111,   409,
     411,   111,  -298,  -298,   418,   420,  -298,   445,  -298,   430,
    -298,  -298,  -298,  -298,  -298,   421,    14,   348,   276,   111,
    -298,    97,   424,   426,   427,  -298,   403,   405,  -298,  -298,
    -298,  -298,   461,   111,   111,   111,   361,   276,   250,   431,
     432,   435,   462,  -298,   -20,  -298,   436,  -298,   348,   348,
     111,   276,   419,  -298,   465,  -298,   348,   250,  -298,   442,
     348,  -298
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     6,     4,    86,     0,     1,     0,    16,
       7,     8,     5,     0,     0,    20,    21,    22,     2,    13,
       0,    17,    18,    14,     9,    87,    10,     0,     0,     0,
      19,     0,    23,    30,    11,     0,    25,     0,     0,    31,
      24,     0,     0,    32,    48,     0,    26,    27,    62,    52,
      53,     0,    48,    44,     0,    49,    50,    33,     0,    68,
      66,    67,    69,    70,     0,    62,    36,    38,    40,    41,
       0,    63,    64,    39,    29,    45,     0,    81,    82,    83,
      84,    85,     0,    75,    74,    51,    28,    15,    37,    86,
       0,     0,    65,     0,     0,     0,     0,    55,     0,   192,
       0,    55,    55,    77,    76,     0,    56,    57,     0,    55,
       0,    55,    54,     0,     0,     0,     0,    59,     0,   172,
     173,     0,     0,     0,     0,   164,   193,   127,   128,     0,
     129,   134,   136,   138,   140,   142,   145,   151,   154,   159,
     160,   162,   163,   168,   169,   170,   171,   175,   176,   177,
       0,     0,     0,    89,    73,    58,     0,   164,   158,   170,
     176,   161,   166,     0,     0,     0,   186,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    47,    46,
     112,     0,   125,     0,     0,     0,     0,    74,    99,     0,
      90,    91,    94,   101,   102,     0,   100,    95,    96,    97,
      98,   109,     0,   111,   110,    93,     0,    72,    60,    71,
       0,   174,     0,     0,   186,   188,     0,   187,   181,   130,
     135,   137,   139,   141,   143,   144,   146,   147,   148,   149,
     150,     0,    78,   152,   153,   155,   156,   157,   183,     0,
      61,     0,   126,     0,     0,   120,   192,   191,     0,    88,
      92,   107,   190,   167,   165,     0,     0,   184,     0,     0,
       0,   186,   179,   108,     0,     0,   122,     0,   121,    76,
     178,   182,   189,    80,    79,     0,     0,     0,     0,   125,
     185,     0,     0,     0,     0,   113,    94,     0,   104,   105,
     106,   116,     0,     0,     0,     0,   120,     0,   123,     0,
       0,     0,     0,   114,   131,   124,     0,   180,     0,     0,
     125,     0,     0,   117,     0,   118,     0,   123,   115,     0,
       0,   119
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -298,  -298,  -298,  -298,  -298,  -298,   469,  -298,  -298,  -298,
    -298,   459,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
     416,  -298,  -298,  -298,   433,  -298,  -298,   428,  -298,    88,
    -298,   366,  -298,  -298,  -298,   413,   299,  -298,    37,  -298,
     -68,    -1,  -102,  -298,   287,   133,  -281,  -297,  -298,  -298,
    -214,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,   181,
     162,  -153,   -67,   323,   -95,  -298,  -298,   324,   320,   325,
     326,    -8,    74,     4,  -120,   -79,  -298,  -298,  -298,   -60,
    -298,  -298,  -110,  -298,  -298,    43,   -94,    72,  -221,  -298,
    -209,  -298,   296
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,     3,     4,     9,    10,    11,    18,    19,    20,
      21,    22,    36,    42,    46,    23,    38,    39,    64,    65,
      66,    67,    51,    52,    53,    54,    55,    56,    68,   105,
     106,   107,    69,    70,    71,    72,   218,    73,   196,   240,
      83,   125,   198,   199,   200,   201,   202,   297,   203,   204,
     205,   206,   207,   208,   298,   209,   299,   210,   300,   277,
     316,   251,   225,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   164,   142,
     143,   144,   145,   286,   146,   147,   148,   149,   226,   227,
     215,   216,   100
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
       6,   -12,   161,   266,   110,     1,   296,    14,   223,    25,
    -175,   159,   159,   154,   166,    49,   167,  -177,    50,  -175,
       5,   322,   323,     7,   224,    31,  -177,   160,   160,   328,
     111,     8,    12,   331,    40,    26,    43,   296,   296,    13,
      47,   276,   158,   126,    57,   296,   278,    13,    15,   296,
     285,    16,    17,    84,   219,   162,   163,    86,   211,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,    84,
     159,   159,   159,   159,   159,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   219,   160,   160,   160,   160,
     160,    82,   276,   212,   315,    13,    84,   278,    27,    96,
      84,    84,   264,   245,   246,   247,   211,    91,    84,   270,
      84,   241,   159,   315,   217,    84,   119,     5,   120,   249,
     157,   157,    25,   165,   163,   252,   173,   174,   160,  -131,
      13,    32,   121,   303,   108,  -131,   302,   122,   108,   108,
      13,   212,    28,   180,   181,   123,   108,     5,   108,    29,
      33,   124,   197,   108,    34,   166,   265,   167,    89,    35,
     211,   166,   153,   258,    13,   234,   235,   324,   157,   157,
     157,   157,   157,   157,   157,   157,   157,   157,   242,   157,
     157,   157,   157,   157,   243,   244,   274,   275,    13,   113,
     114,    37,   211,   211,    13,   212,   213,   118,    76,   150,
     197,   282,    41,    77,    78,    79,    80,    81,    44,    90,
      45,   211,   211,   211,    77,    78,    79,    80,    81,     5,
      13,   157,   252,   211,   211,   214,   211,   212,   212,    48,
      15,   211,   211,    16,    17,   211,   309,   310,   311,   119,
       5,   120,    58,    74,   213,    87,   212,   212,   212,   236,
     237,   238,   239,   252,   197,   119,     5,   120,   212,   212,
     122,   212,   182,   183,   184,    93,   212,   212,   123,    94,
     212,    95,    97,   214,   124,    77,    78,    79,    80,    81,
     190,   119,     5,   120,   191,   291,   197,   197,    59,    98,
     124,    60,    99,    61,    62,    63,    59,   -34,   213,    60,
     101,    61,    62,    63,   102,   197,   197,   314,    49,    49,
     191,    50,    50,   109,   -42,   -43,   124,   197,   197,   103,
     197,   104,   112,   115,   116,   197,   314,   214,   153,   197,
     213,   213,   192,   117,   193,   151,   194,   195,    77,    78,
      79,    80,    81,   175,   176,   177,   178,   179,   152,   213,
     213,   213,   190,   119,     5,   120,   153,   156,   168,   214,
     214,   213,   213,   169,   213,   170,   119,     5,   120,   213,
     213,   171,   185,   213,   172,   186,  -133,  -132,   214,   214,
     214,   187,   191,   188,   189,   220,   221,   222,   124,   248,
     214,   214,   228,   214,   253,   191,   254,   256,   214,   214,
     153,   124,   214,   261,   192,   255,   292,   259,   293,   294,
      77,    78,    79,    80,    81,   119,     5,   120,   262,   263,
     295,   301,   267,    77,    78,    79,    80,    81,   268,   269,
     271,   121,    59,   273,   272,    60,   122,    61,    62,    63,
     313,   -35,   110,   279,   123,   280,   283,   281,   284,   289,
     124,   295,   301,   287,   325,   288,   290,  -181,   304,   313,
     305,   306,  -103,   325,   307,   308,   320,   318,   317,   327,
     319,   321,    77,    78,    79,    80,    81,   330,   326,    24,
      30,    88,   155,    85,    92,    75,   250,   312,   260,   329,
     231,   229,   257,   230,     0,     0,   232,     0,   233
};

static const yytype_int16 yycheck[] =
{
       1,     0,   122,   224,     8,    46,   287,     8,    36,     6,
      27,   121,   122,   115,    34,    47,    36,    27,    50,    36,
       6,   318,   319,     0,    52,    22,    36,   121,   122,   326,
      34,    45,     4,   330,    35,     4,    37,   318,   319,    67,
      41,   255,   121,   110,    45,   326,   255,    67,    47,   330,
     271,    50,    51,    54,   156,   123,   123,    58,   153,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,    70,
     180,   181,   182,   183,   184,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   187,   180,   181,   182,   183,
     184,    54,   306,   153,   308,    67,    97,   306,    67,    36,
     101,   102,   222,   182,   183,   184,   201,    70,   109,    36,
     111,   179,   222,   327,     4,   116,     5,     6,     7,   186,
     121,   122,     6,   124,   191,   192,    18,    19,   222,     8,
      67,     6,    21,    36,    97,     8,   289,    26,   101,   102,
      67,   201,    41,    20,    21,    34,   109,     6,   111,    48,
       6,    40,   153,   116,    68,    34,   223,    36,     6,    42,
     255,    34,    52,    36,    67,   173,   174,   320,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   180,   181,   253,   254,    67,   101,
     102,    42,   287,   288,    67,   255,   153,   109,    57,   111,
     201,   268,    43,    62,    63,    64,    65,    66,    52,    57,
       3,   306,   307,   308,    62,    63,    64,    65,    66,     6,
      67,   222,   289,   318,   319,   153,   321,   287,   288,    52,
      47,   326,   327,    50,    51,   330,   303,   304,   305,     5,
       6,     7,     3,    53,   201,    53,   306,   307,   308,   175,
     176,   177,   178,   320,   255,     5,     6,     7,   318,   319,
      26,   321,    22,    23,    24,     6,   326,   327,    34,     6,
     330,    36,    34,   201,    40,    62,    63,    64,    65,    66,
       4,     5,     6,     7,    34,   286,   287,   288,    44,     6,
      40,    47,     6,    49,    50,    51,    44,    53,   255,    47,
      34,    49,    50,    51,    34,   306,   307,   308,    47,    47,
      34,    50,    50,    34,    53,    53,    40,   318,   319,    37,
     321,    37,     4,    35,     3,   326,   327,   255,    52,   330,
     287,   288,    56,     6,    58,    35,    60,    61,    62,    63,
      64,    65,    66,    13,    14,    15,    16,    17,    35,   306,
     307,   308,     4,     5,     6,     7,    52,    35,     8,   287,
     288,   318,   319,    10,   321,     9,     5,     6,     7,   326,
     327,    12,    27,   330,    11,    36,     8,     8,   306,   307,
     308,    35,    34,     4,     4,    36,    35,    35,    40,     6,
     318,   319,    37,   321,    34,    34,    34,     6,   326,   327,
      52,    40,   330,     4,    56,    34,    58,    53,    60,    61,
      62,    63,    64,    65,    66,     5,     6,     7,     4,    37,
     287,   288,    35,    62,    63,    64,    65,    66,     3,    36,
      34,    21,    44,     4,    37,    47,    26,    49,    50,    51,
     307,    53,     8,    37,    34,    37,    37,    53,    37,     4,
      40,   318,   319,    35,   321,    35,    35,    27,    34,   326,
      34,    34,    59,   330,    59,     4,     4,    35,    37,     4,
      35,    35,    62,    63,    64,    65,    66,    35,    59,    10,
      21,    65,   116,    55,    71,    52,   187,   306,   201,   327,
     170,   168,   196,   169,    -1,    -1,   171,    -1,   172
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    46,    70,    71,    72,     6,   110,     0,    45,    73,
      74,    75,     4,    67,   110,    47,    50,    51,    76,    77,
      78,    79,    80,    84,    75,     6,     4,    67,    41,    48,
      80,    22,     6,     6,    68,    42,    81,    42,    85,    86,
     110,    43,    82,   110,    52,     3,    83,   110,    52,    47,
      50,    91,    92,    93,    94,    95,    96,   110,     3,    44,
      47,    49,    50,    51,    87,    88,    89,    90,    97,   101,
     102,   103,   104,   106,    53,    93,    57,    62,    63,    64,
      65,    66,   107,   109,   110,    96,   110,    53,    89,     6,
      57,   107,   104,     6,     6,    36,    36,    34,     6,     6,
     161,    34,    34,    37,    37,    98,    99,   100,   107,    34,
       8,    34,     4,    98,    98,    35,     3,     6,    98,     5,
       7,    21,    26,    34,    40,   110,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   148,   149,   150,   151,   153,   154,   155,   156,
      98,    35,    35,    52,   111,   100,    35,   110,   144,   151,
     155,   143,   109,   131,   147,   110,    34,    36,     8,    10,
       9,    12,    11,    18,    19,    13,    14,    15,    16,    17,
      20,    21,    22,    23,    24,    27,    36,    35,     4,     4,
       4,    34,    56,    58,    60,    61,   107,   110,   111,   112,
     113,   114,   115,   117,   118,   119,   120,   121,   122,   124,
     126,   133,   148,   154,   156,   159,   160,     4,   105,   111,
      36,    35,    35,    36,    52,   131,   157,   158,    37,   132,
     136,   137,   138,   139,   140,   140,   141,   141,   141,   141,
     108,   109,   110,   142,   142,   144,   144,   144,     6,   131,
     105,   130,   131,    34,    34,    34,     6,   161,    36,    53,
     113,     4,     4,    37,   143,   131,   157,    35,     3,    36,
      36,    34,    37,     4,   131,   131,   119,   128,   159,    37,
      37,    53,   131,    37,    37,   157,   152,    35,    35,     4,
      35,   110,    58,    60,    61,   114,   115,   116,   123,   125,
     127,   114,   130,    36,    34,    34,    34,    59,     4,   131,
     131,   131,   128,   114,   110,   119,   129,    37,    35,    35,
       4,    35,   116,   116,   130,   114,    59,     4,   116,   129,
      35,   116
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    69,    70,    71,    71,    72,    73,    73,    74,    74,
      75,    75,    76,    76,    76,    77,    78,    78,    79,    79,
      80,    80,    80,    81,    81,    82,    82,    83,    83,    84,
      85,    85,    86,    86,    87,    87,    88,    88,    89,    89,
      90,    90,    91,    91,    92,    92,    93,    93,    94,    94,
      95,    95,    96,    96,    97,    98,    98,    99,    99,   100,
     101,   101,   102,   102,   103,   103,   104,   104,   104,   104,
     104,   105,   105,   106,   107,   107,   107,   107,   108,   108,
     108,   109,   109,   109,   109,   109,   110,   110,   111,   112,
     112,   113,   113,   114,   114,   114,   114,   114,   114,   115,
     115,   115,   115,   116,   116,   116,   116,   117,   118,   119,
     119,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   128,   128,   129,   129,   130,   130,   131,   132,   132,
     133,   134,   134,   134,   135,   135,   136,   136,   137,   137,
     138,   138,   139,   139,   139,   140,   140,   140,   140,   140,
     140,   141,   141,   141,   142,   142,   142,   142,   143,   143,
     144,   144,   144,   145,   145,   146,   147,   147,   148,   148,
     148,   148,   149,   149,   149,   149,   149,   149,   150,   152,
     151,   153,   154,   155,   156,   156,   157,   157,   158,   158,
     159,   160,   161,   161
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     0,     1,     3,     0,     1,     1,     2,
       3,     5,     0,     1,     1,     8,     0,     1,     1,     2,
       1,     1,     1,     0,     2,     0,     2,     1,     3,     7,
       0,     1,     2,     3,     0,     1,     1,     2,     1,     1,
       1,     1,     0,     1,     1,     2,     7,     7,     0,     1,
       1,     2,     1,     1,     4,     0,     1,     1,     3,     2,
       7,     7,     0,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     6,     1,     1,     3,     3,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     3,     3,     0,
       1,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     3,     1,
       1,     1,     1,     5,     7,     7,     5,     5,     9,     9,
       0,     1,     1,     0,     1,     0,     1,     1,     1,     1,
       3,     1,     1,     1,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     3,     1,     3,     3,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     3,     2,     1,
       1,     2,     1,     1,     1,     4,     1,     3,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     5,     0,
       9,     3,     5,     3,     4,     6,     0,     1,     1,     3,
       2,     2,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (res, lexer, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, res, lexer); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, pt::Node** res, myFlexLexer& lexer)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (res);
  YY_USE (lexer);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, pt::Node** res, myFlexLexer& lexer)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, res, lexer);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, pt::Node** res, myFlexLexer& lexer)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], res, lexer);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, res, lexer); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, pt::Node** res, myFlexLexer& lexer)
{
  YY_USE (yyvaluep);
  YY_USE (res);
  YY_USE (lexer);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (pt::Node** res, myFlexLexer& lexer)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */


/* User initialization code.  */
#line 61 "parser.y"
{
    (void) yynerrs;
    *ret = nullptr;
}

#line 1453 "parser.tab.c"

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, lexer);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: package_decls import_decls_opt type_decl  */
#line 74 "parser.y"
                                             {
        (void) yynerrs;
        *ret = lexer.make_node(NodeType::ProgramDeclaration, yyvsp[-2], yyvsp[-1], yyvsp[0]);
    }
#line 1659 "parser.tab.c"
    break;

  case 3: /* package_decls: %empty  */
#line 83 "parser.y"
           { yyval = nullptr; }
#line 1665 "parser.tab.c"
    break;

  case 5: /* package_decl: PACKAGE qualified_name SEMI  */
#line 87 "parser.y"
                                             { yyval = lexer.make_node(NodeType::PackageDeclaration, yyvsp[-1]); }
#line 1671 "parser.tab.c"
    break;

  case 6: /* import_decls_opt: %empty  */
#line 93 "parser.y"
           { yyval = nullptr; }
#line 1677 "parser.tab.c"
    break;

  case 10: /* import_decl: IMPORT qualified_name SEMI  */
#line 103 "parser.y"
                               { yyval = lexer.make_node(NodeType::SingleImportDeclaration, yyvsp[-1]); }
#line 1683 "parser.tab.c"
    break;

  case 11: /* import_decl: IMPORT qualified_name '.' STAR ';'  */
#line 104 "parser.y"
                                         { yyval = lexer.make_node(NodeType::MultiImportDeclaration, yyvsp[-3]); }
#line 1689 "parser.tab.c"
    break;

  case 12: /* type_decl: %empty  */
#line 109 "parser.y"
           { yyval = nullptr; }
#line 1695 "parser.tab.c"
    break;

  case 15: /* class_decl: modifiers_opt CLASS ID superclass interfaces LBRACE class_body RBRACE  */
#line 117 "parser.y"
                                                                          {
        yyval = lexer.make_node(NodeType::ClassDeclaration, yyvsp[-7], yyvsp[-5], yyvsp[-4], yyvsp[-3], yyvsp[-1] );
    }
#line 1703 "parser.tab.c"
    break;

  case 16: /* modifiers_opt: %empty  */
#line 124 "parser.y"
           { yyval = nullptr; }
#line 1709 "parser.tab.c"
    break;

  case 18: /* modifiers: modifier  */
#line 129 "parser.y"
             { yyval = lexer.make_node(NodeType::ModifierList, yyvsp[0]); }
#line 1715 "parser.tab.c"
    break;

  case 19: /* modifiers: modifiers modifier  */
#line 130 "parser.y"
                         { yyval = lexer.make_node(NodeType::ModifierList, yyvsp[-1], yyvsp[0]); }
#line 1721 "parser.tab.c"
    break;

  case 23: /* superclass: %empty  */
#line 141 "parser.y"
           { yyval = nullptr; }
#line 1727 "parser.tab.c"
    break;

  case 24: /* superclass: EXTENDS qualified_name  */
#line 142 "parser.y"
                             { yyval = lexer.make_node(NodeType::SuperClass, yyvsp[0]); }
#line 1733 "parser.tab.c"
    break;

  case 25: /* interfaces: %empty  */
#line 147 "parser.y"
           { yyval = nullptr; }
#line 1739 "parser.tab.c"
    break;

  case 26: /* interfaces: IMPLEMENTS interface_list  */
#line 148 "parser.y"
                                { yyval = yyvsp[0]; }
#line 1745 "parser.tab.c"
    break;

  case 27: /* interface_list: qualified_name  */
#line 152 "parser.y"
                   { yyval = lexer.make_node(NodeType::InterfaceTypeList, yyvsp[0]); }
#line 1751 "parser.tab.c"
    break;

  case 28: /* interface_list: interface_list COMMA qualified_name  */
#line 153 "parser.y"
                                          {
        yyval = lexer.make_node(NodeType::InterfaceTypeList, yyvsp[-2], yyvsp[0]);
    }
#line 1759 "parser.tab.c"
    break;

  case 30: /* extends_interfaces_opt: %empty  */
#line 164 "parser.y"
           { yyval = nullptr; }
#line 1765 "parser.tab.c"
    break;

  case 32: /* extends_interfaces: EXTENDS qualified_name  */
#line 169 "parser.y"
                           { yyval = lexer.make_node(NodeType::InterfaceTypeList, yyvsp[0]); }
#line 1771 "parser.tab.c"
    break;

  case 33: /* extends_interfaces: extends_interfaces COMMA qualified_name  */
#line 170 "parser.y"
                                              {
        yyval = lexer.make_node(NodeType::InterfaceTypeList, yyvsp[-2], yyvsp[0]);
    }
#line 1779 "parser.tab.c"
    break;

  case 34: /* class_body: %empty  */
#line 177 "parser.y"
           { yyval = nullptr; }
#line 1785 "parser.tab.c"
    break;

  case 36: /* class_body_decl_list: class_body_decl  */
#line 182 "parser.y"
                    { yyval = lexer.make_node(NodeType::ClassBodyDeclarationList, yyvsp[0]); }
#line 1791 "parser.tab.c"
    break;

  case 37: /* class_body_decl_list: class_body_decl_list class_body_decl  */
#line 183 "parser.y"
                                           {
        yyval = lexer.make_node(NodeType::ClassBodyDeclarationList, yyvsp[-1], yyvsp[0]);
    }
#line 1799 "parser.tab.c"
    break;

  case 42: /* interface_body: %empty  */
#line 199 "parser.y"
           { yyval = nullptr; }
#line 1805 "parser.tab.c"
    break;

  case 44: /* interface_body_decl_list: abstract_method_decl  */
#line 206 "parser.y"
                         { yyval = lexer.make_node(NodeType::InterfaceBodyDeclarationList, yyvsp[0]); }
#line 1811 "parser.tab.c"
    break;

  case 45: /* interface_body_decl_list: interface_body_decl_list abstract_method_decl  */
#line 207 "parser.y"
                                                    {
        yyval = lexer.make_node(NodeType::InterfaceBodyDeclarationList, yyvsp[-1], yyvsp[0]);
    }
#line 1819 "parser.tab.c"
    break;

  case 46: /* abstract_method_decl: abstract_modifiers_opt type ID LPAREN params RPAREN SEMI  */
#line 213 "parser.y"
                                                             {
        yyval = lexer.make_node(NodeType::AbstractMethodDeclaration, yyvsp[-6], yyvsp[-5], yyvsp[-4], yyvsp[-2]);
    }
#line 1827 "parser.tab.c"
    break;

  case 47: /* abstract_method_decl: abstract_modifiers_opt VOID ID LPAREN params RPAREN SEMI  */
#line 216 "parser.y"
                                                               {
        yyval = lexer.make_node(NodeType::AbstractMethodDeclaration, yyvsp[-6], yyvsp[-4], yyvsp[-2]);
        }
#line 1835 "parser.tab.c"
    break;

  case 48: /* abstract_modifiers_opt: %empty  */
#line 222 "parser.y"
           { yyval = nullptr; }
#line 1841 "parser.tab.c"
    break;

  case 50: /* abstract_modifiers: abstract_modifier  */
#line 227 "parser.y"
                      { yyval = lexer.make_node(NodeType::ModifierList, yyvsp[0]); }
#line 1847 "parser.tab.c"
    break;

  case 51: /* abstract_modifiers: abstract_modifiers abstract_modifier  */
#line 228 "parser.y"
                                           {
        yyval = lexer.make_node(NodeType::ModifierList, yyvsp[-1], yyvsp[0]);
    }
#line 1855 "parser.tab.c"
    break;

  case 54: /* field_decl: method_modifiers_opt type var_name SEMI  */
#line 245 "parser.y"
                                            {
        yyval = lexer.make_node(NodeType::FieldDeclaration, yyvsp[-3], yyvsp[-2], yyvsp[-1]);
    }
#line 1863 "parser.tab.c"
    break;

  case 55: /* params: %empty  */
#line 254 "parser.y"
           { yyval = nullptr; }
#line 1869 "parser.tab.c"
    break;

  case 57: /* param_list: param  */
#line 259 "parser.y"
          { yyval = lexer.make_node(NodeType::ParameterList, yyvsp[0]); }
#line 1875 "parser.tab.c"
    break;

  case 58: /* param_list: param_list COMMA param  */
#line 260 "parser.y"
                             {
        yyval = lexer.make_node(NodeType::ParameterList, yyvsp[-2], yyvsp[0]);
    }
#line 1883 "parser.tab.c"
    break;

  case 59: /* param: type ID  */
#line 266 "parser.y"
            { yyval = lexer.make_node(NodeType::Parameter, yyvsp[-1], yyvsp[0]); }
#line 1889 "parser.tab.c"
    break;

  case 60: /* method_decl: method_modifiers_opt VOID ID LPAREN params RPAREN method_body  */
#line 275 "parser.y"
                                                                  {
        yyval = lexer.make_node(NodeType::MethodDeclaration, yyvsp[-6], yyvsp[-4], yyvsp[-2], yyvsp[0]);
    }
#line 1897 "parser.tab.c"
    break;

  case 61: /* method_decl: method_modifiers_opt type ID LPAREN params RPAREN method_body  */
#line 278 "parser.y"
                                                                    {
        yyval = lexer.make_node(NodeType::MethodDeclaration, yyvsp[-6], yyvsp[-5], yyvsp[-4], yyvsp[-2], yyvsp[0]);
    }
#line 1905 "parser.tab.c"
    break;

  case 62: /* method_modifiers_opt: %empty  */
#line 284 "parser.y"
           { yyval = nullptr; }
#line 1911 "parser.tab.c"
    break;

  case 64: /* method_modifiers: method_modifier  */
#line 289 "parser.y"
                    { yyval = lexer.make_node(NodeType::ModifierList, yyvsp[0]); }
#line 1917 "parser.tab.c"
    break;

  case 65: /* method_modifiers: method_modifiers method_modifier  */
#line 290 "parser.y"
                                       { yyval = lexer.make_node(NodeType::ModifierList, yyvsp[-1], yyvsp[0]); }
#line 1923 "parser.tab.c"
    break;

  case 71: /* method_body: block  */
#line 302 "parser.y"
          { yyval = yyvsp[0]; }
#line 1929 "parser.tab.c"
    break;

  case 72: /* method_body: SEMI  */
#line 303 "parser.y"
           { yyval = nullptr; }
#line 1935 "parser.tab.c"
    break;

  case 73: /* constructor_decl: method_modifiers_opt ID LPAREN params RPAREN block  */
#line 311 "parser.y"
                                                                     {
    yyval = lexer.make_node(NodeType::ConstructorDeclaration, yyvsp[-5], yyvsp[-4], yyvsp[-2], yyvsp[0]);
}
#line 1943 "parser.tab.c"
    break;

  case 74: /* type: qualified_name  */
#line 319 "parser.y"
                   { yyval = lexer.make_node(NodeType::Type, yyvsp[0]); }
#line 1949 "parser.tab.c"
    break;

  case 75: /* type: basic_type  */
#line 320 "parser.y"
                 { yyval = lexer.make_node(NodeType::Type, yyvsp[0]); }
#line 1955 "parser.tab.c"
    break;

  case 76: /* type: qualified_name LBRACK RBRACK  */
#line 321 "parser.y"
                                   { yyval = lexer.make_node(NodeType::ArrayType, yyvsp[-2]); }
#line 1961 "parser.tab.c"
    break;

  case 77: /* type: basic_type LBRACK RBRACK  */
#line 322 "parser.y"
                               { yyval = lexer.make_node(NodeType::ArrayType, yyvsp[-2]); }
#line 1967 "parser.tab.c"
    break;

  case 78: /* object_type: qualified_name  */
#line 326 "parser.y"
                   { yyval = lexer.make_node(NodeType::Type, yyvsp[0]); }
#line 1973 "parser.tab.c"
    break;

  case 79: /* object_type: qualified_name LBRACK RBRACK  */
#line 327 "parser.y"
                                   { yyval = lexer.make_node(NodeType::ArrayType, yyvsp[-2]); }
#line 1979 "parser.tab.c"
    break;

  case 80: /* object_type: basic_type LBRACK RBRACK  */
#line 328 "parser.y"
                               { yyval = lexer.make_node(NodeType::ArrayType, yyvsp[-2]); }
#line 1985 "parser.tab.c"
    break;

  case 86: /* qualified_name: ID  */
#line 339 "parser.y"
                                                                      { yyval = lexer.make_node(NodeType::QualifiedName, yyvsp[0]); }
#line 1991 "parser.tab.c"
    break;

  case 87: /* qualified_name: qualified_name '.' ID  */
#line 340 "parser.y"
                                                                   { yyval = lexer.make_node(NodeType::QualifiedName, yyvsp[-2], yyvsp[0]); }
#line 1997 "parser.tab.c"
    break;

  case 88: /* block: LBRACE statements_opt RBRACE  */
#line 347 "parser.y"
                                 { yyval = lexer.make_node(NodeType::Block, yyvsp[-1]); }
#line 2003 "parser.tab.c"
    break;

  case 89: /* statements_opt: %empty  */
#line 353 "parser.y"
           { yyval = nullptr; }
#line 2009 "parser.tab.c"
    break;

  case 90: /* statements_opt: statements  */
#line 354 "parser.y"
                 { yyval = yyvsp[0]; }
#line 2015 "parser.tab.c"
    break;

  case 91: /* statements: statement  */
#line 358 "parser.y"
              { yyval = lexer.make_node(NodeType::StatementList, yyvsp[0]); }
#line 2021 "parser.tab.c"
    break;

  case 92: /* statements: statement statements  */
#line 359 "parser.y"
                           { yyval = lexer.make_node(NodeType::StatementList, yyvsp[-1], yyvsp[0]); }
#line 2027 "parser.tab.c"
    break;

  case 95: /* statement: if_then_statement  */
#line 365 "parser.y"
                        { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2033 "parser.tab.c"
    break;

  case 96: /* statement: if_then_else_statement  */
#line 366 "parser.y"
                             { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2039 "parser.tab.c"
    break;

  case 97: /* statement: while_statement  */
#line 367 "parser.y"
                      { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2045 "parser.tab.c"
    break;

  case 98: /* statement: for_statement  */
#line 368 "parser.y"
                    { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2051 "parser.tab.c"
    break;

  case 104: /* statement_no_short_if: if_then_else_statement_no_short_if  */
#line 380 "parser.y"
                                         { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2057 "parser.tab.c"
    break;

  case 105: /* statement_no_short_if: while_statement_no_short_if  */
#line 381 "parser.y"
                                  { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2063 "parser.tab.c"
    break;

  case 106: /* statement_no_short_if: for_statement_no_short_if  */
#line 382 "parser.y"
                                { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2069 "parser.tab.c"
    break;

  case 107: /* expr_statement: statement_expr SEMI  */
#line 386 "parser.y"
                        { yyval = lexer.make_node(NodeType::Statement, yyvsp[-1]); }
#line 2075 "parser.tab.c"
    break;

  case 108: /* return_statement: RETURN expr_opt SEMI  */
#line 390 "parser.y"
                         { yyval = lexer.make_node(NodeType::ReturnStatement, yyvsp[-1]); }
#line 2081 "parser.tab.c"
    break;

  case 112: /* empty_statement: SEMI  */
#line 400 "parser.y"
         { yyval = lexer.make_node(NodeType::Statement); }
#line 2087 "parser.tab.c"
    break;

  case 113: /* if_then_statement: IF LPAREN expr RPAREN statement  */
#line 407 "parser.y"
                                    { yyval = lexer.make_node(NodeType::IfStatement, yyvsp[-2], yyvsp[0]); }
#line 2093 "parser.tab.c"
    break;

  case 114: /* if_then_else_statement: IF LPAREN expr RPAREN statement_no_short_if ELSE statement  */
#line 411 "parser.y"
                                                               { yyval = lexer.make_node(NodeType::IfStatement, yyvsp[-4], yyvsp[-2], yyvsp[0]); }
#line 2099 "parser.tab.c"
    break;

  case 115: /* if_then_else_statement_no_short_if: IF LPAREN expr RPAREN statement_no_short_if ELSE statement_no_short_if  */
#line 415 "parser.y"
                                                                           { yyval = lexer.make_node(NodeType::IfStatement, yyvsp[-4], yyvsp[-2], yyvsp[0]); }
#line 2105 "parser.tab.c"
    break;

  case 116: /* while_statement: WHILE LPAREN expr RPAREN statement  */
#line 419 "parser.y"
                                       { yyval = lexer.make_node(NodeType::WhileStatement, yyvsp[-2], yyvsp[0]); }
#line 2111 "parser.tab.c"
    break;

  case 117: /* while_statement_no_short_if: WHILE LPAREN expr RPAREN statement_no_short_if  */
#line 423 "parser.y"
                                                   { yyval = lexer.make_node(NodeType::WhileStatement, yyvsp[-2], yyvsp[0]); }
#line 2117 "parser.tab.c"
    break;

  case 118: /* for_statement: FOR LPAREN for_init_opt SEMI expr_opt SEMI for_update_opt RPAREN statement  */
#line 427 "parser.y"
                                                                               {
        yyval = lexer.make_node(NodeType::ForStatement, yyvsp[-6], yyvsp[-4], yyvsp[-2], yyvsp[0]);
    }
#line 2125 "parser.tab.c"
    break;

  case 119: /* for_statement_no_short_if: FOR LPAREN for_init_opt SEMI expr_opt SEMI for_update_opt RPAREN statement_no_short_if  */
#line 433 "parser.y"
                                                                                           {
        yyval = lexer.make_node(NodeType::ForStatement, yyvsp[-6], yyvsp[-4], yyvsp[-2], yyvsp[0]);
    }
#line 2133 "parser.tab.c"
    break;

  case 120: /* for_init_opt: %empty  */
#line 439 "parser.y"
           { yyval = nullptr; }
#line 2139 "parser.tab.c"
    break;

  case 121: /* for_init_opt: local_decl_statement  */
#line 440 "parser.y"
                           { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2145 "parser.tab.c"
    break;

  case 122: /* for_init_opt: statement_expr  */
#line 441 "parser.y"
                     { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2151 "parser.tab.c"
    break;

  case 123: /* for_update_opt: %empty  */
#line 445 "parser.y"
           { yyval = nullptr; }
#line 2157 "parser.tab.c"
    break;

  case 124: /* for_update_opt: statement_expr  */
#line 446 "parser.y"
                     { yyval = lexer.make_node(NodeType::Statement, yyvsp[0]); }
#line 2163 "parser.tab.c"
    break;

  case 125: /* expr_opt: %empty  */
#line 451 "parser.y"
           { yyval = nullptr; }
#line 2169 "parser.tab.c"
    break;

  case 126: /* expr_opt: expr  */
#line 452 "parser.y"
           { yyval = yyvsp[0]; }
#line 2175 "parser.tab.c"
    break;

  case 130: /* assignment: lvalue BECOMES assignment_expr  */
#line 465 "parser.y"
                                           { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2181 "parser.tab.c"
    break;

  case 135: /* conditional_or_expr: conditional_or_expr OR conditional_and_expr  */
#line 477 "parser.y"
                                                  { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2187 "parser.tab.c"
    break;

  case 137: /* conditional_and_expr: conditional_and_expr AND eager_or_expr  */
#line 482 "parser.y"
                                             { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2193 "parser.tab.c"
    break;

  case 139: /* eager_or_expr: eager_or_expr VERT eager_and_expr  */
#line 488 "parser.y"
                                        { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2199 "parser.tab.c"
    break;

  case 141: /* eager_and_expr: eager_and_expr AMP equality_expr  */
#line 493 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2205 "parser.tab.c"
    break;

  case 143: /* equality_expr: equality_expr EQ relational_expr  */
#line 499 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2211 "parser.tab.c"
    break;

  case 144: /* equality_expr: equality_expr NE relational_expr  */
#line 500 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2217 "parser.tab.c"
    break;

  case 146: /* relational_expr: relational_expr LT additive_expr  */
#line 506 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2223 "parser.tab.c"
    break;

  case 147: /* relational_expr: relational_expr GT additive_expr  */
#line 507 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2229 "parser.tab.c"
    break;

  case 148: /* relational_expr: relational_expr LE additive_expr  */
#line 508 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2235 "parser.tab.c"
    break;

  case 149: /* relational_expr: relational_expr GE additive_expr  */
#line 509 "parser.y"
                                       { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2241 "parser.tab.c"
    break;

  case 150: /* relational_expr: relational_expr INSTANCEOF object_type  */
#line 510 "parser.y"
                                             { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2247 "parser.tab.c"
    break;

  case 152: /* additive_expr: additive_expr PLUS multiplicative_expr  */
#line 516 "parser.y"
                                             { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2253 "parser.tab.c"
    break;

  case 153: /* additive_expr: additive_expr MINUS multiplicative_expr  */
#line 517 "parser.y"
                                              { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2259 "parser.tab.c"
    break;

  case 155: /* multiplicative_expr: multiplicative_expr STAR unary_expr  */
#line 523 "parser.y"
                                          { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2265 "parser.tab.c"
    break;

  case 156: /* multiplicative_expr: multiplicative_expr SLASH unary_expr  */
#line 524 "parser.y"
                                           { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2271 "parser.tab.c"
    break;

  case 157: /* multiplicative_expr: multiplicative_expr PCT unary_expr  */
#line 525 "parser.y"
                                         { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[-1], yyvsp[0]); }
#line 2277 "parser.tab.c"
    break;

  case 158: /* unary_expr_negative: MINUS unary_expr  */
#line 530 "parser.y"
                     { yyval = lexer.make_node(NodeType::Expression, yyvsp[-1], yyvsp[0]); }
#line 2283 "parser.tab.c"
    break;

  case 161: /* unary_expr: NOT unary_expr_negative  */
#line 536 "parser.y"
                              { yyval = lexer.make_node(NodeType::Expression, yyvsp[-1], yyvsp[0]); }
#line 2289 "parser.tab.c"
    break;

  case 165: /* cast_expr: LPAREN cast_type RPAREN unary_expr_negative  */
#line 546 "parser.y"
                                                { yyval = lexer.make_node(NodeType::Expression, yyvsp[-2], yyvsp[0]); }
#line 2295 "parser.tab.c"
    break;

  case 166: /* cast_type: basic_type  */
#line 550 "parser.y"
               { yyval = lexer.make_node(NodeType::Type, yyvsp[0]); }
#line 2301 "parser.tab.c"
    break;

  case 167: /* cast_type: basic_type LBRACK RBRACK  */
#line 551 "parser.y"
                               { yyval = lexer.make_node(NodeType::ArrayType, yyvsp[-2]); }
#line 2307 "parser.tab.c"
    break;

  case 178: /* array_create: NEW qualified_name LBRACK expr RBRACK  */
#line 572 "parser.y"
                                          { yyval = lexer.make_node(NodeType::ArrayCreate, yyvsp[-3], yyvsp[-1]); }
#line 2313 "parser.tab.c"
    break;

  case 179: /* @1: %empty  */
#line 576 "parser.y"
                                             { yyval = lexer.make_node(NodeType::ArrayAccess, yyvsp[-3], yyvsp[-1]); }
#line 2319 "parser.tab.c"
    break;

  case 180: /* array_access_expr: primary_without_array LBRACK expr RBRACK @1 qualified_name LBRACK expr RBRACK  */
#line 577 "parser.y"
                                      { yyval = lexer.make_node(NodeType::ArrayAccess, yyvsp[-8], yyvsp[-6]); }
#line 2325 "parser.tab.c"
    break;

  case 181: /* array_cast: qualified_name LBRACK RBRACK  */
#line 581 "parser.y"
                                 { yyval = lexer.make_node(NodeType::ArrayCastType, yyvsp[-2]); }
#line 2331 "parser.tab.c"
    break;

  case 182: /* class_create_expr: NEW qualified_name LBRACE arg_list_opt RBRACE  */
#line 585 "parser.y"
                                                  { yyval = lexer.make_node(NodeType::ClassCreation, yyvsp[-3], yyvsp[-1]); }
#line 2337 "parser.tab.c"
    break;

  case 183: /* field_access_expr: primary DOT ID  */
#line 589 "parser.y"
                   { yyval = lexer.make_node(NodeType::FieldAccess, yyvsp[-2], yyvsp[0]); }
#line 2343 "parser.tab.c"
    break;

  case 184: /* method_invocation_expr: qualified_name LPAREN arg_list_opt RPAREN  */
#line 593 "parser.y"
                                              { yyval = lexer.make_node(NodeType::MethodInvocation, yyvsp[-3], yyvsp[-1]); }
#line 2349 "parser.tab.c"
    break;

  case 185: /* method_invocation_expr: primary DOT ID LPAREN arg_list_opt RPAREN  */
#line 594 "parser.y"
                                                { yyval = lexer.make_node(NodeType::MethodInvocation, yyvsp[-5], yyvsp[-3], yyvsp[-1]); }
#line 2355 "parser.tab.c"
    break;

  case 186: /* arg_list_opt: %empty  */
#line 599 "parser.y"
           { yyval = nullptr; }
#line 2361 "parser.tab.c"
    break;

  case 188: /* arg_list: expr  */
#line 604 "parser.y"
         { yyval = lexer.make_node(NodeType::ArgumentList, yyvsp[0]); }
#line 2367 "parser.tab.c"
    break;

  case 189: /* arg_list: arg_list COMMA expr  */
#line 605 "parser.y"
                          { yyval = lexer.make_node(NodeType::ArgumentList, yyvsp[-2], yyvsp[0]); }
#line 2373 "parser.tab.c"
    break;

  case 190: /* local_decl_statement: local_decl SEMI  */
#line 611 "parser.y"
                    { yyval = lexer.make_node(NodeType::Statement, yyvsp[-1]); }
#line 2379 "parser.tab.c"
    break;

  case 191: /* local_decl: type var_name  */
#line 615 "parser.y"
                  { yyval = lexer.make_node(NodeType::LocalDeclaration, yyvsp[-1], yyvsp[0]); }
#line 2385 "parser.tab.c"
    break;

  case 192: /* var_name: ID  */
#line 619 "parser.y"
       { yyval = lexer.make_node(NodeType::Variable, yyvsp[0]); }
#line 2391 "parser.tab.c"
    break;

  case 193: /* var_name: ID BECOMES expr  */
#line 620 "parser.y"
                      { yyval = lexer.make_node(NodeType::Variable, yyvsp[-2], yyvsp[0]); }
#line 2397 "parser.tab.c"
    break;


#line 2401 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (res, lexer, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, res, lexer);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, res, lexer);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (res, lexer, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, res, lexer);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, res, lexer);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 625 "parser.y"


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

static void yyerror(YYSTYPE* ret, myFlexLexer& lexer, const char* s) {
    (void) ret;
    std::cerr << "Parsing error! : " << s << std::endl;
}
