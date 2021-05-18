#include <iostream>
#include "interp.h"
#include "builtin_math.h"
#include "builtin_logic.h"

Parse_Node *eval_list(Parse_Node *node, Symbol_Table *env);
Parse_Node *eval_backtick(Parse_Node *node, Symbol_Table *env);
Parse_Node *eval_backtick_list(Parse_Node *node, Symbol_Table *env);
Parse_Node *expand_splice(Parse_Node *node, Symbol_Table *env);
Parse_Node *expand_eval_macro(Parse_Node *func, Parse_Node *node, Symbol_Table *env);
Parse_Node *apply_fun(Parse_Node *fun, Parse_Node *node, Symbol_Table *env);

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
    if (is_keyword(node)) {
      return node;
    }
    
    return env->lookup(node->token.name);
    break;
  }
  case PARSE_NODE_SYNTAX: {
    switch (node->subtype) {
    case SYNTAX_QUOTE: {
      return node->first;      
      break;
    }
    case SYNTAX_BACKTICK: {
      return eval_backtick(node->first, env);
      break;
    }
    case SYNTAX_COMMA: {
      fprintf(stderr, "Error: comma found not inside backtick\n");
      return nullptr;
      break;
    }
    case SYNTAX_COMMA_AT: {
      fprintf(stderr, "Error: ,@ found not inside backtick or unquoted list\n");
      return nullptr;
      break;
    }
    }

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

  node = expand_splice(node, env);

  Parse_Node *func_sym = node->first;
  if (func_sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: invalid function call, %s is not a symbol\n", func_sym->cprint());
    fprintf(stderr, "     in list: %s\n", node->cprint());
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
    return expand_eval_macro(func, node, env);
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

Parse_Node *expand_splice(Parse_Node *node, Symbol_Table *env) {
  Parse_Node *ret = node;
  Parse_Node *prev = nullptr;
  
  while (node->first != nullptr) {
    if (node->first->subtype == SYNTAX_COMMA_AT) {
      Parse_Node *elist = eval_parse_node(node->first->first, env);
      if (!is_list(elist)) {
	fprintf(stderr, "Error: ,@ can only be used with list, %s is not a list\n",
		node->first->first->cprint());
	return nullptr;
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
  return ret;
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
  Parse_Node *ret = node;
  node = expand_splice(node, env);
  while (node->first != nullptr) {
    node->first = eval_backtick(node->first, env);
    node = node->next;
  }
  
  return ret;
}

Parse_Node *expand_macro(Parse_Node *macro, Parse_Node *node, Symbol_Table *env) {
  Symbol_Table *macro_env = new Symbol_Table(env);
  
  // bind given arguments to symbols in macro-args
  Parse_Node *cur_param = macro->first;
  Parse_Node *cur_arg = node->next;
  while (cur_param->first != nullptr) {
    Parse_Node *param = cur_param->first;
    Parse_Node *arg = cur_arg->first;
    if (param->type != PARSE_NODE_SYMBOL) {
      fprintf(stderr, "Error: macro parameter %s is not a symbol\n", param->cprint());
      return nullptr;
    }

    if (param->token.name == "&rest") {
      cur_param = cur_param->next;
      param = cur_param->first;
      if (param == nullptr) {
	fprintf(stderr, "Error: variable required after &rest\n");
	return nullptr;
      }
      macro_env->table[param->token.name] = cur_arg;
      cur_param = cur_param->next;
      if (cur_param->first != nullptr) {
	fprintf(stderr, "Error: there should only be one parameter after &rest\n");
	return nullptr;
      }
      continue;
    } else {
      macro_env->table[param->token.name] = arg;
    }    
    cur_param = cur_param->next;
    cur_arg = cur_arg->next;
  }

  // eval the macro's defintion
  Parse_Node *cur = macro->next;
  Parse_Node *ret;
  while (cur->first != nullptr) {
    ret = eval_parse_node(cur->first, macro_env);
    cur = cur->next;
  }
  
  return ret;
}

Parse_Node *expand_eval_macro(Parse_Node *macro, Parse_Node *node, Symbol_Table *env) {
  Parse_Node *expand = expand_macro(macro, node, env);
  return eval_parse_node(expand, env);
}

Parse_Node *builtin_expand(Parse_Node *args, Symbol_Table *env) {
  if (args->first == nullptr) {
    fprintf(stderr, "Error: expand requires at least one argument\n");
    return nullptr;
  }

  Parse_Node *macro_sym = args->first;
  if (macro_sym->type != PARSE_NODE_SYMBOL) {
    fprintf(stderr, "Error: invalid expand call, %s is not a symbol\n", macro_sym->cprint());
    return nullptr;
  }

  Parse_Node *macro = env->lookup(macro_sym->token.name);
  if (macro == nullptr || macro->subtype != FUNCTION_MACRO) {
    fprintf(stderr, "Error: could not find macro named %s\n", macro_sym->cprint());
    return nullptr;
  }
  return expand_macro(macro, args, env);
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

  if(is_keyword(sym)) {
    fprintf(stderr, "keyword symbols cannot be reassigned\n");
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

Parse_Node *set_first(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 1) {
    fprintf(stderr, "Error: first takes exactly one argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if(is_keyword(earg)) {
    fprintf(stderr, "keyword symbols cannot be reassigned\n");
    return nullptr;
  }
  
  if (!is_list(earg) && !is_string(earg)) {
    fprintf(stderr, "Error: argument %s not a list\n", earg->cprint());
    return nullptr;
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
    fprintf(stderr, "Error: last takes exactly one argument\n");
    return nullptr;
  }

  Parse_Node *earg = eval_parse_node(args->first, env);
  if (!is_list(earg) && !is_string(earg)) {
    fprintf(stderr, "Error: argument %s not a sequence\n", earg->cprint());
  }

  if (earg->first == nullptr) {
    return earg;
  }

  if (is_string(earg)) {
    return earg;
  }

  Parse_Node *prev = earg->first;
  Parse_Node *cur = earg->next;
  while (cur->first != nullptr) {
    prev = cur;
    cur = cur->next;
  }

  return prev;
}

Parse_Node *builtin_last(Parse_Node *args, Symbol_Table *env) {
  Parse_Node *ret = set_last(args, env);
  // printf("builting got %s\n", ret->cprint());
  if (ret == nullptr || ret->first == nullptr) {
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
    fprintf(stderr, "Error: nth takes exactly two argument\n");
    return nullptr;
  }

  Parse_Node *n = eval_parse_node(args->first, env);
  if (n->subtype != LITERAL_INTEGER) {
    fprintf(stderr, "Error: argument %s not an integer\n", n->cprint());
  }
  
  Parse_Node *seq = eval_parse_node(args->next->first, env);
  if (!is_sequence(seq)) {
    fprintf(stderr, "Error: argument %s not a sequence\n", seq->cprint());
  }

  int nth = n->val.u64;

  if (seq->subtype == LITERAL_STRING) {
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

Parse_Node *builtin_append(Parse_Node *args, Symbol_Table *env) {
  if (args->length() != 2) {
    fprintf(stderr, "Error: append takes exactly two argument\n");
    return nullptr;
  }

  Parse_Node *list = eval_parse_node(args->next->first, env);
  Parse_Node *end = list;
  if (list->type != PARSE_NODE_LIST) {
    fprintf(stderr, "Error: first argument to append, %s, is not a list\n", list->cprint());
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
    fprintf(stderr, "set invalid accessor, %s is not a symbol\n", sym->cprint());
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
      fprintf(stderr, "Error: %s is not a string\n", earg->cprint());
      return nullptr;
    }
    load_file(earg->token.name, env);
    args = args->next;
  }
  return tru;
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

  create_builtin("list", builtin_list, &env);
  create_builtin("first", builtin_first, &env);
  create_builtin("last", builtin_last, &env);
  create_builtin("nth", builtin_nth, &env);
  create_builtin("pop", builtin_pop, &env);
  create_builtin("push", builtin_push, &env);
  create_builtin("append", builtin_append, &env);
  create_builtin("length", builtin_length, &env);
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
