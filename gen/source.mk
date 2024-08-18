gen/gen_parser.c: src/parser.y gen/gen_lexer.inc
	bison -Wcounterexamples -o gen/gen_parser.c src/parser.y

gen/gen_lexer.inc: src/lexer.l
	flex -o gen/gen_lexer.inc src/lexer.l

build/gen/gen_parser.o: gen/gen_parser.c
	mkdir -p build/gen
	$(COMPILE) $(SKIP_ANALYZER) -c gen/gen_parser.c -o build/gen/gen_parser.o
