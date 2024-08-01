#ifndef section_table_hpp
#define section_table_hpp

#include "elfTypes.hpp"
#include "relocationTable.hpp"
#include "symbolTable.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

class RelocationTable;
class Symbol;
class SectionTable;
class SymbolTable;

struct BigLiteral {
    public:
    int locationCounter;    // u bazenu, popunjava se na kraju
    int literal = -1;       // sama vrednost, na 4B
    vector<int> fixupAdr;   // iz instrukcije
    Symbol* sym = nullptr;  // simbol ciju vresnot i dalje ne znamo pa ide ovde za svaki slucaj
    BigLiteral(int li) : literal(li) {}
    BigLiteral(Symbol* s) : sym(s) {}
  };
 
class Section {
  public:

  // basic polja
  string name;
  int sectionTableIndex;
  unsigned int length;
  RelocationTable *relocationTable = nullptr;
  char* code = 0;       // sadrzaj! - sadrzaj STRing tabele je u posebnom polju
  char* literalPool = nullptr;  // alocira se na kraju prolaza ako postoji potreba za tim 
  vector<BigLiteral*> temporaryPool; /* popunjava se tokom prolaza */

  // stvari potrebne za elf (pored ovih gore)
  int strTableIndex; // indeks unutar tabele stringova -> za ime sekcije - postavljam nakon pravljenja string tabele
  SectionType type = SHT_PROGBITS; // podrazumevano progbits, kod symtab, strtab i rela sekcija je drugacije (postavlja se kad se prave)
  int elfFileOffset; // pomeraj do sekcije od pocetka elf fajla - postavlja se tokom pisanja binarnog fajla za linker
  int entrySize = 0; // velicina ulaza kod sekcija symbol table i string table, inace 0
  int bestieSection = -1; // index povezane sekcije; kod rela: symtab, kod symtab: strtab; Postavlja se pri pravljenju tih sekcija
  int info = 0; // kod rela: index sekcije za koju se pise; kod symtab: val + 1, val = vrednost poslednjeg lokalnog simbola

  // sadrzaj string tabele cuvam kao niz stringova
  vector<string> STR_TAB_Section_Names;

  // metode
  BigLiteral* findLiteralInTempPool(int literal);
  BigLiteral* findLitSymbolInTempPool(Symbol* s);

  Section(string n, int i) : name(n), sectionTableIndex(i) {
    code = new char[512]; // (char*)malloc(sizeof(int)*600);
  } 

  void addInstruction(unsigned int instr, int locCnt);
  void skip(int locCnt, int num);

  // ne znam jos
  void writeRelaTable();

  // for writing into pretty ELF file
  void ELF_WriteSection(std::ofstream&);
  void ELF_WriteSectionContent(std::ofstream&, SymbolTable*);

  // create string table content
  void fillStringTableContent(SectionTable*);

  char* getSectionCode() { return code; }
  
};

class SectionTable {
  public:  
  vector<Section*> sectionTable;
  Section* getSection(string name);
  Section* getSection(int num);
  void add(Section*);
  void writeSecTable();
  int tableSize();

};

#endif