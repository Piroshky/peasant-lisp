all:  lexer.cpp parser.cpp symbol-table.cpp interp.cpp peasant-lisp.cpp
	g++ $? -o pl
clean:
	rm *.o
