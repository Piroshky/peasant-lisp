#include "builtin_logic.h"

Parse_Node *tru;
Parse_Node *fal;

bool is_number(Parse_Node *node) {
  return (node->subtype == LITERAL_INTEGER || node->subtype == LITERAL_FLOAT);
}

bool is_bool(Parse_Node *node) {
  return (node->subtype == LITERAL_BOOLEAN);
}

bool bool_value(Parse_Node *node) {
  if (is_bool(node)) {
    return node->val.b;
  }
  return true;
}

Parse_Node *builtin_and(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    bool val = bool_value(earg);
    if (val == false) {
      return fal;
    }    
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_or(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    bool val = bool_value(earg);
    if (val) {
      return tru;
    }    
    args = args->next;
  }
  return fal;
}

Parse_Node *builtin_not(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: not takes exactly 1 argument\n");
  }
  Parse_Node *earg = eval_parse_node(args->first, env);
  bool val = bool_value(earg);
  if (val) {
    return fal;
  }
  return tru;
}
