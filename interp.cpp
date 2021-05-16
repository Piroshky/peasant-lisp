#include "interp.h"

Parse_Node *eval_list(Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_macro(Parse_Node *func, Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_fun(Parse_Node *fun, Parse_Node *node, Symbol_Table *env);

Parse_Node *eval_parse_node(Parse_Node *node, Symbol_Table *env) {
  switch (node->type) {
  case PARSE_NODE_LITERAL: {
    return node;
  }

  case PARSE_NODE_LIST: {
    return eval_list(node, env);
    break;
  }
    
  case PARSE_NODE_SYMBOL: {
    return env->lookup(node->token.name);
    break;
  }
  case PARSE_NODE_SYNTAX: {
    return node->first; // this is only quote, obviously will need more handling for more syntax
    break;
  }
  default:
    fprintf(stderr, "Error: could not evaluate unknown type\n");
    return nullptr;
  }  
  return nullptr;
}

Parse_Node *eval_list(Parse_Node *node, Symbol_Table *env) {
  // The empty list evaluates to itself
  if (node->first == nullptr) {
    return node;
  }

  Parse_Node *func_sym = node->first;
  if (func_sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: invalid function call, %s is not a symbol\n", func_sym->cprint());
    return nullptr;
  }

  Parse_Node *func = env->lookup(func_sym->token.name);
  if (func == nullptr) {
    fprintf(stderr, "Error: could not find function or macro named %s\n", func_sym->cprint());
    return nullptr;
  }
  
  switch (func->subtype) {
  case FUNCTION_BUILTIN: {
    return func->val.func(node->next, env);
    break;
  }
  case FUNCTION_MACRO: {
    return apply_macro(func, node, env);
    break;
  }
  case FUNCTION_NATIVE: {
    return apply_fun(func, node, env);
    break;
  }
    
  default: {
    fprintf(stderr, "Error: function type not recognized\n");
    fprintf(stderr, "node: %s\n", node->cprint());
    fprintf(stderr, "func_sym: %s\n", func_sym->cprint());
    fprintf(stderr, "func: %s\n", func->cprint());
    return nullptr;
    break;
  }
  }  
}

Parse_Node *apply_fun(Parse_Node *fun, Parse_Node *node, Symbol_Table *env) {
  Symbol_Table *fun_env = new Symbol_Table(env);
  
  // bind given arguments to symbols in macro-args
  Parse_Node *cur_param = fun->first;
  Parse_Node *cur_arg = node->next;
  while (cur_param->first != nullptr) {
    Parse_Node *param = cur_param->first;
    Parse_Node *arg = cur_arg->first;
    switch(param->type) {
    case PARSE_NODE_SYMBOL: {
      fun_env->table[param->token.name] = eval_parse_node(arg, env);
      break;
    }
    default: {
      fprintf(stderr, "Shouldn't reach here\n");
      fprintf(stderr, "cur_param: %s\n", cur_param->cprint());
      fprintf(stderr, "arg type: %s\n", arg->print_type());
      fprintf(stderr, "param type: %s\n", param->print_type());
      fprintf(stderr, "arg: %s\n", arg->cprint());
      fprintf(stderr, "param: %s\n", param->cprint());
      return nullptr;
      break;
    }
    }
    cur_param = cur_param->next;
    cur_arg = cur_arg->next;
  }

  Parse_Node *cur = fun->next;
  Parse_Node *ret;
  while (cur->first != nullptr) {
    ret = eval_parse_node(cur->first, fun_env);
    cur = cur->next;
  }
  
  return ret;
}

Parse_Node *apply_macro(Parse_Node *macro, Parse_Node *node, Symbol_Table *env) {
  Symbol_Table *macro_env = new Symbol_Table(env);
  
  // bind given arguments to symbols in macro-args
  Parse_Node *cur_param = macro->first;
  Parse_Node *cur_arg = node->next;
  while (cur_param->first != nullptr) {
    Parse_Node *param = cur_param->first;
    Parse_Node *arg = cur_arg->first;
    switch(param->type) {
    case PARSE_NODE_SYMBOL: {
      macro_env->table[param->token.name] = arg;
      break;
    }
    default: {
      fprintf(stderr, "Shouldn't reach here\n");
      fprintf(stderr, "cur_param: %s\n", cur_param->cprint());
      fprintf(stderr, "arg type: %s\n", arg->print_type());
      fprintf(stderr, "param type: %s\n", param->print_type());
      fprintf(stderr, "arg: %s\n", arg->cprint());
      fprintf(stderr, "param: %s\n", param->cprint());
      return nullptr;
      break;
    }
    }
    cur_param = cur_param->next;
    cur_arg = cur_arg->next;
  }

  Parse_Node *cur = macro->next;
  Parse_Node *ret;
  while (cur->first != nullptr) {
    ret = eval_parse_node(cur->first, macro_env);
    cur = cur->next;
  }
  
  return ret;
}

Parse_Node *builtin_quote(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_QUOTE};
  q->first = args;
  return q;
}

Parse_Node *builtin_defun(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs < 2) {
    fprintf(stderr, "Error: defun requires a fun-name argument, and an fun-args argument\n");
    return nullptr;
  }

  Parse_Node *fun_name = args->first;
  if (fun_name->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: argument fun-name requires a symbol\n");
    return nullptr;
  }

  Parse_Node *fun_args = args->next->first;
  if (fun_args->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument fun-args requires a list\n");
    return nullptr;
  }

  Parse_Node *new_fun = new Parse_Node{PARSE_NODE_FUNCTION, FUNCTION_NATIVE};
  new_fun->token = fun_name->token;
  new_fun->first = fun_args;
  new_fun->next = args->next->next;
  
  env->table[fun_name->token.name] = new_fun;
  return new_fun;
}

Parse_Node *builtin_defmacro(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs < 2) {
    fprintf(stderr, "Error: defmacro requires a macro-name argument, and an macro-args argument\n");
    return nullptr;
  }

  Parse_Node *macro_name = args->first;
  if (macro_name->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: argument macro-name requires a symbol\n");
    return nullptr;
  }

  Parse_Node *macro_args = args->next->first;
  if (macro_args->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument macro-args requires a list\n");
    return nullptr;
  }

  Parse_Node *new_macro = new Parse_Node{PARSE_NODE_FUNCTION, FUNCTION_MACRO};
  new_macro->token = macro_name->token;
  new_macro->first = macro_args; //reuse the cons cells slots
  new_macro->next = args->next->next;
  
  env->table[macro_name->token.name] = new_macro;
  return new_macro;
}

Parse_Node *builtin_defsym(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2) {
    fprintf(stderr, "defsym takes exactly two arguments.\nInstead received %d\n", nargs);
    return nullptr;
  }

  Parse_Node *sym = args->first;
  
  if (sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "first argument given to defsym was not a symbol\n");
    return nullptr;
  }
  
  // std::map<std::string, Parse_Node *>::iterator it;
  // it = env.table.find(sym.token.name);
  // if (it == env.table.end()) {
  //   //
  // }

  Parse_Node *val = eval_parse_node(args->next->first, env);
  env->table[sym->token.name] = val;
  
  return sym;
}

Parse_Node *builtin_eval(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: eval takes exactly one argument\n");
    return nullptr;
  }
  Parse_Node *arg = eval_parse_node(args->first, env);
  return eval_parse_node(arg, env);
}

Parse_Node *builtin_progn(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();

  Parse_Node *cur = args;
  Parse_Node *ret = cur;
  
  while (cur->first != nullptr) {
    ret = eval_parse_node(cur->first, env);
    cur = cur->next;
  }

  return ret;
}

Parse_Node *builtin_list(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *list = new Parse_Node{PARSE_NODE_LIST};
  Parse_Node *cur = list;
  Parse_Node *arg = args;
  while (arg->first != nullptr) {
    cur->first = eval_parse_node(arg->first, env);
    cur->next = new Parse_Node{PARSE_NODE_LIST};
    cur = cur->next;
    arg = arg->next;
  }
  return list;
}

Parse_Node *builtin_first(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: first takes exactly one argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (earg->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument %s not a list\n", earg->cprint());
  }

  if (earg->first == nullptr) {
    return earg;
  }  

  return earg->first;
}

Parse_Node *builtin_last(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: last takes exactly one argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (earg->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument %s not a list\n", earg->cprint());
  }

  if (earg->first == nullptr) {
    return earg;
  }

  Parse_Node *prev = earg->first;
  Parse_Node *cur = earg->next;
  while (cur->first != nullptr) {
    prev = cur->first;
    cur = cur->next;
  }

  return prev;
}

Parse_Node *builtin_pop(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: pop takes exactly one argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (earg->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument %s not a list\n", earg->cprint());
  }

  if (earg->first == nullptr) {
    return earg;
  }  

  return earg->next;
}

Parse_Node *builtin_push(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: pop takes exactly two argument\n");
    return nullptr;
  }
  
  Parse_Node *earg2 = eval_parse_node(args->next->first, env);
  if (earg2->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument %s not a list\n", earg2->cprint());
  }

  Parse_Node *earg1 = eval_parse_node(args->first, env);
  Parse_Node *ret = new Parse_Node{PARSE_NODE_LIST};
  ret->first = earg1;
  ret->next = earg2;

  return ret;
}

Parse_Node *builtin_nth(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: nth takes exactly two argument\n");
    return nullptr;
  }

  Parse_Node *earg1 = eval_parse_node(args->first, env);
  if (earg1->subtype != LITERAL_INTEGER) {
    fprintf(stderr, "Error: argument %s not an integer\n", earg1->cprint());
  }
  
  Parse_Node *earg2 = eval_parse_node(args->next->first, env);
  if (earg2->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument %s not a list\n", earg2->cprint());
  }

  int nth = earg1->val.u64;
  Parse_Node *cur = earg2;
  for (int i = 1; i < nth; ++i) {
    if (cur->first == nullptr) {
      return cur;
    }
    cur = cur->next;    
  }
  if (cur->first == nullptr) {
    return cur;
  }
  return cur->first;
}

Parse_Node *builtin_let(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr, "Error: too few arguments in `let`\n");
    return nullptr;
  }

  Symbol_Table *let_env = new Symbol_Table(env);
  Parse_Node *defs = args->first;

  if (defs->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: invalid let bindings (not a list)\n");
    return nullptr;
  }
  
  while (defs->first != nullptr) {
    Parse_Node *let_form = defs->first;

    Parse_Node *sym;
    Parse_Node *val;      
    switch (let_form->type) {
    case PARSE_NODE_LIST: {

      int lfs = let_form->length();
      if (lfs != 2 && lfs != 1) {
	fprintf(stderr, "Error: invalid let binding: %s\n"
		"     : let form length %d\n"
		, let_form->cprint(), lfs);
	return nullptr;
      }
     
      sym = let_form->first;

      if (sym->type != PARSE_NODE_SYMBOL) {
	fprintf(stderr, "Error: invalid let binding: `%s` is not a symbol\n", sym->cprint());
	return nullptr;
      }
      
      if (lfs == 1) {
	val = new Parse_Node{PARSE_NODE_LIST};
      } else {
	val = eval_parse_node(let_form->next->first, let_env);
      }
      break;
    }
    case PARSE_NODE_SYMBOL: {
      sym = let_form;
      val = new Parse_Node{PARSE_NODE_LIST};
      break;
    }
    default: {
      fprintf(stderr, "Error: invalid let binding: %s\n", let_form->cprint());
      return nullptr;
      break;
    }
    }
    let_env->table[sym->token.name] = val;
    defs = defs->next;
  }

  if (args->next->first == nullptr) {
    return new Parse_Node{PARSE_NODE_LIST};
  }

  Parse_Node *ret;
  Parse_Node *cur = args->next;  
  while (cur->first != nullptr) {
    ret = eval_parse_node(cur->first, let_env);
    cur = cur->next;
  }
  return ret;  
}

Parse_Node *builtin_if(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2 && nargs != 3) {
    fprintf(stderr, "Error: incorrect number of arguments\n");
    return nullptr;
  }

  Parse_Node *condition = eval_parse_node(args->first, env);

  if (condition->subtype != LITERAL_BOOLEAN) {
    fprintf(stderr, "Error: if statement condition did not evaluate to boolean\n");
    return nullptr;
  }

  Parse_Node *ret;
  if (condition->val.b == true) {
    ret = eval_parse_node(args->next->first, env);
  } else if (nargs == 2) {
    ret = new Parse_Node{PARSE_NODE_LIST};
  } else {
    ret = eval_parse_node(args->next->next->first, env);
  }
  return ret;
}

Parse_Node *builtin_add(Parse_Node *args, Symbol_Table *env) {
  int64_t sum = 0;
  double fsum = 0;

  Parse_Node *cur = args;
  while (cur->first != nullptr) {
    Parse_Node *arg = eval_parse_node(cur->first, env);
    if (arg->type != PARSE_NODE_LITERAL) {
      fprintf(stderr, "+ was given non literal to add\n");
      return nullptr;
    }
    
    if (arg->subtype == LITERAL_FLOAT) {
      fsum += arg->val.dub;
    } else if (arg->subtype == LITERAL_INTEGER) {
      sum += arg->val.u64;
    } else {
      fprintf(stderr, "argument not a number\n");
      return nullptr;
    }
    cur = cur->next;
  }
  
  Parse_Node *s = new Parse_Node{PARSE_NODE_LITERAL};
  if (fsum != 0.0) {
    s->subtype = LITERAL_FLOAT;
    s->val.dub = fsum + sum;
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

  Parse_Node *cur = args;
  while (cur->first != nullptr) {
    Parse_Node *arg = eval_parse_node(cur->first, env);
    
    if (arg->subtype == LITERAL_FLOAT) {
      fprod *= arg->val.dub;
    } else if (arg->subtype == LITERAL_INTEGER) {
      prod *= arg->val.u64;
    } else {
      fprintf(stderr, "* was given non number to multiply\n");
    }
    cur = cur->next;
  }

  Parse_Node *s = new Parse_Node{PARSE_NODE_LITERAL};
  if (fprod != 1) {
    s->subtype = LITERAL_FLOAT;
    s->val.dub = fprod * prod;
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

Parse_Node *builtin_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr, "Error: not enough arguments\n");
    return nullptr;
  }

  double dval;
  int val;

  Parse_Node *first = eval_parse_node(args->first, env);
  if (!is_number(first)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  bool is_int = (first->subtype == LITERAL_INTEGER);
  if (is_int) {
    val = first->val.u64;
  } else {
    dval = first->val.dub;
  }

  Parse_Node *cur = args->next;
  while(cur->first != nullptr) {
    Parse_Node *ecur = eval_parse_node(cur->first, env);
    if (!is_number(ecur)) {
	fprintf(stderr, "Error: argument %s, is not a number",
		cur->first->cprint());
	return nullptr;      
      }
    bool cur_int = (ecur->subtype == LITERAL_INTEGER);          
    if (is_int) {
      if (cur_int) {
	if (! (val == ecur->val.u64)) {return fal;}
      } else {
	if (! (val == ecur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (dval == ecur->val.u64)) {return fal;}
      } else {
	if (! (dval == ecur->val.dub)) {return fal;}
      }
    }
    cur = cur->next;
  }
  return tru;  
}

Parse_Node *builtin_less_than(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  Parse_Node *prev = eval_parse_node(args->first, env);
  bool prev_int = (prev->subtype == LITERAL_INTEGER);
  
  if (!is_number(prev)) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    prev->cprint());
    return nullptr;
  }
  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    Parse_Node *ecur = eval_parse_node(cur->first, env);
    if (!is_number(ecur)) {
      fprintf(stderr, "Error: argument %s, is not a number",
	      cur->first->cprint());
      return nullptr;      
    }
    bool cur_int = (ecur->subtype == LITERAL_INTEGER);
    if (prev_int) {
      if (cur_int) {
	if (! (prev->val.u64 < ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.u64 < ecur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (prev->val.dub < ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.dub < ecur->val.dub)) {return fal;}
      }
    }
    prev = ecur;
    prev_int = cur_int;
    cur = cur->next;
  }
  return tru;
}

Parse_Node *builtin_less_than_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  Parse_Node *prev = eval_parse_node(args->first, env);
  bool prev_int = (prev->subtype == LITERAL_INTEGER);
  
  if (!is_number(prev)) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    prev->cprint());
    return nullptr;
  }
  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    Parse_Node *ecur = eval_parse_node(cur->first, env);
    if (!is_number(ecur)) {
      fprintf(stderr, "Error: argument %s, is not a number",
	      cur->first->cprint());
      return nullptr;      
    }
    bool cur_int = (ecur->subtype == LITERAL_INTEGER);
    if (prev_int) {
      if (cur_int) {
	if (! (prev->val.u64 <= ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.u64 <= ecur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (prev->val.dub <= ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.dub <= ecur->val.dub)) {return fal;}
      }
    }    
    prev = ecur;
    prev_int = cur_int;
    cur = cur->next;
  }
  return tru;
}

Parse_Node *builtin_greater_than(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  Parse_Node *prev = eval_parse_node(args->first, env);
  bool prev_int = (prev->subtype == LITERAL_INTEGER);
  
  if (!is_number(prev)) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    prev->cprint());
    return nullptr;
  }
  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    Parse_Node *ecur = eval_parse_node(cur->first, env);
    if (!is_number(ecur)) {
      fprintf(stderr, "Error: argument %s, is not a number",
	      cur->first->cprint());
      return nullptr;      
    }
    bool cur_int = (ecur->subtype == LITERAL_INTEGER);
    if (prev_int) {
      if (cur_int) {
	if (! (prev->val.u64 > ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.u64 > ecur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (prev->val.dub > ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.dub > ecur->val.dub)) {return fal;}
      }
    }    
    prev = ecur;
    prev_int = cur_int;
    cur = cur->next;
  }
  return tru;
}

Parse_Node *builtin_greater_than_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr,"Error: not enough arguments\n");
    return nullptr;
  }
  Parse_Node *prev = eval_parse_node(args->first, env);
  bool prev_int = (prev->subtype == LITERAL_INTEGER);
  
  if (!is_number(prev)) {
    fprintf(stderr, "Error: argument 1, %s, is not a number\n",
	    prev->cprint());
    return nullptr;
  }
  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    Parse_Node *ecur = eval_parse_node(cur->first, env);
    if (!is_number(ecur)) {
      fprintf(stderr, "Error: argument %s, is not a number",
	      cur->first->cprint());
      return nullptr;      
    }
    bool cur_int = (ecur->subtype == LITERAL_INTEGER);
    if (prev_int) {
      if (cur_int) {
	if (! (prev->val.u64 >= ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.u64 >= ecur->val.dub)) {return fal;}
      }
    } else {
      if (cur_int) {
	if (! (prev->val.dub >= ecur->val.u64)) {return fal;}
      } else {
	if (! (prev->val.dub >= ecur->val.dub)) {return fal;}
      }
    }    
    prev = ecur;
    prev_int = cur_int;
    cur = cur->next;
  }
  return tru;
}

Parse_Node *builtin_and(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->first->cprint());
      return nullptr;
    }
    if (earg->val.b == false) {
      return fal;
    }    
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_or(Parse_Node *args, Symbol_Table *env) {
  while(args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (!is_bool(earg)) {
      fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	      args->first->cprint());
      return nullptr;
    }
    if (earg->val.b == true) {
      return tru;
    }    
    args = args->next;
  }
  return fal;
}

Parse_Node *builtin_not(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: not takes exactly 1 argument\n");
  }
  Parse_Node *earg = eval_parse_node(args->first, env);
  if (!is_bool(earg)) {
    fprintf(stderr, "Error: argument does not evaluate to boolean: %s\n",
	    args->first->cprint());
    return nullptr;
  }
  if (earg->val.b) {
    return fal;
  }
  return tru;
}

Parse_Node *builtin_bitand(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: takes 2+ arguments\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = earg->val.u64;
  } else {
    val = *((int64_t *)(&earg->val.dub));
  }

  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    earg = eval_parse_node(cur->first, env);

    if(!is_number(earg)) {
      fprintf(stderr, "Error: argument %s is not a number\n", cur->first->cprint());
      return nullptr;
    }
    
    if (earg->subtype == LITERAL_INTEGER) {
      val &= earg->val.u64;
    } else {
      val &= *((int64_t *)(&earg->val.dub));
    }
    cur = cur->next;
  }
  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
}

Parse_Node *builtin_bitor(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: takes 2+ arguments\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = earg->val.u64;
  } else {
    val = *((int64_t *)(&earg->val.dub));
  }

  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    earg = eval_parse_node(cur->first, env);

    if(!is_number(earg)) {
      fprintf(stderr, "Error: argument %s is not a number\n", cur->first->cprint());
      return nullptr;
    }
    
    if (earg->subtype == LITERAL_INTEGER) {
      val |= earg->val.u64;
    } else {
      val |= *((int64_t *)(&earg->val.dub));
    }
    cur = cur->next;
  }
  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
}

Parse_Node *builtin_bitxor(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: takes 2+ arguments\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = earg->val.u64;
  } else {
    val = *((int64_t *)(&earg->val.dub));
  }

  Parse_Node *cur = args->next;
  while (cur->first != nullptr) {
    earg = eval_parse_node(cur->first, env);

    if(!is_number(earg)) {
      fprintf(stderr, "Error: argument %s is not a number\n", cur->first->cprint());
      return nullptr;
    }
    
    if (earg->subtype == LITERAL_INTEGER) {
      val ^= earg->val.u64;
    } else {
      val ^= *((int64_t *)(&earg->val.dub));
    }
    cur = cur->next;
  }
  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
}

Parse_Node *builtin_bitnot(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: takes 1 argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = ~earg->val.u64;
  } else {
    val = ~*((int64_t *)(&earg->val.dub));
  }

  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
}

Parse_Node *builtin_bitshift_left(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2 && nargs != 1) {
    fprintf(stderr, "Error: takes 1 or 2 arguments\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = earg->val.u64;
  } else {
    val = *((int64_t *)(&earg->val.dub));
  }

  if (nargs == 1) {
    val <<= 1; 
  } else {
    Parse_Node *earg = eval_parse_node(args->next->first, env);
    if(earg->subtype != LITERAL_INTEGER) {
      fprintf(stderr, "Error: argument %s is not an integer\n", args->next->first->cprint());
      return nullptr;
    }
    val <<= earg->val.u64;    
  }

  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
}

Parse_Node *builtin_bitshift_right(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2 && nargs != 1) {
    fprintf(stderr, "Error: takes 1 or 2 arguments\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);

  if(!is_number(earg)) {
    fprintf(stderr, "Error: argument %s is not a number\n", args->first->cprint());
    return nullptr;
  }
  
  int64_t val;
  if (earg->subtype == LITERAL_INTEGER) {
    val = earg->val.u64;
  } else {
    val = *((int64_t *)(&earg->val.dub));
  }

  if (nargs == 1) {
    val >>= 1; 
  } else {
    Parse_Node *earg = eval_parse_node(args->next->first, env);
    if(earg->subtype != LITERAL_INTEGER) {
      fprintf(stderr, "Error: argument %s is not an integer\n", args->next->first->cprint());
      return nullptr;
    }
    val >>= earg->val.u64;    
  }

  Parse_Node *r = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  r->val.u64 = val;
  return r;
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

  create_builtin("defun", builtin_defun, &env);
  create_builtin("defmacro", builtin_defmacro, &env);
  create_builtin("defsym", builtin_defsym, &env);
  create_builtin("let", builtin_let, &env);
  create_builtin("progn", builtin_progn, &env);
  create_builtin("if", builtin_if, &env);
  create_builtin("eval", builtin_eval, &env);

  create_builtin("list", builtin_list, &env);
  create_builtin("first", builtin_first, &env);
  create_builtin("last", builtin_last, &env);
  create_builtin("pop", builtin_pop, &env);
  create_builtin("push", builtin_push, &env);
  create_builtin("nth", builtin_nth, &env);

  create_builtin("+", builtin_add, &env);
  create_builtin("*", builtin_multiply, &env);
  
  create_builtin("=", builtin_equal, &env);
  create_builtin("<", builtin_less_than, &env);
  create_builtin("<=", builtin_less_than_equal, &env);
  create_builtin(">", builtin_greater_than, &env);
  create_builtin(">=", builtin_greater_than_equal, &env);

  create_builtin("and", builtin_and, &env);
  create_builtin("or", builtin_or, &env);
  create_builtin("not", builtin_not, &env);
  
  create_builtin("&", builtin_bitand, &env);
  create_builtin("|", builtin_bitor, &env);
  create_builtin("^", builtin_bitxor, &env);
  create_builtin("~", builtin_bitnot, &env);
  create_builtin("<<", builtin_bitshift_left, &env);
  create_builtin(">>", builtin_bitshift_right, &env);

  return env;
}
