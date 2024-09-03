ASSEMBLER_SRC = src/assembler.cpp\
	src/main_assembler.cpp\
	src/relocationTable.cpp\
	src/sectionTable.cpp\
	src/symbolTable.cpp\
	misc/lexer.cpp\
	misc/parser.cpp\

LINKER_SRC = src/linker.cpp\
	src/main_linker.cpp\

EMULATOR_SRC = src/emulator.cpp\
	src/main_emulator.cpp\

all: clean asembler linker emulator

asembler: flex
	g++ -Iinc -o ${@} ${ASSEMBLER_SRC}

linker:
	g++ -Iinc -o ${@} ${LINKER_SRC}

emulator:
	g++ -Inc -o ${@} ${EMULATOR_SRC}

bison:
	bison -d misc/parser.y
     
flex: bison
	flex misc/lex.l

clean: clean_ass clean_link clean_emu

clean_ass:
	rm -f misc/lexer.cpp 
	rm -f misc/parser.cpp 
	rm -f misc/lexer.hpp
	rm -f misc/parser.hpp
	rm -f asembler
	rm -f tests/*.txt
	rm -f tests/*.o

clean_link:
	rm -f linker
	rm -f tests/*.hex

clean_emu:
	rm -f emulator

dump: 
	hexdump -C tests/main.o > main.dump