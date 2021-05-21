#pragma once
#include "parser.h"
#include "interp_exceptions.h"

bool is_integer(Parse_Node *node);

bool is_float(Parse_Node *node);

bool is_string(Parse_Node *node);

bool is_list(Parse_Node *node);

bool is_sym(Parse_Node *node);

bool is_keyword(Parse_Node *node);

//assumes that node is a list
bool is_empty_list(Parse_Node *node);

bool is_sequence(Parse_Node *node);

bool is_error(Parse_Node *node);
