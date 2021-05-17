#include "parser.h"

void Symbol_Table::insert(std::string symbol, Parse_Node *node) {
  table.insert(std::pair<std::string, Parse_Node *>(symbol, node));  
}

Parse_Node *Symbol_Table::lookup(std::string symbol) {
  std::map<std::string, Parse_Node *>::iterator it;

  it = table.find(symbol);
  if (it != table.end()) {
    return it->second;
  } else if (parent_table != nullptr) {
    return parent_table->lookup(symbol);
  } else {
    fprintf(stderr, "Error: unbound symbol: `%s`\n", symbol.c_str());
    return nullptr;    
  }
}

Parse_Node *Symbol_Table::set(std::string symbol, Parse_Node *node) {
  std::map<std::string, Parse_Node *>::iterator it;

  it = table.find(symbol);
  if (it != table.end()) {
    table[symbol] = node;
    return node;
  } else if (parent_table != nullptr) {
    return parent_table->set(symbol, node);
  } else {
    fprintf(stderr, "Error: unbound symbol: `%s`\n", symbol.c_str());
    return nullptr;    
  }
}
