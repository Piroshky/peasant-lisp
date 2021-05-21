#include <iostream>
#include <string>
#include "parser.h"

const char *parse_node_types[] = {
  "PARSE_NODE_LIST",
  "PARSE_NODE_SYMBOL",
  "PARSE_NODE_LITERAL",
  "PARES_NODE_FUNCTION",
  "PARES_NODE_SYNTAX",
  "PARES_NODE_ERROR"
};

const char *parse_node_subtypes[] = {
  "SUBTYPE_NONE",
  "SYMBOL_KEYWORD",
  "SYMBOL_FUNCTION",
  "SYMBOL_SYMBOL",
  "SYMBOL_VARIABLE",
  "LITERAL_INTEGER",
  "LITERAL_FLOAT",
  "LITERAL_STRING",
  "LITERAL_BOOLEAN",
  "FUNCTION_MACRO",
  "FUNCTION_BUILTIN",
  "FUNCTION_NATIVE",
  "SYNTAX_QUOTE",
  "SYNTAX_BACKTICK",
  "SYNTAX_COMMA",
  "SYNTAX_COMMA_AT",
};

Parse_Node *Parser::parse_text(std::string input) {
  lex.feed(input);
  return parse_next_token();
}

void Parser::parse_top_level_expressions() {
  Token t = lex.peek_next_token();
  while (t.type != TOKEN_END_OF_FILE) {
    Parse_Node *new_node = parse_next_token();
    top_level_expressions.push_back(new_node);
    t = lex.peek_next_token();
  }
}

Parse_Node *Parser::parse_next_token() {
  Token t = lex.next_token();

  switch (t.type) {
  case TOKEN_L_PAREN: {
    Parse_Node *list = parse_list(t);
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
    Parse_Node *sym = new Parse_Node{PARSE_NODE_SYMBOL};
    if (t.name[0] == ':') {
      sym->subtype = SYMBOL_KEYWORD;
    }
    sym->token = t;
    sym->nesting_depth = current_depth;
    return sym;
    break;
  }
    
  case TOKEN_STRING: {
    Parse_Node *str = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_STRING};
    str->token = t;
    str->nesting_depth = current_depth;
    return str;
    break;
  }

  case TOKEN_INTEGER: {
    uint64_t value = std::stoi(t.name);
    Parse_Node *integer = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER, t};
    integer->val.u64=value;
    integer->nesting_depth = current_depth;
    return integer;
    break;
  }
    
  case TOKEN_FLOAT: {
    double ddouble = std::stod(t.name);
    Parse_Node *ffloat = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_FLOAT};
    ffloat->val.dub = ddouble;
    ffloat->token = t;
    ffloat->nesting_depth = current_depth;
    return ffloat;
    break;
  }

  case TOKEN_QUOTE: {
    Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_QUOTE};
    Token t = lex.peek_next_token();
    if (t.type == TOKEN_END_OF_FILE) {
      fprintf(stderr, "Error: expected object after quote, reached end of file instead\n");      
      exit(1);
    }
    Parse_Node *a = parse_next_token();
    q->first = a;
    return q;
    break;
  }

  case TOKEN_BACKTICK: {
    Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_BACKTICK};
    Token t = lex.peek_next_token();
    if (t.type == TOKEN_END_OF_FILE) {
      fprintf(stderr, "Error: expected object after backtick, reached end of file instead\n");      
      exit(1);
    }
    Parse_Node *a = parse_next_token();
    q->first = a;
    return q;
    break;
  }
    
  case TOKEN_COMMA: {
    Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_COMMA};
    Token t = lex.peek_next_token();
    if (t.type == TOKEN_END_OF_FILE) {
      fprintf(stderr, "Error: expected object after comma, reached end of file instead\n");      
      exit(1);
    }
    Parse_Node *a = parse_next_token();
    q->first = a;
    return q;
    break;
  }

  case TOKEN_COMMA_AT: {
    Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_COMMA_AT};
    Token t = lex.peek_next_token();
    if (t.type == TOKEN_END_OF_FILE) {
      fprintf(stderr, "Error: expected object after ,@ but reached end of file instead\n");      
      exit(1);
    }
    Parse_Node *a = parse_next_token();
    q->first = a;
    return q;
    break;
  }
    
  case TOKEN_END_OF_FILE: {
    return nullptr;
  }

  default:
    fprintf(stderr, "Error in parse_next_token, this should never occur\n");
    exit(1);    
  }
}

Parse_Node *Parser::parse_list(Token start) {
  Parse_Node *list = new Parse_Node{PARSE_NODE_LIST};
  list->nesting_depth = current_depth;
  list->token = start;
  Token t = lex.peek_next_token();
  current_depth++;
  Parse_Node *cur = list;
  Parse_Node *next;
  while (t.type != TOKEN_R_PAREN) {
    if (t.type == TOKEN_END_OF_FILE) {
      std::cerr << "Unmatched '(' at " << lex.filename << ":" << t.start_line << ":" << t.start_char << std::endl;
      // std::cerr << "\n";
      exit(1);
    }

    cur->first = parse_next_token();
    next = new Parse_Node{PARSE_NODE_LIST};
    cur->next = next;
    cur = next;
    t = lex.peek_next_token();    
  }
  current_depth--;
  lex.next_token(); // eat right parenthesis
  return list;
}

int Parse_Node::length() {
  if (type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: called length on object that is not a list\n");
    return 0;
  }

  int len = 0;
  Parse_Node *cur = this;
  while (cur->first != nullptr) {
    ++len;
    cur = cur->next;
  }
  return len;  
}

std::string Parse_Node::print() {
  switch (type) {
  case PARSE_NODE_LIST: {
    std::string list = "(";
    Parse_Node *cur = this;
    while (cur->first != nullptr) {
      list += cur->first->print();
      if (cur->next->first != nullptr) {
	list += " ";
      }
      cur = cur->next;
    }
    list += ")";
    return list;
  }

  case PARSE_NODE_SYMBOL: {
    return token.name;
  }
    
  case PARSE_NODE_LITERAL: {
    switch (subtype) {
    case LITERAL_INTEGER: {
      return std::to_string(val.u64);
    }
    case LITERAL_FLOAT: {
      return std::to_string(val.dub);     
    }
      
    case LITERAL_BOOLEAN: {
      return (val.b ? "true" : "false");
    }
    case LITERAL_STRING: {
      return token.name;
    }
    }
  }
  case PARSE_NODE_FUNCTION: {
    return "#'" + token.name;
    break;
  }
  case PARSE_NODE_SYNTAX: {
    switch (subtype) {
    case SYNTAX_QUOTE: {
      return "'" + first->print();      
      break;
    }
    case SYNTAX_BACKTICK: {
      return "`" + first->print();      
      break;
    }
    case SYNTAX_COMMA: {
      return "," + first->print();      
      break;
    }
    case SYNTAX_COMMA_AT: {
      return ",@" + first->print();      
      break;
    }
    }    
  }
    
  case PARSE_NODE_ERROR: {
    return "[ERROR]";   
  }
    
  default:
    fprintf(stderr, "Tried to print unknown symbol type\n");
    exit(1);
  }
}

void Parse_Node::debug_print_parse_node() {

  for(int i = 0; i < nesting_depth; i++) printf("  ");
  
  printf("Parse_Node type: %s", parse_node_types[type]);
  if (type != PARSE_NODE_LIST) {
    std::cout << ", name: " << token.name << "\n";;
  } else {
    printf("\n");
    Parse_Node *cur = this;
    while (cur->first != nullptr) {
      cur->debug_print_parse_node();
      cur = cur->next;
    }    
  } 
}

std::string print_parse_node_type(Parse_Node_Type t) {
  return parse_node_types[t];  
}

std::string Parse_Node::print_type() {
  return print_parse_node_type(type);
}


std::string print_parse_node_subtype(Parse_Node_Subtype t) {
  return parse_node_subtypes[t];  
}

std::string Parse_Node::print_subtype() {
  return print_parse_node_subtype(subtype);
}
