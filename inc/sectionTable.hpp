#ifndef section_table_hpp
#define section_table_hpp

#include "elfTypes.hpp"
#include "relocationTable.hpp"
#include "symbolTable.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;


class Symbol;
class SectionTable;
class SymbolTable;
class RelocationTable;

/*
  struktura za privremeno pamcenje velikih literala
*/
struct BigLiteral {

    public:
    int locationCounter;        // u bazenu, popunjava se na kraju - ne treba?
    int literal = -1;           // sama vrednost, na 4B
    vector<int> fixupAdr;       // iz instrukcije
    Symbol* sym = nullptr;      // simbol ciju vresnot i dalje ne znamo pa ide ovde za svaki slucaj
    bool isSymbolValue = false; // fleg ako dodajem vr. simbola; zbog relokacionih zapisa u bazenu
    BigLiteral(int li) : literal(li) {}
    BigLiteral(Symbol* s) : sym(s) { isSymbolValue = true; }
};
 


class Section {

public:
  string name;

  int sectionTableIndex;
  
  unsigned int length;
  
  RelocationTable *relocationTable = nullptr;
  
  uint8_t* code = 0;      
  
  char* literalPool = 0;             // alocira se na kraju prolaza ako postoji potreba za tim 
  
  vector<BigLiteral*> temporaryPool; // popunjava se tokom prolaza 

  int poolSize = 0;                  // length + poolSize = ukupna velicina sekcije u bajtovima



  // stvari potrebne za elf (pored ovih gore)
  int strTableIndex = -1;             // indeks unutar tabele stringova -> za ime sekcije - postavljam nakon pravljenja string tabele
  
  SectionType type = SHT_PROGBITS;   // podrazumevano progbits, kod symtab, strtab i rela sekcija je drugacije (postavlja se kad se prave)
  
  int elfFileOffset;                 // pomeraj do sekcije od pocetka elf fajla - postavlja se tokom pisanja binarnog fajla za linker
  
  int entrySize = 0;                 // velicina ulaza kod sekcija symbol table i string table, inace 0
  
  int bestieSection = -1;            // index povezane sekcije; kod rela: symtab, kod symtab: strtab; Postavlja se pri pravljenju tih sekcija
  
  int info = 0;                      // kod rela: index sekcije za koju se pise; kod symtab: val + 1, val = vrednost poslednjeg lokalnog simbola

  // sadrzaj string tabele 
  vector<string> STR_TAB_Section_Names;



  // koliko bi trebalo da alociram za velicinu sekcije?
  Section(string n, int i);  

  BigLiteral* findLiteralInTempPool(int literal);

  BigLiteral* findLitSymbolInTempPool(Symbol* s);

  void addInstruction(unsigned int instr, int locCnt);

  void skip(int locCnt, int num);

  uint8_t* getSectionCode() { return code; }

  // ne znam jos
  void writeRelaTable();

  // ispis sekcije u fajl
  void ELF_WriteSectionHeader(std::ofstream&);

  // ispis sadrzaja sekcije u fajl
  void ELF_WriteSectionContent(std::ofstream&, SymbolTable*);

  int iSectionFindStrIndex(string); 
  
};



class SectionTable {

public:  
  vector<Section*> sectionTable;
  
  Section* getSection(string name);
  
  Section* getSection(int num);
  
  void add(Section* section) { sectionTable.push_back(section); }
  
  void writeSecTable();
  
  int tableSize() { return sectionTable.size(); }

  int getNumProgbitsSections();

  int getNumRelaSections();

};

#endif