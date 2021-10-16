OBJS = ./src/main.cpp ./src/assembler.cpp ./src/symbol_table.cpp ./src/section_table.cpp ./src/relocation_table.cpp

prog: $(OBJS)
	g++ -std=c++11 -g -gdwarf-2 $(OBJS) -o asembler

run:
	./asembler -o izlaz.o ulaz.s

test:
	./asembler -o test_1.o ./tests/test_1.s
	./asembler -o test_2.o ./tests/test_2.s
	./asembler -o test_3.o ./tests/test_3.s
	./asembler -o test_4.o ./tests/test_4.s
	./asembler -o test_5.o ./tests/test_5.s


clean:
	rm *^(\.cpp$|\.h$\.md$) asembler