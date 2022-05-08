UF-C.tab.c UF-C.tab.h: ./Parser-Bison/UF-C.y
	bison -r all --defines=./Parser-Bison/UF-C.tab.h --output=./Parser-Bison/UF-C.tab.c ./Parser-Bison/UF-C.y

lex.UF-C.c: ./Lexer-Flex/UF-C.l
	flex -o ./Lexer-Flex/lex.UF-C.c ./Lexer-Flex/UF-C.l

UF-C: lex.UF-C.c UF-C.tab.c
	gcc ./Parser-Bison/UF-C.tab.c ./Lexer-Flex/lex.UF-C.c ./Utils/AST.c -o UF-C