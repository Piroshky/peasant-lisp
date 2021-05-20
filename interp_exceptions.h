struct returnException: public std::exception {
  Parse_Node *ret;
  const char * what () const throw () {
    return "";
  }
  returnException(Parse_Node *r) {
    ret = r;
  }
};

struct runtimeError: public std::exception {
  std::string err;
  const char * what () const throw () {
    return err.c_str();
  }
  runtimeError(std::string e) {
    err = e;
  }
};
