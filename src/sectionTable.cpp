#include "../inc/sectionTable.hpp"
#include <iomanip>
using namespace std;

void Section::addInstruction(unsigned int instr, int locCnt) {
  // 4b of every instr; little endian
 
  for(int i = 0; i < 4; i++) {
    // upis najmanjeg bajta
    // 00 00 00 00
    // 0000 0000 0000 0000   0000 0000 0000 0000
    //                                 ---------
    //                                  najnizi
  
    unsigned int lastByte = (instr % 256) & 0x000000FF;
    instr = instr >> 8; 
    code[locCnt + i] = lastByte; 
  }  
}

void Section::skip(int locCnt, int num) {
  for(int i = 0; i < num; i++) {
    // (&code[locCnt])[i] = 0x00;
    code[locCnt + i] = 0x00;
  }
}

BigLiteral* Section::findLiteralInTempPool(int literal) {
  cout << "Searching for literal: " << literal << endl;
  for(auto i : temporaryPool) {
    cout << "In temp pool: " << i->literal << endl;
    if(i->literal == literal) return i;
  }
  return nullptr;
}

BigLiteral* Section::findLitSymbolInTempPool(Symbol* symbol) {
  if(symbol == nullptr) return nullptr;
  for (auto s : temporaryPool) {
    if(s->sym == nullptr) continue;
    if(s->sym->symbolName == symbol->symbolName) return s;
  }
  return nullptr;
}

void SectionTable::add(Section* section) {
  sectionTable.push_back(section);
}

Section* SectionTable::getSection(string name) {
  for(Section* s : sectionTable) { //cout << "naislli na sekciju: " << s->name << " name je " << name << endl;
    if(s->name == name) return s;
  }
  return nullptr;
}

Section* SectionTable::getSection(int num) {
  for(Section* s : sectionTable) {
    if(s->sectionTableIndex == num) return s;
  }
  return nullptr;
}

void SectionTable::writeSecTable() {
  cout << "---------------------------------" << endl;
  cout << "   Section  :  Index  :  Length  " << endl;
  cout << "---------------------------------" << endl;
  int size = sectionTable.size();
  int i = 0;
  while(i < size) {
    cout << setw(17) << setfill(' ') << left << sectionTable[i]->name <<
      setw(11) << setfill(' ') << left << sectionTable[i]->sectionTableIndex <<
      setw(10) << setfill(' ') << left << sectionTable[i]->length << 
      endl;

      cout << "Temp Pool: " << endl;
      if(sectionTable[i]->temporaryPool.size() == 0) {
        cout << "Nothing in temp pool." << endl;
      }
      for (int j = 0; j < sectionTable[i]->temporaryPool.size(); j++) {
        cout << "literal: ";
        if(sectionTable[i]->temporaryPool[j]->sym == nullptr) {
          cout << " broj: " << (sectionTable[i]->temporaryPool[j]->literal) << endl;
        } else {
          cout << " sym: " << sectionTable[i]->temporaryPool[j]->sym->symbolName << endl;
        }
      }
      i++;
  }
  cout << "---------------------------------"<< endl;
}

void Section::writeRelaTable() {
  cout << "-------------------------------" << endl;
  cout << " Symbol  :  Offset  :  Addend  " << endl;
  cout << "-------------------------------" << endl;
  int size = relocationTable->relocationTable.size();
  int i = 0;
  while(i < size) {
    cout << setw(11) << relocationTable->relocationTable[i]->getSymbolRefNum() <<
      setw(8) << relocationTable->relocationTable[i]->getOffset() <<
      setw(10) << relocationTable->relocationTable[i]->getAddend() << 
      endl;
      i++;
  }
  cout << " ----------------------------  "<< endl;
}

void Section::ELF_WriteSection(std::ofstream& outputFile) {
  // name : sh_name : sh_type : sh_offset : sh_size : sh_link : sh_info : sh_entsize
  string t;
  switch(type) {
    case SHT_PROGBITS: t = "SHT_PROGBITS"; break;
    case SHT_SYMTAB: t = "SHT_SYMTAB"; break;
    case SHT_STRTAB: t = "SHT_STRTAB"; break;
    case SHT_RELA: t = "SHT_RELA"; 
  }
  outputFile << setw(21) << left << name << " "
  << setw(7) << strTableIndex << " "
  << setw(18) << t << " "
  << setw(9) << elfFileOffset << " "
  << setw(10) << length << " "
  << setw(9) << bestieSection << " "
  << setw(9) << info << " "
  << setw(9) << entrySize << "\n";
}

void Section::ELF_WriteSectionContent(std::ofstream& outputFile, SymbolTable* symbolTab) {
  outputFile << "Section " << name << endl;
  int i, size;
  switch(type) {
    case SHT_PROGBITS:
      for(int i = 0 ; i < length; i++) {
        // vec je smesteno kao little endian, samo kopiram
        outputFile << code[i];
      } 
      outputFile << endl;
    break;

    case SHT_STRTAB:
      for(string s : STR_TAB_Section_Names) {
        outputFile << s << " ";
      }
      outputFile << endl;
    break;

    case SHT_SYMTAB:
      outputFile << "------------------------------------------------------------------------------------------------------- " << endl;
      outputFile << "    Symbol    :    Number    :    Section    :    Value    :    IsGLobal    :   Defined    :    Size    " << endl;
      outputFile << "------------------------------------------------------------------------------------------------------- " << endl;
      size = symbolTab->symbolTable.size();
      i = 0;
      while(i < size) {
        outputFile << setw(22) << setfill(' ') << left << symbolTab->symbolTable[i]->symbolName << 
          setw(15) << setfill(' ') << symbolTab->symbolTable[i]->getNumber()              << 
          setw(12) << setfill(' ') << left << symbolTab->symbolTable[i]->sectionNumber    << 
          setw(18) << setfill(' ') << left << symbolTab->symbolTable[i]->value            << 
          setw(16) << setfill(' ') << left << symbolTab->symbolTable[i]->isGlobal         <<  
          setw(15) << setfill(' ') << left << symbolTab->symbolTable[i]->defined          <<  
          setw(16) << setfill(' ') << left << symbolTab->symbolTable[i]->size             << 
          endl;
          i++;
      }
      outputFile << "------------------------------------------------------------------------------------------------------- " << endl;
    break;

    case SHT_RELA:
      // nisam jos
    break;
  }
  outputFile << endl;
}

void Section::fillStringTableContent(SectionTable* sectionTable) {
  // u STR_TAB_Section_Names pisem nazive sekcija
  for(Section *s : sectionTable->sectionTable) {
    STR_TAB_Section_Names.push_back(s->name);
  } 
}

int SectionTable::tableSize() {
  int size = 0;
  for(Section *s : sectionTable) {
    size++;
  }
  return size;
}