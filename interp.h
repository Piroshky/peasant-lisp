#pragma once
#include "parser.h"

bool is_error(Parse_Node *node);

Parse_Node *eval_parse_node(Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_function(Parse_Node *node, Symbol_Table *env);
  
Symbol_Table create_base_environment();
