all:  lexer.cpp parser.cpp symbol-table.cpp builtin_helpers.cpp builtin_logic.cpp builtin_math.cpp interp.cpp peasant-lisp.cpp
	g++ $? -o pl
clean:
	rm *.o
