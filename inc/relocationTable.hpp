#ifndef relocation_table_hpp
#define relocation_table_hpp

#include "elfTypes.hpp"
#include "symbolTable.hpp"
#include <vector>

using namespace std;

class RelatableEntry {

private:

  unsigned int offset; // ofset u generisanom kodu 
  int symbolRefNum; // number simbola u ts
  int addend; // u odnosu na pocetak sekcije kod lokalnih simbola, u tom slucaju simbol ref pokazuje na sekciju

  /*
  addend: -- offset
  kod lokalnih podataka (koji ne idu 
  u tabelu simbola) kao simbol se stavlja 
  sekcija u kojoj se nalaze
  addend je ofset od pocetka sekcije do tog simbola    
  */
  public: 
  unsigned int getOffset() { return offset; }
  int getSymbolRefNum() { return symbolRefNum; }
  int getAddend() { return addend; }
};

class RelocationTable {
public:

  vector<RelatableEntry*> relocationTable;

  void addRelaEntry(RelatableEntry*);
  RelatableEntry* getRelaEntry(int index);
};

#endif