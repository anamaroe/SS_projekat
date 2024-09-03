#include "../inc/sectionTable.hpp"

Section::Section(string n, int i) { 
    name = n;
    sectionTableIndex = i;
    code = new uint8_t[1024];  
    relocationTable = new RelocationTable(i);
} 

void Section::addInstruction(unsigned int instr, int locCnt) {
  // 4b of every instr; little endian
 
  for(int i = 0; i < 4; i++) {
    // upis najmanjeg bajta
    // 00 00 00 00:
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
    code[locCnt + i] = 0x00;
  }
}

BigLiteral* Section::findLiteralInTempPool(int literal) {
  for(auto i : temporaryPool) {
    // cout << "In temp pool: " << i->literal <<  dec << endl;
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

Section* SectionTable::getSection(string name) {
  for(Section* s : sectionTable) { 
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

int Section::iSectionFindStrIndex(string name) {
  int i = 0;
  for(string s : STR_TAB_Section_Names) {
    if(s == name) {
      return i;
    }
    else {
      i++;
    }
  }
  return -1;
}

int SectionTable::getNumProgbitsSections() {
  int num = 0;
  for(Section *s : sectionTable) {
    if(s->type == SHT_PROGBITS) num++;
  }
  return num;
}

int SectionTable::getNumRelaSections() {
  int num = 0;
  for(Section *s : sectionTable) {
    if(s->type == SHT_RELA) num++;
  }
  return num;
} 



/*****************************************************************************************/
//                    FILE W HELPERS
/*****************************************************************************************/

void Section::ELF_WriteSectionHeader(std::ofstream& outputFile) {
  // name : sh_name : sh_type : sh_offset : sh_size : sh_link : sh_info : sh_entsize
  string t;
  switch(type) {
    case SHT_PROGBITS: t = "SHT_PROGBITS"; break;
    case SHT_SYMTAB:   t = "SHT_SYMTAB";   break;
    case SHT_STRTAB:   t = "SHT_STRTAB";   break;
    case SHT_RELA:     t = "SHT_RELA"; 
  }
  outputFile  << setw(21) << left << name << " "
  << setw(7)  << strTableIndex    << " "
  << setw(18) << t                << " "
  << setw(9)  << elfFileOffset    << " "
  << setw(10) << length           << " "
  << setw(9)  << bestieSection    << " "
  << setw(9)  << info             << " "
  << setw(9)  << entrySize        << endl;
}

void Section::ELF_WriteSectionContent(std::ofstream& outputFile, SymbolTable* symbolTab) {
  outputFile << "Section " << name << endl;
  int i, size;
  switch(type) {
    case SHT_PROGBITS:
      for(int i = 0 ; i < length; i++) {
        // vec je smesteno kao little endian, samo kopiram
        // dodala sam static_cast
        outputFile << static_cast<int>(code[i]);
      }  
      outputFile << endl;

    break;

    case SHT_STRTAB:
      for(string s : STR_TAB_Section_Names) {
        outputFile << s << endl;
      }
      outputFile << endl;
    break;

    case SHT_SYMTAB:
      outputFile << "---------------------------------------------------------------------------------------------------------------------- " << endl;
      outputFile << "    Symbol    :    Number    :    Section    :    Value    :    IsGLobal    :   Defined    :    Size    :    StrInd    " << endl;
      outputFile << "---------------------------------------------------------------------------------------------------------------------- " << endl;
      size = symbolTab->symbolTable.size();
      i = 0;
      while(i < size) {
        outputFile << setw(22) << setfill(' ') << left << symbolTab->symbolTable[i]->symbolName << dec <<
          setw(15) << setfill(' ') << symbolTab->symbolTable[i]->getNumber()              << 
          setw(15) << setfill(' ') << left << symbolTab->symbolTable[i]->sectionNumber    << 
          setw(15) << setfill(' ') << left << symbolTab->symbolTable[i]->value            << 
          setw(16) << setfill(' ') << left << symbolTab->symbolTable[i]->isGlobal         <<  
          setw(15) << setfill(' ') << left << symbolTab->symbolTable[i]->defined          <<  
          setw(13) << setfill(' ') << left << symbolTab->symbolTable[i]->size             <<  
          setw(2)  << setfill(' ') << left << symbolTab->symbolTable[i]->stringTableIndex <<
          endl; i++;
      }
      outputFile << "---------------------------------------------------------------------------------------------------------------------- " << endl;
    break;

    case SHT_RELA:
      // nisam jos
      outputFile << "---------------------------------------------------------" << endl;
      outputFile << "    Offset    :    Number    :    Addend    :    Type    " << endl;
      outputFile << "---------------------------------------------------------" << endl;
      for(int i = 0 ; i < length; i = i + 16) { 
        outputFile << "    ";
        outputFile << +(code[i + 3]) << +(code[i + 2]) << +(code[i + 1]) <<
          std::setw(3) << std::setfill(' ') << +(code[i]) << "         ";
        outputFile << +(code[i + 7]) << +(code[i + 6]) << +(code[i + 5]) << 
          std::setw(2) << std::setfill(' ') << +(code[i + 4]) << "           ";
        outputFile << +(code[i + 11]) << +(code[i + 10]) << +(code[i + 9]) <<
          std::setw(2) << std::setfill(' ') << +(code[i + 8]) << "         ";
        outputFile << +(code[i + 15]) << +(code[i + 14]) << +(code[i + 13]) <<
          std::setw(2) << std::setfill(' ') << +(code[12]) << endl;

        //  cout << "AAA" << +code[12] << endl;
      }  
    break;
  }
  outputFile << endl << endl;
}



/*****************************************************************************************/
//                    COUT >> HELPERS
/*****************************************************************************************/

void SectionTable::writeSecTable() {
  cout << "---------------------------------------------" << endl;
  cout << "   Section  :  Index  :  Length  : str_tab_i " << endl;
  cout << "---------------------------------------------" << endl;
  int size = sectionTable.size();
  int i = 0;
  while(i < size) {
    cout << setw(17) << setfill(' ') << left << sectionTable[i]->name        <<
      setw(11) << setfill(' ') << left << sectionTable[i]->sectionTableIndex <<
      setw(10) << setfill(' ') << left << sectionTable[i]->length            << 
      setw(6)  << setfill(' ') << left << sectionTable[i]->strTableIndex     << endl;

      cout << "Temp Pool: " << endl;
      if(sectionTable[i]->temporaryPool.size() == 0) {
        cout << "Nothing in temp pool." << endl;
      }
      for (int j = 0; j < sectionTable[i]->temporaryPool.size(); j++) {
        cout << "literal: ";
        if(sectionTable[i]->temporaryPool[j]->sym == nullptr) {
          cout << " broj: " << hex << (sectionTable[i]->temporaryPool[j]->literal) << dec << endl;
        } else {
          cout << " sym: " << sectionTable[i]->temporaryPool[j]->sym->symbolName << endl;
        }
      }
      cout << endl;
      i++;
  }
  cout << "---------------------------------"<< endl;
}

void Section::writeRelaTable() {
  cout << "Relocations for section: " << this->name << endl;
  cout << "---------------------------------------" << endl;
  cout << " Symbol  :  Offset  :  Addend  :  Type " << endl;
  cout << "---------------------------------------" << endl;

  for(RelatableEntry *r : relocationTable->relocationTable) {
    cout << setw(11) << r->getSymbolRefNum() <<
      setw(8) << r->getOffset() <<
      setw(10) << r->getAddend() << 
      setw(10) << r->getType() << 
      endl;
  } 
  cout << " ----------------------------  "<< endl;
}
