cmake_minimum_required(VERSION 3.21)

project(OLKCompiler VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)
enable_testing()

# Define YYDEBUG for bison
add_compile_definitions(YYDEBUG=1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

################################################################################
#                         Flex and Bison targets                               #
################################################################################

find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.8 REQUIRED)

set(PARSER_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(LEXER_OUT "${PARSER_DIR}/lexer.cpp")
set(PARSER_OUT "${PARSER_DIR}/parser.cpp")

flex_target(
    LEXER
    "src/lexer/lexer.l"
    "${LEXER_OUT}"
    DEFINES_FILE
    "${PARSER_DIR}/lexer.yy.h"
)

bison_target(
    PARSER
    "src/parser/parser.y"
    "${PARSER_OUT}"
    DEFINES_FILE
    "${PARSER_DIR}/parser.tab.h"
)

add_flex_bison_dependency(LEXER PARSER)

################################################################################
#                          Create Library                                      #
################################################################################

add_library(CompilerLib
    # flex bison
    "${LEXER_OUT}"
    "${PARSER_OUT}"

    # ast
    "src/ast/astNode.cpp"

    # parser
    "src/lexer/joos1w.cpp"

    # parseTree
    "src/parseTree/parseTree.cpp"
    "src/parseTree/parseTreeVisitor.cpp"
    "src/parseTree/expressionVisitor.cpp"

    # static check
    "src/staticCheck/envManager.cpp"
    "src/staticCheck/typeLinker.cpp"
    "src/staticCheck/nameDisambiguator.cpp"
    "src/staticCheck/exprResolver.cpp"
    # "src/staticCheck/typeResolver.cpp"
)
target_include_directories(
    CompilerLib
    PUBLIC
    "include"
    "${PARSER_DIR}"
)

################################################################################
#                          Build Scanner Tool                                  #
################################################################################

# Macro to add an executable with lexer and parser sources
macro(add_tool TARGET_NAME)
    add_executable(${TARGET_NAME}
        ${ARGN}
    )
    target_link_libraries(${TARGET_NAME} CompilerLib)
    target_include_directories(${TARGET_NAME} PRIVATE
        "${PARSER_DIR}"
        lib
        "."
    )
endmacro()

add_tool(
    lexer
    "driver/lexer/main.cpp"
)

add_tool(
    parser
    "driver/parser/main.cpp"
)

add_tool(
    joosc
    "driver/joosc/main.cpp"
)
