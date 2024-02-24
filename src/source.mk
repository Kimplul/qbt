SRC_LOCAL != echo src/*.c
SOURCES += $(SRC_LOCAL) gen/gen_parser.c

gen/gen_parser.c: src/parser.y gen/gen_lexer.inc
	bison -Wcounterexamples -o gen/gen_parser.c src/parser.y

gen/gen_lexer.inc: src/lexer.l
	flex -o gen/gen_lexer.inc src/lexer.l
