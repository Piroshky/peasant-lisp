#include "builtin_math.h"
#include "builtin_logic.h"

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
