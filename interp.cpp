#include <iostream>
#include <exception>
#include <limits>
#include "interp.h"
#include "builtin_math.h"
#include "builtin_logic.h"
#include "interp_exceptions.h"

Parse_Node *eval_list(Parse_Node *node, Symbol_Table *env);
Parse_Node *eval_backtick(Parse_Node *node, Symbol_Table *env);
Parse_Node *eval_backtick_list(Parse_Node *node, Symbol_Table *env);
Parse_Node *expand_splice(Parse_Node *node, Symbol_Table *env);
Parse_Node *expand_eval_macro(Parse_Node *func, Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_fun(Parse_Node *fun, Parse_Node *node, Symbol_Table *env, bool is_fun);

bool is_integer(Parse_Node *node) {
  return (node->subtype == LITERAL_INTEGER);
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

Parse_Node *eval_parse_node(Parse_Node *node, Symbol_Table *env) {
  try {
    switch (node->type) {
    case PARSE_NODE_LITERAL: {
      return node;
    }

    case PARSE_NODE_LIST: {
      return eval_list(node, env);
      break;
    }
    
    case PARSE_NODE_SYMBOL: {
      if (is_keyword(node)) {
	return node;
      }
    
      return env->lookup(node->token.name);
    }
    case PARSE_NODE_SYNTAX: {
      switch (node->subtype) {
      case SYNTAX_QUOTE: {
	return node->first;      
      }
      case SYNTAX_BACKTICK: {
	return eval_backtick(node->first, env);
      }
      case SYNTAX_COMMA: {
	throw runtimeError("Error: comma found not inside backtick\n");
      }
      case SYNTAX_COMMA_AT: {
	throw runtimeError("Error: ,@ found not inside backtick or unquoted list\n");
      }
      }

      break;
    }
    default:
      fprintf(stderr, "Error: could not evaluate unknown type\n");
      return nullptr;
    }
  } catch (runtimeError e) {
    fprintf(stderr, "%s", e.what());
    return new Parse_Node{PARSE_NODE_ERROR};
  }
  fprintf(stderr, "Error: ran into unhandled parse_node to eval\n");
  return nullptr;
}

Parse_Node *eval_list(Parse_Node *node, Symbol_Table *env) {
  // The empty list evaluates to itself
  if (node->first == nullptr) {
    return node;
  }  
  node = expand_splice(node, env);

  Parse_Node *func_sym = node->first;
  if (func_sym->type != PARSE_NODE_SYMBOL) {
    throw runtimeError( "Error: invalid function call, " + func_sym->print() + " is not a symbol\n" );
  }

  Parse_Node *func = env->lookup(func_sym->token.name);
  if (func == nullptr) {
    throw runtimeError("Error: could not find function or macro named " + func_sym->print() + "\n");
  }
  
  switch (func->subtype) {
  case FUNCTION_BUILTIN: 
    return func->val.func(node->next, env);
  
  case FUNCTION_MACRO: 
    return expand_eval_macro(func, node, env);

  case FUNCTION_NATIVE:
    return apply_fun(func, node, env, true);
      
  default: 
    throw runtimeError("Error: unknown function subtype\n");
  }
}

Parse_Node *expand_splice(Parse_Node *node, Symbol_Table *env) {
  // printf(" expanding splice in list %s\n", node->print().c_str());
  
  Parse_Node *ret = node;
  Parse_Node *prev = nullptr;
  
  while (node->first != nullptr) {
    // printf(" node: %s\n", node->first->print().c_str());
    // printf(" type: %s\n", node->first->print_type().c_str());
    // printf(" subtype: %s\n", node->first->print_subtype().c_str());
    
    if (node->first->subtype == SYNTAX_COMMA_AT) {
      Parse_Node *elist = eval_parse_node(node->first->first, env);
      if (!is_list(elist)) {
	throw runtimeError("Error: ,@ can only be used with list, "
			   + node->first->first->print() +
			   " is not a list\n");
      }
      if (prev == nullptr) {
	ret = elist;
      } else {
	prev->next = elist;
      }
      Parse_Node *cur = elist;
      while (!is_empty_list(cur->next)) {
	cur = cur->next;
      }
      cur->next = node->next;
      prev = cur;
    } else {
      prev = node;
    }
    node = node->next;
  }
  // printf(" after expanding splice %s\n", ret->print().c_str());
  return ret;
}


Parse_Node *apply_fun(Parse_Node *fun, Parse_Node *node, Symbol_Table *env, bool is_fun) {
  // if is_fun we evaluate the arguments, if not is_fun then it's a macro so no argument evaluation
  Symbol_Table *fun_env = new Symbol_Table(env);
  
  // bind given arguments to symbols in fun-params
  Parse_Node *cur_param = fun->first;
  Parse_Node *cur_arg = node->next;

  // printf("apply_fun: applying args to %s %s\n", is_fun ? "function" : "macro", fun->token.name.c_str());
  
  while (!is_empty_list(cur_param)) {
    Parse_Node *param = cur_param->first;
    Parse_Node *arg;

    if (param->token.name == "&rest") {
      cur_param = cur_param->next;
      param = cur_param->first;

      if (is_fun) {
	Parse_Node *rest = new Parse_Node{PARSE_NODE_LIST};
	Parse_Node *cur_rest = rest;
	while (!is_empty_list(cur_arg)) {
	  cur_rest->first = eval_parse_node(cur_arg->first, env);
	  cur_rest->next = new Parse_Node{PARSE_NODE_LIST};
	  cur_rest = cur_rest->next;
	  cur_arg = cur_arg->next;
	}
        arg = rest;
	
      } else {
        arg = cur_arg;
      }
      // cur_param = cur_param->next;
      fun_env->table[param->token.name] = arg;

      // printf("apply_fun: arg is %s\n", arg->print().c_str());
      
      break;
      
    } else if (param->token.name == "&opt" || param->token.name == "&optional") {

      cur_param = cur_param->next;
      while (!is_empty_list(cur_param)) {
	if (is_list(cur_param->first)) { // if the paramater has a default...
	  Parse_Node *sym = cur_param->first->first;

	  if (is_empty_list(cur_arg)) { // evaluate the default if no arg provided
	    Parse_Node *val = cur_param->first->next->first;
	    arg = eval_parse_node(val, env);
	  } else { 
	    arg = eval_parse_node(cur_arg->first, env);
	  }
	  fun_env->table[sym->token.name] = arg;
	  
	} else {
	  Parse_Node *sym = cur_param->first;
	  if (is_empty_list(cur_arg)) {
	    fun_env->table[sym->token.name] = fal;
	  } else {
	    if (is_fun) {
	      arg = eval_parse_node(cur_arg->first, env);
	    } else {
	      arg = cur_arg->first;
	    }
	    fun_env->table[sym->token.name] = arg;
	  }
	}
	cur_param = cur_param->next;
	cur_arg = cur_arg->next;
      }
      continue;
      
    } else {
      if (is_fun) {
	arg = eval_parse_node(cur_arg->first, env);
      } else {
	arg = cur_arg->first;
      }
    }
    
    fun_env->table[param->token.name] = arg;
    cur_param = cur_param->next;
    cur_arg = cur_arg->next;
  }
  
  // evaluate the body  
  Parse_Node *cur = fun->next;
  
  Parse_Node *ret;
  while (!is_empty_list(cur)) {
    try {
      ret = eval_parse_node(cur->first, fun_env);
    } catch (returnException e) {
      return e.ret;
    }
    cur = cur->next;    
  }
  return ret;
}


Parse_Node *eval_backtick(Parse_Node *node, Symbol_Table *env) {
  if (node->subtype == SYNTAX_COMMA) {
    return eval_parse_node(node->first, env);
  }
  
  if (is_list(node)) {
    node = eval_backtick_list(node, env);
  }
  return node;
}

Parse_Node *eval_backtick_list(Parse_Node *node, Symbol_Table *env) {  
  node = expand_splice(node, env);  
  Parse_Node *ret = new Parse_Node{PARSE_NODE_LIST};
  Parse_Node *list = ret;
  while (!is_empty_list(node)) {
    list->first = eval_backtick(node->first, env);
    list->next = new Parse_Node{PARSE_NODE_LIST};
    list = list->next;
    node = node->next;
  }  
  return ret;
}

Parse_Node *expand_eval_macro(Parse_Node *macro, Parse_Node *node, Symbol_Table *env) {
  Parse_Node *expand = apply_fun(macro, node, env, false);
  return eval_parse_node(expand, env);
}

Parse_Node *builtin_expand(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    throw runtimeError("Error: expand requires at least one argument\n");
  }

  Parse_Node *macro_sym = args->first;
  if (!is_sym(macro_sym)) {
    throw runtimeError("Error: invalid expand call, " + macro_sym->print() + " is not a symbol\n" );
  }

  Parse_Node *macro = env->lookup(macro_sym->token.name);
  if (macro == nullptr || macro->subtype != FUNCTION_MACRO) {
    throw runtimeError("Error: could not find macro named " + macro_sym->print() + "\n");
  }
  return apply_fun(macro, args, env, false);
}

Parse_Node *builtin_quote(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *q = new Parse_Node{PARSE_NODE_SYNTAX, SYNTAX_QUOTE};
  q->first = args;
  return q;
}

Parse_Node *build_function(Parse_Node *args, Symbol_Table *env, bool is_fun) {
  std::string obj_type = (is_fun ? "function" : "macro");
  std::string abbrev = (is_fun ? "func" : "macro");
  std::string builtin_name = (is_fun ? "defun" : "defmacro");  
  
  int nargs = args->length();
  if (nargs < 2) {
    throw runtimeError("Error: "
		       + builtin_name + " requires a " +
		       abbrev + "-name argument, and a " +
		       abbrev +  "-params argument\n");
  }

  Parse_Node *fun_name = args->first;
  if (!is_sym(fun_name)) {
    throw runtimeError("Error: argument " + abbrev +  "-name requires a symbol\n");
  }

  
  Parse_Node *fun_params = args->next->first;
  if (!is_list(fun_params)) {
    throw runtimeError("Error: argument " + abbrev + "-params requires a list\n");
  }

  Parse_Node *param_list = fun_params;
  while (!is_empty_list(param_list)) {
    Parse_Node *param = param_list->first;

    if (!is_sym(param)) {
      throw runtimeError("Error: " + obj_type
			  +
			 " parameter `" + param->print() +
			 "` is not a symbol\n");
    }
    
    if (param->token.name == "&rest") {
      param_list = param_list->next;
      param = param_list->first;
      if (param == nullptr) {
	throw runtimeError("Error: paramater required after &rest\n");
      }

      param_list = param_list->next;
      if (param_list->first != nullptr) {
	throw runtimeError("Error: there can only be one parameter after &rest\n");
      }
      continue;
    }

    if (param->token.name == "&opt" || param->token.name == "&optional") {
      param_list = param_list->next;
      param = param_list->first;
      if (param == nullptr) {
	throw runtimeError("Error: one or more paramater required after &optional\n");
      }

      param_list = param_list->next;
      while (!is_empty_list(param_list)) {
	param = param_list->first;
	if (is_list(param)) {
	  if (param->length() != 2) {
	    throw runtimeError("Error: two items required in lists after &optional\n"
	                       "       A symbol, and a default value\n");
	  }
	  if (!is_sym(param->first)) {
	    throw runtimeError("Error: first item in parameter default list " +
			       param->print() +
			       + " is not a symbol\n");
	  }
	}
	param_list = param_list->next;
      }
      continue;
    }
    param_list = param_list->next;
  }

  Parse_Node *new_fun = new Parse_Node{PARSE_NODE_FUNCTION};
  if (is_fun) {
    new_fun->subtype = FUNCTION_NATIVE;
  } else {
    new_fun->subtype = FUNCTION_MACRO;
  }
  new_fun->token = fun_name->token;
  new_fun->first = fun_params;
  new_fun->next = args->next->next;

  // printf("defining function %s: %s, %s\n",
  // 	 fun_name->token.name.c_str(),
  // 	 new_fun->first->print().c_str(),
  // 	 new_fun->next->print().c_str());
  
  env->table[fun_name->token.name] = new_fun;
  return new_fun;
}

Parse_Node *builtin_defun(Parse_Node *args, Symbol_Table *env) {
  return build_function(args, env, true);
}

Parse_Node *builtin_return(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *earg = eval_parse_node(args->first, env);
  throw returnException(earg);
}

Parse_Node *builtin_defmacro(Parse_Node *args, Symbol_Table *env) {
  return build_function(args, env, false);
}

Parse_Node *builtin_defsym(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2) {
    throw runtimeError("Error: defsym takes exactly two arguments.\n");
  }

  Parse_Node *sym = args->first;
  
  if (!is_sym(sym)) {
    throw runtimeError("Error: first argument given to defsym was not a symbol\n");
  }

  if(is_keyword(sym)) {
    throw runtimeError("Error: keyword symbols cannot be reassigned\n");
  }

  Parse_Node *val = eval_parse_node(args->next->first, env);
  env->table[sym->token.name] = val;
  Parse_Node *inserted =   env->table[sym->token.name];  
  return sym;
}

Parse_Node *builtin_eval(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    throw runtimeError("Error: eval takes exactly one argument\n");
  }
  Parse_Node *arg = eval_parse_node(args->first, env);
  return eval_parse_node(arg, env);
}

Parse_Node *builtin_progn(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();

  Parse_Node *cur = args;
  Parse_Node *ret = cur;
  
  while (!is_empty_list(cur)) {
    ret = eval_parse_node(cur->first, env);
    cur = cur->next;
  }
  return ret;
}

Parse_Node *builtin_list(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *list = new Parse_Node{PARSE_NODE_LIST};
  Parse_Node *cur = list;
  Parse_Node *arg = args;
  while (!is_empty_list(arg)) {
    cur->first = eval_parse_node(arg->first, env);
    cur->next = new Parse_Node{PARSE_NODE_LIST};
    cur = cur->next;
    arg = arg->next;
  }
  return list;
}

Parse_Node *set_first(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    throw runtimeError("Error: first takes exactly one argument\n");
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if(is_keyword(earg)) {
    throw runtimeError("keyword symbols cannot be reassigned\n");
  }
  
  if (!is_list(earg) && !is_string(earg)) {
    throw runtimeError("Error: argument " + earg->print() + " not a list\n");
  }
  return earg;
}

Parse_Node *builtin_first(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *ret = set_first(args, env);
  if (ret == nullptr || ret->first == nullptr) {
    return ret;    
  }
  if (is_string(ret)) {
    Parse_Node *n = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_STRING};
    n->token.name = ret->token.name[0];
    return n;
  }
  return ret->first;
}

Parse_Node *set_last(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    throw runtimeError("Error: last takes exactly one argument\n");
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (!is_list(earg) && !is_string(earg)) {
    throw runtimeError("Error: argument " + earg->print() + " not a sequence\n");
  }
  if (is_empty_list(earg) || is_string(earg)) {
    return earg;
  }

  Parse_Node *prev = earg->first;
  Parse_Node *cur = earg->next;
  while (!is_empty_list(cur)) {
    prev = cur;
    cur = cur->next;
  }
  return prev;
}

Parse_Node *builtin_last(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *ret = set_last(args, env);
  if (ret == nullptr || is_empty_list(ret)) {
    return ret;  
  }
  if (is_string(ret)) {
    Parse_Node *n = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_STRING};
    int l = ret->token.name.length() - 1;
    n->token.name = ret->token.name[l];
    return n;
  }  
  return ret->first;
}

Parse_Node *set_nth(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    throw runtimeError("Error: nth takes exactly two argument\n");
  }

  Parse_Node *n = eval_parse_node(args->first, env);
  if (!is_integer(n)) {
    throw runtimeError("Error: argument " + n->print() + " not an integer\n");
  }
  
  Parse_Node *seq = eval_parse_node(args->next->first, env);
  if (!is_sequence(seq)) {
    throw runtimeError("Error: argument " + seq->print() + " not a sequence\n");
  }

  int nth = n->val.u64;

  if (is_string(seq)) {
    --nth;
    int len = seq->token.name.length();
    if (nth >= len) {
      nth = len - 1;
    }
    seq->val.u64 = nth;  //I won't tell anyone if you won't
    return seq;
  }
  
  Parse_Node *cur = seq;
  for (int i = 1; i < nth; ++i) {
    if (cur->first == nullptr) {
      return cur;
    }
    cur = cur->next;
  }
  if (cur->first == nullptr) {
    return cur;
  }
  return cur;
}

Parse_Node *builtin_nth(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *ret = set_nth(args, env);
  if (ret == nullptr || ret->first == nullptr || ret->subtype == LITERAL_STRING) {
    return ret;  
  }
  if (is_string(ret)) {
    Parse_Node *n = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_STRING};
    n->token.name = ret->token.name[ret->val.u64];
    return n;
  }  
  return ret->first;  
}

Parse_Node *builtin_pop(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    throw runtimeError("Error: pop takes exactly one argument\n");
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (!is_list(earg)) {
    throw runtimeError("Error: argument " + earg->print() + " not a list\n");
  }
  if (is_empty_list(earg)) {
    return earg;
  }
  return earg->next;
}

Parse_Node *builtin_push(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    throw runtimeError("Error: pop takes exactly two argument\n");
  }
  Parse_Node *earg2 = eval_parse_node(args->next->first, env);
  if (!is_list(earg2)) {
    throw runtimeError("Error: argument " + earg2->print() + " not a list\n");
  }
  Parse_Node *earg1 = eval_parse_node(args->first, env);
  Parse_Node *ret = new Parse_Node{PARSE_NODE_LIST};
  ret->first = earg1;
  ret->next = earg2;
  return ret;
}

Parse_Node *builtin_append(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: append takes exactly two argument\n");
    return nullptr;
  }

  Parse_Node *list = eval_parse_node(args->next->first, env);
  Parse_Node *end = list;
  if (list->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: first argument to append, %s, is not a list\n", list->print().c_str());
    return nullptr;
  }

  while (end->first != nullptr) {
    end = end->next;
  }

  Parse_Node *earg1 = eval_parse_node(args->first, env);
  end->first = earg1;
  end->next = new Parse_Node{PARSE_NODE_LIST};
  return list;
}

// concatenate the string representation of the arguments
Parse_Node *builtin_string_concatenate(Parse_Node *args, Symbol_Table *env) {
  std::string con = "";
  while (!is_empty_list(args)) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (is_string(earg)) {
      con += earg->token.name;
    } else {
      con += earg->print();
    }
    args = args->next;
  }
  Parse_Node *ret = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_STRING};
  ret->token.name = con;
  return ret;
}

Parse_Node *builtin_length(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: append takes exactly one argument\n");
    return nullptr;
  } 
  
  Parse_Node *list = eval_parse_node(args->first, env);
  if (list->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: argument to length is not a list\n");
    return nullptr;
  }

  Parse_Node *ret = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  ret->val.u64 = list->length();
  return ret;
}

Parse_Node *builtin_empty_q(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    throw runtimeError("Error: empty? takes exactly one argument\n");
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  
  if (!is_list(earg) || !is_empty_list(earg)) {
    return fal;
  }
  return tru;
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
		, let_form->print().c_str(), lfs);
	return nullptr;
      }
     
      sym = let_form->first;

      if (sym->type != PARSE_NODE_SYMBOL) {
	fprintf(stderr, "Error: invalid let binding: `%s` is not a symbol\n", sym->print().c_str());
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
      fprintf(stderr, "Error: invalid let binding: %s\n", let_form->print().c_str());
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

Parse_Node *builtin_print(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *ret = nullptr;
  while (args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    std::cout << earg->print() << std::endl;
    ret = earg;
    args = args->next;
  }
  if (ret == nullptr) {
    fprintf(stderr, "Error: print takes at least one argument\n");    
  }
  return ret;
}

Parse_Node *builtin_for_each(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs < 1) {
    fprintf(stderr, "Error: print takes at least one argument\n");
    return nullptr;
  }

  Parse_Node *binding = args->first;
  if (binding->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: for-each binding must be a list\n");
    return nullptr;
  }

  if (binding->length() != 2) {
    fprintf(stderr, "Error: for-each binding must be exactly two argument\n");
    return nullptr;
  }

  Parse_Node *sym = binding->first;
  if (sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: for-each binding first argument is not a symbol\n");
    return nullptr;    
  }

  Parse_Node *list = eval_parse_node(binding->next->first, env);
  if (list->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: for-each binding second argument is not a list\n");
    return nullptr;    
  }
  
  Symbol_Table *for_each_env = new Symbol_Table(env);
  Parse_Node *cur = list;
  Parse_Node *ret = args->first; // this way if there is no body the empty list is returned
  while (cur->first != nullptr) {
    for_each_env->table[sym->token.name] = cur->first;
    Parse_Node *body = args->next;
    while (body->first != nullptr) {
      ret = eval_parse_node(body->first, for_each_env);
      body = body->next;
    }
    cur = cur->next;
  }
  return ret;
}

bool evals_to_true(Parse_Node *node, Symbol_Table *env) {
  Parse_Node *val = eval_parse_node(node, env);
  if (!is_bool(val)) {
    fprintf(stderr, "Error: while condition didn't evaluate to a boolean\n");
    return false;
  }
  return val->val.b;
}

Parse_Node *builtin_while(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs < 2) {
    fprintf(stderr, "Error: while takes 2+ arguments\n", nargs);
    return nullptr;
  }  
  Parse_Node *condition = args->first;
  Parse_Node *body = args->next;
  Parse_Node *ret;

  while(evals_to_true(condition, env)) {
    Parse_Node *cur = body;
    while (!is_empty_list(cur)) {
      ret = eval_parse_node(cur->first, env);
      cur = cur->next;
    }
  }
  return ret;
}

Parse_Node *builtin_set(Parse_Node *args, Symbol_Table *env) {
  int nargs = args->length();
  if (nargs != 2) {
    fprintf(stderr, "defsym takes exactly two arguments.\nInstead received %d\n", nargs);
    return nullptr;
  }

  Parse_Node *sym = args->first;
  
  if (!is_list(sym) && !is_sym(sym)) {
    fprintf(stderr, "first argument given to defsym was not a symbol or list\n");
    return nullptr;
  }
  
  Parse_Node *val = eval_parse_node(args->next->first, env);
  
  if (is_sym(sym)) {    
    env->set(sym->token.name, val);  
    return sym;
  }

  Parse_Node *acc_form = sym;
  sym = sym->first;
  if (!is_sym(sym)) {
    fprintf(stderr, "set invalid accessor, %s is not a symbol\n", sym->print().c_str());
    return nullptr;
  }
  
  std::string sy = sym->token.name;
  
  Parse_Node *place;
  if (sy == "first") {
    place = set_first(acc_form->next, env);
    place->first = val;

  } else if (sy == "last") {
    place = set_last(acc_form->next, env);
    
  } else if (sy == "nth") {
    place = set_nth(acc_form->next, env);
    
  } else {
    fprintf(stderr, "Error: no set accessor named %s found\n", sy.c_str());
    return nullptr;
  }
  if (is_string(place)) {
    if (!is_string(val)) {
      fprintf(stderr, "Error: can only set a character to be a character\n");
      return nullptr;
    }
    char newc = val->token.name[0];
    if (sy == "first") {
      place->token.name[0] = newc;

    } else if (sy == "last") {
      int l = place->token.name.length() - 1;
      place->token.name[l] = newc;
    
    } else if (sy == "nth") {
      place->token.name[place->val.u64] = newc;
    
    } else {
      fprintf(stderr, "Error: no set accessor named %s found\n", sy.c_str());
      return nullptr;
    }
  }
  
  place->first = val;
  return val;
}

Parse_Node *single_type_of(Parse_Node *arg, Symbol_Table *env) {
  Parse_Node *node = eval_parse_node(arg, env);
  Parse_Node *ret = new Parse_Node{PARSE_NODE_SYMBOL};
  std::string name;
  switch (node->type) {
  case PARSE_NODE_LITERAL: {
    switch (node->subtype) {
    case LITERAL_INTEGER: {
      name = "integer";  
      break;
    }
    case LITERAL_FLOAT: {
      name = "float";
      break;
    }
    case LITERAL_STRING: {
      name = "string";
      break;
    }
    case LITERAL_BOOLEAN: {
      name = "boolean";
      break;
    }
    default:
      fprintf(stderr, "Error: subtype not found\n");
      return nullptr;
    }
    break;
  }

  case PARSE_NODE_LIST: {    
    name = "list";
    break;
  }
    
  case PARSE_NODE_SYMBOL: {
    name = "symbol";
    break;
  }

  default:
    fprintf(stderr, "Error: unknown type\n");
    return nullptr;
  }
  ret->token.name = name;
  return ret;
}

Parse_Node *builtin_type_of(Parse_Node *args, Symbol_Table *env) {
  if (is_empty_list(args->next)) {
    return single_type_of(args->first, env);
  }
  Parse_Node *list = new Parse_Node{PARSE_NODE_LIST};
  Parse_Node *cur = list;
  while (!is_empty_list(args)) {
    cur->first = single_type_of(args->first, env);
    cur->next = new Parse_Node{PARSE_NODE_LIST};
    cur = cur->next;
    args = args->next;
  }
  return list;
}

Parse_Node *builtin_type_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->length() == 1) {
    fprintf(stderr, "Error: type= take 2+ arguments\n");
    return nullptr;
  }
  Parse_Node *prev = eval_parse_node(args->first, env);
  args = args->next;
  while (!is_empty_list(args)) {
    Parse_Node *cur = eval_parse_node(args->first, env);
    if (cur->type != prev->type || cur->subtype != prev->subtype) {
      return fal;
    }
    prev = cur;
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_symbol_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->length() == 1) {
    fprintf(stderr, "Error: symbol= take 2+ arguments\n");
    return nullptr;
  }
  Parse_Node *first = eval_parse_node(args->first, env);
  if(!is_sym(first)) {
    fprintf(stderr, "Error: %s does not evaluate to a symbol\n", args->first->print().c_str());
    return nullptr;
  }
  
  args = args->next;
  while (!is_empty_list(args)) {
    Parse_Node *cur = eval_parse_node(args->first, env);
    if(!is_sym(cur)) {
      fprintf(stderr, "Error: %s does not evaluate to a symbol\n", args->first->print().c_str());
      return nullptr;
    }
    
    if (cur->token.name != first->token.name) {
      return fal;
    }
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_string_equal(Parse_Node *args, Symbol_Table *env) {
  if (args->length() == 1) {
    fprintf(stderr, "Error: string= take 2+ arguments\n");
    return nullptr;
  }
  Parse_Node *first = eval_parse_node(args->first, env);
  if(!is_string(first)) {
    fprintf(stderr, "Error: %s does not evaluate to a string\n", args->first->print().c_str());
    return nullptr;
  }
  
  args = args->next;
  while (!is_empty_list(args)) {
    Parse_Node *cur = eval_parse_node(args->first, env);
    if(!is_string(cur)) {
      fprintf(stderr, "Error: %s does not evaluate to a string\n", args->first->print().c_str());
      return nullptr;
    }
    
    if (cur->token.name != first->token.name) {
      return fal;
    }
    args = args->next;
  }
  return tru;
}


void load_file(std::string file_name, Symbol_Table *env) {
  Parser parse = Parser(file_name.c_str());
  parse.parse_top_level_expressions();

  for (int i = 0; i < parse.top_level_expressions.size(); i++) {
    Parse_Node *evaled = eval_parse_node(parse.top_level_expressions[i], env);
  }
}

Parse_Node *builtin_load(Parse_Node *args, Symbol_Table *env) {
  while (args->first != nullptr) {
    Parse_Node *earg = eval_parse_node(args->first, env);
    if (args->first->subtype != LITERAL_STRING) {
      fprintf(stderr, "Error: %s is not a string\n", earg->print().c_str());
      return nullptr;
    }
    load_file(earg->token.name, env);
    args = args->next;
  }
  return tru;
}

Parse_Node *builtin_get_int(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 0) {
    throw runtimeError("Error: get-int takes no arguments\n");
  }
  Parse_Node *ret = new Parse_Node{PARSE_NODE_LITERAL, LITERAL_INTEGER};
  std::cin >> ret->val.u64;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  return ret;
}

void create_builtin(std::string symbol, Parse_Node *(*func)(Parse_Node *, Symbol_Table *), Symbol_Table *env) {
  Parse_Node *f = new Parse_Node{PARSE_NODE_FUNCTION, FUNCTION_BUILTIN};
  f->val.func = func;
  f->token.name = symbol;
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
  create_builtin("return", builtin_return, &env);
  create_builtin("expand", builtin_expand, &env);
  create_builtin("defsym", builtin_defsym, &env);
  create_builtin("set", builtin_set, &env);
  create_builtin("let", builtin_let, &env);
  create_builtin("progn", builtin_progn, &env);
  create_builtin("if", builtin_if, &env);
  create_builtin("eval", builtin_eval, &env);
  create_builtin("print", builtin_print, &env);
  create_builtin("for-each", builtin_for_each, &env);
  create_builtin("load", builtin_load, &env);
  create_builtin("while", builtin_while, &env);
  create_builtin("type-of", builtin_type_of, &env);
  create_builtin("type=", builtin_type_equal, &env);
  create_builtin("symbol=", builtin_symbol_equal, &env);
  create_builtin("string=", builtin_symbol_equal, &env);
  create_builtin("get-int", builtin_get_int, &env);

  create_builtin("list", builtin_list, &env);
  create_builtin("first", builtin_first, &env);
  create_builtin("last", builtin_last, &env);
  create_builtin("nth", builtin_nth, &env);
  create_builtin("pop", builtin_pop, &env);
  create_builtin("push", builtin_push, &env);
  create_builtin("append", builtin_append, &env);
  create_builtin("length", builtin_length, &env);
  create_builtin("empty?", builtin_empty_q, &env);
  create_builtin("~", builtin_string_concatenate, &env);

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

  load_file("native.lisp", &env);

  return env;
}
