#pragma once
#include "parser.h"

Parse_Node *eval_parse_node(Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_function(Parse_Node *node, Symbol_Table *env);
  
Symbol_Table create_base_environment();
