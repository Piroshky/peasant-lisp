#pragma once
#include "parser.h"
#include "interp_exceptions.h"

#define ARG_COUNT_ZERO(FUNCSTR) if(args->length() != 0) {throw runtimeError("Error: " FUNCSTR " takes no arguments, received " + std::to_string(args->length()) + "\n");}

#define ARG_COUNT_EXACT(FUNCSTR, NARGS) if(args->length() != NARGS ) {throw runtimeError("Error: " FUNCSTR " takes exactly " + std::to_string(NARGS) + " argument" + ((NARGS == 1) ? ", received " : "s, received ") + std::to_string(args->length()) + "\n" );}

#define ARG_COUNT_MIN(FUNCSTR, NARGS) if(args->length() < NARGS ) {throw runtimeError("Error: " FUNCSTR " takes " + std::to_string(NARGS) + "+ arguments, received " + std::to_string(args->length()) + "\n");}

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
