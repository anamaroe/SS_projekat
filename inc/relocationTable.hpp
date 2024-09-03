#ifndef relocation_table_hpp
#define relocation_table_hpp

#include "elfTypes.hpp"
#include "symbolTable.hpp"
#include <vector>
#include <string>
using namespace std;



class RelatableEntry {

private:
  unsigned int offset;  // ofset u generisanom kodu 

  int symbolRefNum;     // number simbola u ts

  int addend;           // u odnosu na pocetak sekcije kod lokalnih simbola, u tom slucaju simbol ref pokazuje na sekciju

  RelocationType type;

  string symbolName;
 
public: 
  RelatableEntry(unsigned int offset, int symbolNumber, int addend, RelocationType type, string symName) : 
    offset(offset), symbolRefNum(symbolNumber), addend(addend), type(type), symbolName(symName) {}
  
  unsigned int getOffset();

  int getSymbolRefNum();

  int getAddend();

  RelocationType getType();

};



class RelocationTable {

public:
  int section;

  RelocationTable(int sectionNum) : section(sectionNum) {}
  
  vector<RelatableEntry*> relocationTable;
  
  void addRelaEntry(RelatableEntry*);

};

#endif