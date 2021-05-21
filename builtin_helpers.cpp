#include "builtin_helpers.h"

bool is_integer(Parse_Node *node) {
  return (node->subtype == LITERAL_INTEGER);
}

bool is_float(Parse_Node *node) {
  return (node->subtype == LITERAL_FLOAT);
}

bool is_string(Parse_Node *node) {
  if (node->type != PARSE_NODE_LITERAL) {
    return false;
  }
  return (node->subtype == LITERAL_STRING);
}

bool is_list(Parse_Node *node) {
  return (node->type == PARSE_NODE_LIST);
}

bool is_sym(Parse_Node *node) {
  return (node->type == PARSE_NODE_SYMBOL);
}

bool is_keyword(Parse_Node *node) {
  return (node->subtype == SYMBOL_KEYWORD);
}

//assumes that node is a list
bool is_empty_list(Parse_Node *node) {
  return (node->first == nullptr);
}

bool is_sequence(Parse_Node *node) {
  return is_list(node) || is_string(node);
}

bool is_error(Parse_Node *node) {
  return (node->type == PARSE_NODE_ERROR);
}
