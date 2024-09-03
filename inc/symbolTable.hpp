#ifndef symbol_table_hpp
#define symbol_table_hpp

#include "elfTypes.hpp"
#include <vector>
#include "sectionTable.hpp"
#include <iostream>
#include <iomanip>
#include <string>
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

  int number;

  unsigned int value;  

  bool isGlobal;
  
  bool defined; 

  vector<Backpatch*> flink; 

  unsigned int size; 

  int stringTableIndex = -1; // postavlja se pri pravljenju string tabele

  static int counter;

 
  
  Symbol() {}

  Symbol(string name, short sectionNum, unsigned int value, bool isGlobal, bool defined) {
      this->defined = defined;
      this->isGlobal = isGlobal;
      this->number = counter++;
      this->sectionNumber = sectionNum;
      this->size = 0;
      this->symbolName = name;
      this->value = value;
  }

  void addBackpatchAddr(unsigned int address, short section) { 
    flink.push_back(new Backpatch(address, section));
  }

  int getNumber() { return number; }
  
  void setSectionNumber(int num) { sectionNumber = num; }


};



class SymbolTable {

public:
  vector<Symbol*> symbolTable;

  Symbol* getSymbol(string name);
  
  Symbol* getSymbol(int index);
  
  void add(Symbol* symbol) { symbolTable.push_back(symbol); }
  
  void writeSymTable(); // da li ovo koristim?
  
  int tableSize() { return symbolTable.size(); }
  
  ~SymbolTable();
  
  /* ispravljam vrednosti simbola kod word direktive; pravim relokacioni zapis ako je potrebno */
  void backpatchWordDirectiveAddresses(SectionTable*);

  int getMaxNumber() { return symbolTable.back()->getNumber(); }

};


#endif