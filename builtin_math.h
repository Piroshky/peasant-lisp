#pragma once
#include "parser.h"
#include "interp.h"

Parse_Node *builtin_add(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_multiply(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_greater_than_equal(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_greater_than(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_less_than(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_less_than_equal(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_equal(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitand(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitor(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitxor(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitnot(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitshift_left(Parse_Node *args, Symbol_Table *env);
Parse_Node *builtin_bitshift_right(Parse_Node *args, Symbol_Table *env);
