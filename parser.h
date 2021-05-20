#pragma once

#include <cstdint>
#include <vector>
#include <map>

#include "lexer.h"

struct Symbol_Table;

enum Parse_Node_Type {
  PARSE_NODE_LIST,
  PARSE_NODE_SYMBOL,
  PARSE_NODE_LITERAL,
  PARSE_NODE_FUNCTION,
  PARSE_NODE_SYNTAX,
  PARSE_NODE_ERROR
};

std::string print_parse_node_type(Parse_Node_Type);

enum Parse_Node_Subtype {
  SUBTYPE_NONE,
  
  SYMBOL_KEYWORD,
  SYMBOL_FUNCTION,
  SYMBOL_SYMBOL,
  SYMBOL_VARIABLE,

  LITERAL_INTEGER,
  LITERAL_FLOAT,
  LITERAL_STRING,
  LITERAL_BOOLEAN,

  FUNCTION_MACRO,
  FUNCTION_BUILTIN,
  FUNCTION_NATIVE,

  SYNTAX_QUOTE,
  SYNTAX_BACKTICK,
  SYNTAX_COMMA,
  SYNTAX_COMMA_AT,
};

struct Parse_Node {
  Parse_Node_Type type;
  Parse_Node_Subtype subtype = SUBTYPE_NONE;
  Token token;
  
  union {
    bool b;
    int64_t u64;
    double dub;
    Parse_Node *(*func)(Parse_Node *, Symbol_Table *);
  } val;

  Parse_Node *first = nullptr;
  Parse_Node *next;
  
  int nesting_depth = 0;
  
  std::string print();
  void debug_print_parse_node();
  std::string print_type();
  std::string print_subtype();
  int length();
};

struct Parser {
  std::vector<Parse_Node*> top_level_expressions = {};
  Parse_Node current;
  Lexer lex;

  int current_depth = 0;

  Parser(const char* file) {
    lex = Lexer(file);
  }

  Parser() {
    lex = Lexer();
    lex.filename = "repl";
  }

  Parse_Node *parse_text(std::string input);
  void parse_top_level_expressions();
  Parse_Node *parse_next_token();
  Parse_Node *parse_list(Token start);
};

struct Symbol_Table {

  std::map<std::string, Parse_Node *> table;
  Symbol_Table *parent_table = nullptr;

  Symbol_Table() {}
  Symbol_Table(Symbol_Table *parent) {
    parent_table = parent;
  }
  
  void insert(std::string symbol, Parse_Node *node);
  Parse_Node *lookup(std::string symbol);
  Parse_Node *set(std::string symbol, Parse_Node *node);
};
