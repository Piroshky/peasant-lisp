#include <iostream>
#include "parser.h"

const char *parse_node_types[] = {
  "PARSE_NODE_LIST",
  "PARSE_NODE_SYMBOL",
  "PARSE_NODE_LITERAL",
  "PARES_NODE_FUNCTION"
};



void Parser::parse_top_level_expressions() {
  Token t = lex.peek_next_token();
  while (t.type != TOKEN_END_OF_FILE) {
    Parse_Node new_node = parse_next_token();
    top_level_expressions.push_back(new_node);
    t = lex.peek_next_token();
  }
}

Parse_Node Parser::parse_next_token() {
  Token t = lex.next_token();
  
  switch (t.type) {
  case TOKEN_L_PAREN: {
    Parse_Node list = parse_list(t);
    return list;
    break;
  }

  case TOKEN_R_PAREN:
    std::cerr << "Unexpected ')' at " << lex.filename << ":";
    t.print_position();
    std::cerr << "\n";
    exit(1);
    break;

  case TOKEN_IDENTIFIER: {
    Parse_Node sym = {PARSE_NODE_SYMBOL};
    sym.token = t;
    sym.nesting_depth = current_depth;
    return sym;
    break;
  }

  case TOKEN_INTEGER: {
    uint64_t value = std::stoi(t.name);
    Parse_Node integer = {PARSE_NODE_LITERAL, LITERAL_INTEGER, t};
    integer.val.u64=value;
    integer.nesting_depth = current_depth;
    return integer;
    break;
  }
    
  case TOKEN_FLOAT: {
    double ddouble = std::stod(t.name);
    Parse_Node ffloat = {PARSE_NODE_LITERAL, LITERAL_FLOAT};
    ffloat.val.dub = ddouble;
    ffloat.token = t;
    ffloat.nesting_depth = current_depth;
    return ffloat;
    break;
  }

  default:
    fprintf(stderr, "Error in parse_next_token, this should never occur\n");
    exit(1);    
  }
}

Parse_Node Parser::parse_list(Token start) {
  Parse_Node list = {PARSE_NODE_LIST};
  list.nesting_depth = current_depth;
  list.token = start;
  Token t = lex.peek_next_token();
  current_depth++;
  while (t.type != TOKEN_R_PAREN) {
    if (t.type == TOKEN_END_OF_FILE) {
      std::cerr << "Unmatched '(' at " << lex.filename << ":";
      std::cerr << "\n";
      exit(1);
    }
    
    list.list_items.push_back(parse_next_token());
    t = lex.peek_next_token();
  }
  current_depth--;
  lex.next_token(); // eat right parenthesis  
  return list;
}

void Parse_Node::print_parse_node() {

  switch (type) {
  case PARSE_NODE_LIST: {
    printf("(");
    for(int i = 0; i < list_items.size(); ++i) {
      list_items[i].print_parse_node();
      if (i < list_items.size()-1) {
	printf(" ");
      }
    }
    printf(")");
    break;
  }
    
  case PARSE_NODE_SYMBOL: {
    std::cout << token.name;
    // printf("%s", token.name);
    break;
  }

  case PARSE_NODE_LITERAL: {
    switch (subtype) {
    case LITERAL_INTEGER: {
      printf("%d", val.u64);
      break;
    }
    case LITERAL_FLOAT: {
      printf("%f", val.dub);
      break;
    }
    case LITERAL_STRING: {
      printf("\"stringvalue goes here\"");
      break;
    }
    }
  }

  } 
}


void Parse_Node::debug_print_parse_node() {

  for(int i = 0; i < nesting_depth; i++) printf("  ");
  
  printf("Parse_Node type: %s", parse_node_types[type]);
  if (type != PARSE_NODE_LIST) {
    std::cout << ", name: " << token.name << "\n";;
  } else {
    printf("\n");
    for(auto node : list_items) node.debug_print_parse_node();
  } 
}

void print_parse_node_type(Parse_Node_Type t) {
  printf("%s", parse_node_types[t]);
}
