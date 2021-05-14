#include "interp.h"

Parse_Node *eval_parse_node(Parse_Node *node, Symbol_Table *env) {
  switch (node->type) {
  case PARSE_NODE_LITERAL: {
    return node;
  }

  case PARSE_NODE_LIST: {
    return apply_function(node, env);
    break;
  }
    
  case PARSE_NODE_SYMBOL: {
    return env->lookup(node->token.name);
    break;
  }
    
  }
  
  return nullptr;
}

Parse_Node *apply_function(Parse_Node *node, Symbol_Table *env) {
  if (node->list_items.size() == 0) {
    return node;
  }

  Parse_Node *func_sym = &node->list_items[0];

  if (func_sym->type != PARSE_NODE_SYMBOL) {
    // error illegal function call    
  }

  Parse_Node *func = env->lookup(func_sym->token.name);

  if (func == nullptr) {
    // function not found
  }

  return func->val.func(node, env);
    
}

Parse_Node *builtin_add(Parse_Node *args, Symbol_Table *env) {
  int64_t sum = 0;
  double fsum = 0;
  bool is_float = false;

  for (int i = 1; i < args->list_items.size(); ++i) {
    Parse_Node *arg = eval_parse_node(&args->list_items[i], env);

    if (arg->type == PARSE_NODE_LITERAL) {
      if (is_float) {	      
	if (arg->subtype == LITERAL_FLOAT) {
	  fsum += arg->val.dub;
	} else if (arg->subtype == LITERAL_INTEGER) {
	  fsum += arg->val.u64;
	}
      } else {
	if (arg->subtype == LITERAL_FLOAT) {
	  is_float = true;
	  fsum = sum + arg->val.dub;
	} else if (arg->subtype == LITERAL_INTEGER) {
	  sum += arg->val.u64;
	}
      }
    } else {
      fprintf(stderr, "+ was given non literal to add\n");
    }
  }

  Parse_Node *s = new Parse_Node{PARSE_NODE_LITERAL};
  if (is_float) {
    s->subtype = LITERAL_FLOAT;
    s->val.dub = fsum;
  } else {
    s->subtype = LITERAL_INTEGER;
    s->val.u64 = sum;
  }

  return s;
}

Parse_Node *builtin_multiply(Parse_Node *args, Symbol_Table *env) {
  int64_t prod = 1;
  double fprod = 1;
  bool is_float = false;

  for (int i = 1; i < args->list_items.size(); ++i) {
    Parse_Node *arg = eval_parse_node(&args->list_items[i], env);

    if (arg->type == PARSE_NODE_LITERAL) {
      if (is_float) {	      
	if (arg->subtype == LITERAL_FLOAT) {
	  fprod *= arg->val.dub;
	} else if (arg->subtype == LITERAL_INTEGER) {
	  fprod *= arg->val.u64;
	}
      } else {
	if (arg->subtype == LITERAL_FLOAT) {
	  is_float = true;
	  fprod = prod * arg->val.dub;
	} else if (arg->subtype == LITERAL_INTEGER) {
	  prod *= arg->val.u64;
	}
      }
    } else {
      fprintf(stderr, "* was given non literal to add\n");
    }
  }

  Parse_Node *s = new Parse_Node{PARSE_NODE_LITERAL};
  if (is_float) {
    s->subtype = LITERAL_FLOAT;
    s->val.dub = fprod;
  } else {
    s->subtype = LITERAL_INTEGER;
    s->val.u64 = prod;
  }

  return s;
}

Parse_Node *builtin_defsym(Parse_Node *args, Symbol_Table *env) {

  if (args->list_items.size() != 3) {
    fprintf(stderr, "defsym takes exactly two arguments.\nInstead received %d\n", args->list_items.size()-1);
    
    //error
  }

  Parse_Node *sym = &args->list_items[1];
  
  if (sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "first argument given to defsym was not a symbol\n");
    //error
  }
  
  std::map<std::string, Parse_Node *>::iterator it;
  // it = env.table.find(sym.token.name);
  // if (it == env.table.end()) {
  //   //
  // }
  env->table[sym->token.name] = &args->list_items[2];
  
  return sym;
}

void create_builtin(std::string symbol, Parse_Node *(*func)(Parse_Node *, Symbol_Table *), Symbol_Table *env) {
  Parse_Node *f = new Parse_Node{PARSE_NODE_FUNCTION, FUNCTION_BUILTIN};
  f->val.func = func;
  env->insert(symbol, f);
}

Symbol_Table create_base_environment() {
  Symbol_Table env = Symbol_Table();

  create_builtin("+", builtin_add, &env);
  create_builtin("*", builtin_multiply, &env);
  create_builtin("defsym", builtin_defsym, &env);

  return env;
}
