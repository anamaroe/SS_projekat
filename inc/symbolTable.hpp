#ifndef symbol_table_hpp
#define symbol_table_hpp

#include "elfTypes.hpp"
#include <vector>
#include <string>
#include "sectionTable.hpp"

using namespace std;

class SectionTable;

class Symbol {

public: 

  class Backpatch {
    public:
    unsigned int address; // adresa u contentu sekcije koja treba da bude ispravljena
    short sectionNum; 
    Backpatch(unsigned int a, short section) { this->address = a; sectionNum = section; }
  };

  string symbolName; 

  short sectionNumber; 

  unsigned short number;

  unsigned int value;  

  bool isGlobal;
  
  bool defined; 

  vector<Backpatch*> flink;

  //Backpatch* flink; // ovo je moja info lista za pamcenje svih koriscenja simbola, koristim samo za word direktivu...

  unsigned int size; // za sekcije, simboli -1

  static int counter;

 
  
  Symbol() {}

  Symbol(string name, short sectionNum, unsigned int value, bool isGlobal, bool defined, unsigned int size) {
      this->defined = defined;
      this->isGlobal = isGlobal;
      this->number = counter++;
      this->sectionNumber = sectionNum;
      this->size = size;
      this->symbolName = name;
      this->value = value;
  }

  void addBackpatchAddr(unsigned int address, short section) { 
    flink.push_back(new Backpatch(address, section));
    // if(!flink) {
    //   flink = new Backpatch(address, section);
    // } else {
    //   Backpatch* cur = flink;
    //   while(cur->next != nullptr) cur = cur->next;
    //   // sad pokazuje na zadnji
    //   cur->next = new Backpatch(address, section);
    // }
  }

  int getNumber() { return number; }
  
  void setSectionNumber(int num) { sectionNumber = num; }

  // da li je ovo uopste potrebno
  // void resolveLabelReference(int locationCounter, SectionTable *sect); // ovo je zapravo backpatch
  // idem kroz info listu simbola. upisujem val na lokacije u contentima sekcija

};

class SymbolTable {

public:

  vector<Symbol*> symbolTable;
  Symbol* getSymbol(string name);
  Symbol* getSymbol(int index);
  void add(Symbol*);
  int backpatch(); // TO DO?
  ~SymbolTable();
  void writeSymTable();
  int tableSize();

};


#endif