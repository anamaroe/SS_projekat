#include "../inc/assembler.hpp"

// ./asembler -o izlaz.o ulaz.s

Assembler* assembler = nullptr;

int main(int argc, char *argv[]) {

	if (argc != 4 || strncmp(argv[1], "-o", 2)) {
		cout << "Zadati argumenti nisu dobri." << endl;
		return -1;
	}

	assembler = new Assembler();

	int retVal = assembler->assemble(argv[3]);

	if (retVal == -1) {
		cout << "Greska prilikom asembliranja." << endl;
		return -1;
	}

	delete assembler;
}