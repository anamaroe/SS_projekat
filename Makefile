ASSEMBLER_SRC = src/assembler.cpp\
	src/main_assembler.cpp\
	src/relocationTable.cpp\
	src/sectionTable.cpp\
	src/symbolTable.cpp\
	misc/lexer.cpp\
	misc/parser.cpp\

LINKER_SRC =

EMULATOR_SRC =

all: clean assembler linker emulator

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

clean: clean_ass clean_emu clean_link

clean_ass:
	rm -f misc/lexer.cpp 
	rm -f misc/parser.cpp 
	rm -f misc/lexer.hpp
	rm -f misc/parser.hpp
	rm -f asembler
	rm -f tests/*.txt

clean_emu:

clean_link: