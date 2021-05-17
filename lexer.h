#pragma once

#include <string>
#include <vector>

std::string get_whole_file(const char* filename);

enum Token_Type {
  TOKEN_L_PAREN,
  TOKEN_R_PAREN,
  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,
  TOKEN_FLOAT,
  TOKEN_QUOTE,
  TOKEN_BACKTICK,
  TOKEN_COMMA,
  TOKEN_STRING,
  TOKEN_END_OF_FILE
};

void print_token_type(Token_Type t);

struct Token {
  Token_Type type;
  int start_line = 0;
  int start_char = 0;

  int stop_line = -1;
  int stop_char = -1;

  std::string name;  

  void print_position();
  void print_token();
};

struct Lexer {
  uint64_t pos = 0;
  int line = 1;
  int character = 1;
  std::string source;   // file content
  std::string filename;

  uint64_t current_token = -1;
  std::vector<Token> tokens;
  
  Lexer() {}
  
  Lexer(const char* file) {
    source = get_whole_file(file);
    filename = file;
  }

  void eat_white_space();
  Token next_token();
  Token peek_next_token();
  Token read_identifier();
  Token read_number();
  Token read_string();
};
