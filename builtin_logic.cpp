#include "builtin_logic.h"

Parse_Node *tru;
Parse_Node *fal;

bool is_number(Parse_Node *node) {
  return (node->subtype == LITERAL_INTEGER || node->subtype == LITERAL_FLOAT);
}

bool is_bool(Parse_Node *node) {
  return (node->subtype == LITERAL_BOOLEAN);
}

Parse_Node *builtin_and(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->first->cprint());
      return nullptr;
    }
    if (earg->val.b == false) {
      return fal;
    }    
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_or(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->first->cprint());
      return nullptr;
    }
    if (earg->val.b == true) {
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
  if (!is_bool(earg)) {
    fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	    args->first->cprint());
    return nullptr;
  }
  if (earg->val.b) {
    return fal;
  }
  return tru;
}
