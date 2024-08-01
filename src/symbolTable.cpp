#include "../inc/symbolTable.hpp"
#include <iostream>
#include <iomanip>
#include <string.h>
using namespace std;

int Symbol::counter = 0;

void SymbolTable::add(Symbol* symbol) {
  //cout << "Dodaje se: " << symbol->symbolName << endl;
  symbolTable.push_back(symbol);
  //writeSymTable();
} 

Symbol* SymbolTable::getSymbol(string name) {
  //cout << "Dohvatam simbol: " << name << endl;
  for(Symbol* s : symbolTable) {
    //cout << "u TS sam naisla na: " << s->symbolName << endl;
    if(s->symbolName == name) return s;
  }
  return nullptr;
}

Symbol* SymbolTable::getSymbol(int index) {
  if(index >= symbolTable.size() || index < 0) return nullptr;
  return symbolTable[index];
}

// T0 D0
int SymbolTable::backpatch() {
  // for(Symbol *s : symbolTable) {
  //   if(s->defined) {
  //     while(s->flink) {
  //       break;
  //     }
  //   }
  //   else continue;
  // } 
  return 0;
}
 
SymbolTable::~SymbolTable() {
  for(auto symbol : symbolTable) {
    delete symbol;
  }
}

/* ....................... preparaviti: mozda i dalje nije poznata vrednost simbola ....................... */
// idem kroz info listu simbola. upisujem val na lokacije u contentima sekcija
// void Symbol::resolveLabelReference(int locationCounter, SectionTable *sect) {
//   Section* section = nullptr;
//   while(flink) {
//     // idem kroz info listu i i pisem u contente sekcija...
//     section = sect->getSection(flink->sectionNum);
//     section->addInstruction(locationCounter, flink->address);
//     Backpatch* oldFlink = this->flink;
//     this->flink = this->flink->next;
//     delete oldFlink;
//   }
// }

void SymbolTable::writeSymTable() {
  cout << " ------------------------------------------------------------------------------------------------------- " << endl;
  cout << "     Symbol    :    Number    :    Section    :    Value    :    IsGLobal    :   Defined    :    Size    " << endl;
  cout << " ------------------------------------------------------------------------------------------------------- " << endl;
  int size = symbolTable.size();
  int i = 0;
  while(i < size) {
    cout << setw(22) << setfill(' ') << left << symbolTable[i]->symbolName << 
      setw(15) << setfill(' ') << symbolTable[i]->getNumber()              << 
      setw(12) << setfill(' ') << left << symbolTable[i]->sectionNumber    << 
      setw(18) << setfill(' ') << left << symbolTable[i]->value            << 
      setw(16) << setfill(' ') << left << symbolTable[i]->isGlobal         <<  
      setw(15) << setfill(' ') << left << symbolTable[i]->defined          <<  
      setw(16) << setfill(' ') << left << symbolTable[i]->size             << 
      endl;
      i++;
  }
  cout << " ------------------------------------------------------------------------------------------------------- " << endl;
}

int SymbolTable::tableSize() {
  int size = 0;
  for (Symbol *s : symbolTable) {
    size++;
  }
  return size;
}