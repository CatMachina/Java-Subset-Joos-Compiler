CC = clang++-16
CXXFLAGS = -std=c++2b -g -Og -DYYDEBUG=1
FLEX = flex
BISON = bison
BISONFLAGS = --locations -k
INCLUDES = -Ilib -I$(PARSER_DIR)
PARSER_DIR = ./compiler_build
LEXER_OUT = $(PARSER_DIR)/lexer.cpp
PARSER_OUT = $(PARSER_DIR)/parser.cpp
LEXER_HEADER = $(PARSER_DIR)/lexer.yy.h
PARSER_HEADER = $(PARSER_DIR)/parser.tab.h

JOOSC_SRCS = \
  src/parser/myBisonParser.cpp src/parseTree/parseTree.cpp \
  driver/joosc/main.cpp \
  ${LEXER_OUT} ${PARSER_OUT}

all: joosc

joosc:
	mkdir -p $(PARSER_DIR)
	$(FLEX) --outfile=$(LEXER_OUT) --header-file=$(LEXER_HEADER) src/lexer/lexer.l
	$(BISON) $(BISONFLAGS) --output=$(PARSER_OUT) --defines=$(PARSER_HEADER) src/parser/parser.y
	$(CC) $(CXXFLAGS) $(INCLUDES) $(JOOSC_SRCS) -o $@

clean:
	rm -rf $(PARSER_DIR) && rm joosc

.PHONY: all clean joosc
