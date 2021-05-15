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
    fprintf(stderr, "function call error: not a function\n");
    return nullptr;
  }

  Parse_Node *func = env->lookup(func_sym->token.name);

  if (func == nullptr) {

    return nullptr;
  }

  if (func->type != PARSE_NODE_FUNCTION) {
    fprintf(stderr, "Error: `%s` is not a function.\n", func_sym->token.name.c_str());
    return nullptr;
  }

  return func->val.func(node, env);    
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

Parse_Node *builtin_progn(Parse_Node *args, Symbol_Table *env) {
  if (args->list_items.size() == 1) {
    return new Parse_Node{PARSE_NODE_LIST};
  }

  Parse_Node *ret;
  for (int i = 1; i < args->list_items.size(); ++i) {
    ret = eval_parse_node(&args->list_items[i], env);
  }
  return ret;
}

Parse_Node *builtin_let(Parse_Node *args, Symbol_Table *env) {
  if (args->list_items.size() < 2) {
    fprintf(stderr, "Error: too few arguments in `let`\n");
    return nullptr;
  }

  Symbol_Table *let_env = new Symbol_Table(env);
  Parse_Node *defs = &args->list_items[1];

  for (int i = 0; i < defs->list_items.size(); ++i) {
    Parse_Node *let_form = &defs->list_items[i];

    switch (let_form->type) {
    case PARSE_NODE_LIST: {

      int lfs = let_form->list_items.size();
      if (lfs != 2 && lfs != 1) {
	fprintf(stderr, "Error: invalid let binding: %s\n"
		"     : let form length %d\n"
		, let_form->print_parse_node().c_str(), lfs);
	return nullptr;
      }
     
      Parse_Node *sym = &let_form->list_items[0];

      if (sym->type != PARSE_NODE_SYMBOL) {
	fprintf(stderr, "Error: invalid let binding: `%s` is not a symbol\n", sym->print_parse_node());
	return nullptr;
      }
      
      if (lfs == 1) {

      } else {
	let_env->table[sym->token.name] = eval_parse_node(&let_form->list_items[1], let_env);
      }      
      break;
    }
    case PARSE_NODE_SYMBOL: {
      let_env->table[let_form->token.name] = new Parse_Node{PARSE_NODE_LIST};      
      break;
    }
    default: {
      fprintf(stderr, "Error: invalid let binding: %s\n", let_form->print_parse_node().c_str());
      return nullptr;
      break;
    }
    }
  }

  if (args->list_items.size() == 2) {
    return new Parse_Node{PARSE_NODE_LIST};
  }

  Parse_Node *ret;
  for (int i = 2; i < args->list_items.size(); ++i) {
    ret = eval_parse_node(&args->list_items[i], let_env);
  }
  return ret;  
}

Parse_Node *builtin_if(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->list_items.size();
  if (nargs != 3 && nargs != 4) {
    fprintf(stderr, "Error: incorrect number of arguments\n");
    return nullptr;    
  }

  Parse_Node *condition = eval_parse_node(&args->list_items[1], env);

  if (condition->subtype != LITERAL_BOOLEAN) {
    fprintf(stderr, "Error: if statement condition did not evaluate to boolean\n");
    return nullptr;
  }

  Parse_Node *ret;
  if (condition->val.b == true) {
    ret = eval_parse_node(&args->list_items[2], env);
  } else if (nargs == 3) {
    ret = new Parse_Node{PARSE_NODE_LIST};
  } else {
    ret = eval_parse_node(&args->list_items[3], env);
  }
  return ret;
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

Parse_Node *tru;
Parse_Node *fal;

bool is_number(Parse_Node *node) {
  return (node->subtype == LITERAL_INTEGER || node->subtype == LITERAL_FLOAT);
}

bool is_bool(Parse_Node *node) {
  return (node->subtype == LITERAL_BOOLEAN);
}

Parse_Node *builtin_less_than(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->list_items.size();
  if (nargs == 1) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  
  Parse_Node *last = &args->list_items[1];
  bool last_int = (last->subtype == LITERAL_INTEGER);
  
  if (is_number(last)) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    last->print_parse_node().c_str());
    return nullptr;
  }  

  if (nargs == 2) {return tru;}

  for (int i = 1; i < nargs-1; ++i) {
    Parse_Node *cur = &args->list_items[i+1];
    
    bool cur_int = (cur->subtype == LITERAL_INTEGER);
    if (!cur_int && cur->subtype != LITERAL_FLOAT) {
      fprintf(stderr, "Error: argument %d, %s, is not a number",
	      i+1, last->print_parse_node().c_str());
      return nullptr;
    }

    if (last_int) {
      if (cur_int) {
	if (! (last->val.u64 < cur->val.u64)) {return fal;}
      } else {
	if (! (last->val.u64 < cur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (last->val.dub < cur->val.u64)) {return fal;}
      } else {
	if (! (last->val.dub < cur->val.dub)) {return fal;}
      }
    }
    
    last = cur;
    last_int = cur_int;
  }
  return tru;
}

Parse_Node *builtin_greater_than(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->list_items.size();
  if (nargs == 1) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  
  Parse_Node *last = &args->list_items[1];
  bool last_int = (last->subtype == LITERAL_INTEGER);
  
  if (!last_int && last->subtype != LITERAL_FLOAT) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    last->print_parse_node().c_str());
    return nullptr;
  }  

  if (nargs == 2) {return tru;}

  for (int i = 1; i < nargs-1; ++i) {
    Parse_Node *cur = &args->list_items[i+1];
    
    bool cur_int = (cur->subtype == LITERAL_INTEGER);
    if (!cur_int && cur->subtype != LITERAL_FLOAT) {
      fprintf(stderr, "Error: argument %d, %s, is not a number",
	      i+1, last->print_parse_node().c_str());
      return nullptr;
    }

    if (last_int) {
      if (cur_int) {
	if (! (last->val.u64 > cur->val.u64)) {return fal;}
      } else {
	if (! (last->val.u64 > cur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (last->val.dub > cur->val.u64)) {return fal;}
      } else {
	if (! (last->val.dub > cur->val.dub)) {return fal;}
      }
    }
    
    last = cur;
    last_int = cur_int;
  }
  return tru;
}

Parse_Node *builtin_and(Parse_Node *args, Symbol_Table *env) {
  for (int i = 1; i < args->list_items.size(); ++i) {
    Parse_Node *earg = eval_parse_node(&args->list_items[i], env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->list_items[i].print_parse_node().c_str());
      return nullptr;
    }
    if (earg->val.b == false) {
      return fal;
    }
  }
  return tru;
}

Parse_Node *builtin_or(Parse_Node *args, Symbol_Table *env) {
  for (int i = 1; i < args->list_items.size(); ++i) {
    Parse_Node *earg = eval_parse_node(&args->list_items[i], env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->list_items[i].print_parse_node().c_str());
      return nullptr;
    }
    if (earg->val.b == true) {
      return tru;
    }
  }
  return fal;
}

Parse_Node *builtin_not(Parse_Node *args, Symbol_Table *env) {
  if (args->list_items.size() != 2) {
    fprintf(stderr, "Error: not takes exactly 1 argument\n");
  }
  Parse_Node *earg = eval_parse_node(&args->list_items[1], env);
  if (!is_bool(earg)) {
    fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	    args->list_items[1].print_parse_node().c_str());
    return nullptr;
  }
  if (earg->val.b) {
    return fal;
  }
  return tru;
}

void create_builtin(std::string symbol, Parse_Node *(*func)(Parse_Node *, Symbol_Table *), Symbol_Table *env) {
  Parse_Node *f = new Parse_Node{PARSE_NODE_FUNCTION, FUNCTION_BUILTIN};
  f->val.func = func;
  env->insert(symbol, f);
}

Symbol_Table create_base_environment() {
  Symbol_Table env = Symbol_Table();

  tru = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_BOOLEAN};
  tru->val.b = true;
  fal = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_BOOLEAN};
  fal->val.b = false;
  
  env.insert("true", tru);
  env.insert("false", fal);

  create_builtin("+", builtin_add, &env);
  create_builtin("*", builtin_multiply, &env);
  create_builtin("defsym", builtin_defsym, &env);
  create_builtin("let", builtin_let, &env);
  create_builtin("progn", builtin_progn, &env);
  create_builtin("if", builtin_if, &env);
  create_builtin("<", builtin_less_than, &env);
  create_builtin(">", builtin_greater_than, &env);
  create_builtin("and", builtin_and, &env);
  create_builtin("or", builtin_or, &env);
  create_builtin("not", builtin_not, &env);

  return env;
}
