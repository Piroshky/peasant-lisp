#include <cstdio>
#include <cerrno>
#include <iostream>

#include "lexer.h"
#include "parser.h"
#include "interp.h"

int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("usage: %s source-file\n", argv[0]);
    return 1;
  }

  // Token t = lex.next_token();

  // while (t.type != TOKEN_END_OF_FILE) {
  //   printf("token type ");
  //   print_token_type(t.type);
  //   printf(" pos %d:%d ", t.start_line, t.start_char);
  //   if (t.type == TOKEN_IDENTIFIER || TOKEN_INTEGER) {std::cout << t.name;}
  //   std::cout << '\n';
  //   t = lex.next_token();
  // }

  Symbol_Table env = create_base_environment();
  Parser parse = Parser(argv[1]);

  parse.parse_top_level_expressions();

  for (int i = 0; i < parse.top_level_expressions.size(); i++) {
    // parse.top_level_expressions[i].debug_print_parse_node();
    printf("> ");
    parse.top_level_expressions[i].print_parse_node();
    printf("\n");
    eval_parse_node(&parse.top_level_expressions[i], &env)->print_parse_node();
    printf("\n\n");
  }
  
  return 0;  
}
