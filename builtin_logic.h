#pragma once
#include "parser.h"
#include "interp.h"

extern Parse_Node *tru;
extern Parse_Node *fal;

bool is_number(Parse_Node *node);
bool is_bool(Parse_Node *node);

Parse_Node *builtin_and(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_or(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_not(Parse_Node *args, Symbol_Table *env);
