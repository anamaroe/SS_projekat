#include "../inc/symbolTable.hpp"


int Symbol::counter = 0;

Symbol* SymbolTable::getSymbol(string name) { 
  for(Symbol* s : symbolTable) { 
    if(s->symbolName == name) return s;
  }
  return nullptr;
}

Symbol* SymbolTable::getSymbol(int index) {
  if(index >= symbolTable.size() || index < 0) return nullptr;
  return symbolTable[index];
}

/*
  pravljenje relokacionih zapisa tipa ABS kod .word direktive;
  ili upis literalne vrednosti ako je poznata
*/
void SymbolTable::backpatchWordDirectiveAddresses(SectionTable* sectionTable) {
  for(Symbol* s : symbolTable) {
    for(Symbol::Backpatch *bp : s->flink) {
      unsigned short relaNumber;
      string symbolName;
      if(s->isGlobal) {
        relaNumber = s->number;
        symbolName = s->symbolName;
      } else {
        relaNumber = bp->sectionNum;
        symbolName = sectionTable->getSection(bp->sectionNum)->name;
      }
      // unsigned int offset, int symbolNumber, int addend, RelocationType type, string symName
      RelatableEntry *rela = new RelatableEntry(bp->address, relaNumber, 0, R_X86_64_32S, symbolName);
      sectionTable->getSection(bp->sectionNum)->relocationTable->addRelaEntry(rela);
      // if(!s->defined || s->isGlobal) { 
      //   // Napravi relokacioni zapis
      //   unsigned short relaNumber;
      //   string symbolName;
      //   if(s->isGlobal) {
      //     relaNumber = s->number;
      //     symbolName = s->symbolName;
      //   } else {
      //     relaNumber = bp->sectionNum;
      //     symbolName = sectionTable->getSection(bp->sectionNum)->name;
      //   }
      //   // unsigned int offset, int symbolNumber, int addend, RelocationType type, string symName
      //   RelatableEntry *rela = new RelatableEntry(bp->address, relaNumber, 0, R_X86_64_32S, symbolName);
      //   sectionTable->getSection(bp->sectionNum)->relocationTable->addRelaEntry(rela);
      // } else if(s->defined) {
      //   // upis u sadrzaj sekcije
      //   sectionTable->getSection(bp->sectionNum)->addInstruction(s->value, bp->address);
      //   cout << "Upisana vrednost za .word: " << s->value << " " << bp->address << endl;
      // } 
    }
  }
} 

void SymbolTable::writeSymTable() {
  cout << " ---------------------------------------------------------------------------------------------------------------------- " << endl;
  cout << "     Symbol    :    Number    :    Section    :    Value    :    IsGLobal    :   Defined    :    Size   :    str_ind    " << endl;
  cout << " ---------------------------------------------------------------------------------------------------------------------- " << endl;
  int size = symbolTable.size();
  int i = 0;
  while(i < size) {
    cout << setw(22) << setfill(' ') << left << symbolTable[i]->symbolName << 
      setw(15) << setfill(' ') << symbolTable[i]->getNumber()              << 
      setw(12) << setfill(' ') << left << symbolTable[i]->sectionNumber    << 
      setw(18) << setfill(' ') << left << symbolTable[i]->value            << 
      setw(16) << setfill(' ') << left << symbolTable[i]->isGlobal         <<  
      setw(15) << setfill(' ') << left << symbolTable[i]->defined          <<  
      setw(16) << setfill(' ') << left << hex << symbolTable[i]->size << dec << 
      setw(15) << setfill(' ') << left << symbolTable[i]->stringTableIndex << 
      endl;
      i++;
  }
  cout << " ------------------------------------------------------------------------------------------------------- " << endl;
}
 
SymbolTable::~SymbolTable() {
  for(auto symbol : symbolTable) {
    delete symbol;
  }
} 
 