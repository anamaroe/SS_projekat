#include "../inc/linker.hpp"

Linker::Linker(bool hex, string outFile, vector<string> inFiles, vector<PlaceSection*> sectionPlaces) {
  this->hasHex = hex;
  this->outputFileName = outFile;
  this->inputFiles = inFiles;
  this->sectionPlaces = sectionPlaces;
}

Linker::~Linker() {
  for(PlaceSection * ps : sectionPlaces) {
    for(LinkerSection *ls : ps->sectionsWSameName) {
      for(LinkerRelocation * lr : ls->relocations) {
        delete lr;
      }
      delete ls;
    }
    delete ps;
  }
  for(LinkerInputFile * f : files) {
    for(LinkerSymbol *ls : f->symbolTable->symbolTable) {
      delete ls;
    }
    delete f;
  }
} 
  
int Linker::link() {
  using LinkerMethod = int (Linker::*)();

  LinkerMethod steps[] = {
      &Linker::readInputFiles,
      &Linker::mapSections,
      &Linker::checkForUndefAndMultipleDefs,
      &Linker::resolveSymbols, 
      &Linker::resolveRelocations,
      &Linker::writeLinkerOutputFiles
  };
  
  for (auto step : steps) {
      int ret = (this->*step)();
      if (ret == -1) return ret;
  }
  
  return 0;
}



/*****************************************************************************************/
//                    LINKER METHODS 
/*****************************************************************************************/


int Linker::readInputFiles() {
  string file = "tests/";
  for(string s : inputFiles) {  
    ifstream inFile(file + s, std::ios::binary);
    if (!inFile) {
        cout << "Can't open file in linker: " << file + s << endl;
        return 1;
    }
    LinkerInputFile *linkerFile = new LinkerInputFile(file + s);
    files.push_back(linkerFile);

    int symTabLen, progSectionsCnt, relocsCnt;
    inFile.read(reinterpret_cast<char*>(&symTabLen), sizeof(uint32_t));
    inFile.read(reinterpret_cast<char*>(&progSectionsCnt), sizeof(uint32_t));
    inFile.read(reinterpret_cast<char*>(&relocsCnt), sizeof(uint32_t));

    // cout << symTabLen << " " << progSectionsCnt << " " << relocsCnt << endl;
 
    for(int i = 0; i < symTabLen; i++) { 
      int num, strIndex, global, defined, nameLength;
      short sectionNum;
      unsigned int value; 
      string symbolName;

      inFile.read(reinterpret_cast<char*>(&num), sizeof(int));
      inFile.read(reinterpret_cast<char*>(&strIndex), sizeof(int));
      inFile.read(reinterpret_cast<char*>(&sectionNum), sizeof(short));
      inFile.read(reinterpret_cast<char*>(&global), sizeof(int));
      inFile.read(reinterpret_cast<char*>(&defined), sizeof(int));
      inFile.read(reinterpret_cast<char*>(&value), sizeof(unsigned int));
      inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(int));
      symbolName.resize(nameLength);
      inFile.read(&symbolName[0], nameLength); 

      // napravi simbol, dodaj u sym tab tog linkerovog fajla
      LinkerSymbol *ls = new LinkerSymbol(num, strIndex, sectionNum, global, defined, value, symbolName);
      if(num == sectionNum) ls->type = LinkerSymbol::SECTION;
      else ls->type = LinkerSymbol::SYMBOL;
      linkerFile->symbolTable->symbolTable.push_back(ls);
    }

    for(int i = 0; i < progSectionsCnt; i++) {
      int num, len, stringLen;
      uint8_t* code;
      string name;

      inFile.read(reinterpret_cast<char*>(&num), sizeof(int));  
      inFile.read(reinterpret_cast<char*>(&len), sizeof(unsigned int));  
      code = new uint8_t[len];
      inFile.read(reinterpret_cast<char*>(&code[0]), len);
      inFile.read(reinterpret_cast<char*>(&stringLen), sizeof(int));
      name.resize(stringLen);
      inFile.read(&name[0], stringLen); 
 
      LinkerSection *ls = new LinkerSection(len, name, LinkerSection::PROGBITS);  
      ls->code = code; 
      ls->num = num; 
      linkerFile->sectionTable->sectionTable.push_back(ls); 
    } 

    for(int i = 0; i < relocsCnt; i++) {
      int num, len, stringLen;
      string name;
      int offset, symNum, addend;
      RelocationType type;

      // code: offset, symNum, addend, relaType - sve po int
      inFile.read(reinterpret_cast<char*>(&num), sizeof(int)); 
      inFile.read(reinterpret_cast<char*>(&len), sizeof(unsigned int));

      LinkerSection *ls = new LinkerSection(len, name, LinkerSection::RELA);  
 
      int relocations = len / (sizeof(int) * 4);
      for(int j = 0; j < relocations; j++) {
        inFile.read(reinterpret_cast<char*>(&offset), sizeof(int));  
        inFile.read(reinterpret_cast<char*>(&symNum), sizeof(int));  
        inFile.read(reinterpret_cast<char*>(&addend), sizeof(int));  
        inFile.read(reinterpret_cast<char*>(&type), sizeof(int)); 

        RelocationType t;
        switch(type) {
          case 0 : t = R_X86_64_32S; break;
          case 1 : t = R_X86_64_12S; break;
          case 2 : t = R_X86_64_PC32; break;
        }
        LinkerRelocation *reloc = new LinkerRelocation(offset, addend, symNum, t);

        string symbolName;
        bool nameSet = false;
        for(LinkerSymbol * sym : linkerFile->symbolTable->symbolTable) {
          if(sym->num == symNum) {
            symbolName = sym->name;
            nameSet = true;
          }
        }
        if(!nameSet) {
          for(LinkerSection * s : linkerFile->sectionTable->sectionTable) {
            if(s->num == symNum) {
              symbolName = s->name;
            }
          }
        }
        reloc->symbolName = symbolName;
        // cout << "PROCITANA RELOKACIJA: offset: " << dec << offset<< " br simbola " <<
        //    symNum  << dec << " " << addend << " " << type << endl;
        ls->relocations.push_back(reloc);  
      }

      inFile.read(reinterpret_cast<char*>(&stringLen), sizeof(int)); 
      name.resize(stringLen);
      inFile.read(&name[0], stringLen);  
      ls->name = name;

      string parentSectionName = name.erase(0, 6); 
      LinkerSection *parentSection = linkerFile->sectionTable->getSectionByName(parentSectionName);
      parentSection->relaSection = ls;  
    }
  }
  return 0;
}


int Linker::mapSections() {

  // zadate sekcije
  vector<PlaceSection*> givenSections = sectionPlaces;

  // ubacim ostale u sectionPlaces
  for(LinkerInputFile *lf : files) {
    for(LinkerSection *ls : lf->sectionTable->sectionTable) {

      if(ls->type != LinkerSection::PROGBITS) continue;

      PlaceSection * ps = getPlacedSection(ls->name);

      if(!ps) {
        ps = new PlaceSection(0, ls->name);
        sectionPlaces.push_back(ps);
      }
      ps->length += ls->length;  
      ps->sectionsWSameName.push_back(ls);  // svaki place section ima vektor sekcija tog istog imena
    } 
  }
  // sad su sve sekcije u mapi, sa svojim duzinama
  // proveravam da li se zadate preklapaju:
  for(PlaceSection* gs : givenSections) {
    PlaceSection *ps = getPlacedSection(gs->section);
    gs->length = ps->length; // tu je vec ukupna duzina
  }

  for(int i = 0; i < givenSections.size(); i++) {
    for(int j = 1; j < givenSections.size(); j++) {
      if(j == i) break;
      if(((givenSections[i]->place <= givenSections[j]->place + givenSections[j]->length) 
        && (givenSections[j]->place <= givenSections[i]->place))
        || ((givenSections[j]->place >= givenSections[i]->place + givenSections[i]->length)
        && (givenSections[j]->place <= givenSections[i]->place))) {  
          cout << "Specified sections are overlaping! Sections " << givenSections[i]->section << " and " <<
            givenSections[j]->section << endl;
          return -1;
      }  
    }
  }
  unsigned int lastAddress = 0x00000000;
  PlaceSection *lastPlacedSection;

  for(PlaceSection* ps : givenSections) {
    if(ps->place > lastAddress) {
      lastAddress = ps->place;
      lastPlacedSection = ps;
    }
  }

  // sekcije se smestaju nakon poslednje navedene
  for(PlaceSection* ps : sectionPlaces) {
    if(ps->place == 0) {
      // place it
      if(lastPlacedSection == nullptr) {
        ps->place = lastAddress;
      } else {
        ps->place = lastAddress + lastPlacedSection->length;
      }
      lastAddress = ps->place;
      lastPlacedSection = ps;
    }
  }
  for(PlaceSection* ps : sectionPlaces) { 
  // svaka PlaceSection ima vektor LinkerSection-a, koje se izto zovu.
  // treba svakoj toj da namestim base, uzimajuci u obzir PlaceSection->place i section->length

    int startBaseAddress = ps->place;

    for(LinkerSection* lis : ps->sectionsWSameName) {
      lis->base = startBaseAddress;
      startBaseAddress += lis->length;
    }

  }
  return 0;
}


int Linker::checkForUndefAndMultipleDefs() {
  for(LinkerInputFile * linkerFile1 : files) {
    for(LinkerSymbol *symbol1 : linkerFile1->symbolTable->symbolTable) {
      bool exists = true;
      if(!symbol1->defined && symbol1->global) {
        exists = false;
      }
      for(LinkerInputFile * linkerFile2 : files) {
        if(linkerFile1->filename == linkerFile2->filename) continue;
        for(LinkerSymbol *symbol2 : linkerFile2->symbolTable->symbolTable) {

          if(symbol1->name == symbol2->name && symbol1->defined && symbol1->global && symbol2->defined && symbol2->global) {
            cout << "Error: Multiple definitions of a symbol " << symbol1->name << endl;
            return -1;
          }
          if(symbol1->name == symbol2->name && (
           symbol1->defined && symbol1->global && !symbol2->defined && symbol2->global ||
          !symbol1->defined && symbol1->global &&  symbol2->defined && symbol2->global
          )
          ) {
            //cout << "tu sam";
            exists = true;
            break;
          }
        }
      }
      if(!exists) {
        cout << "Error: Undefined symbol: " << symbol1->name << endl;
        return -1;
      }
    }
  }
  return 0;
}
  

int Linker::resolveSymbols() {
  for(LinkerInputFile * lif : files) {
    for(LinkerSymbol * ls : lif->symbolTable->symbolTable) {

      if(ls->global && !ls->defined) { continue; }
      unsigned int newSymbolValue = ls->value; 

      unsigned int old = newSymbolValue;
      LinkerSection * sect = nullptr;
      
      // base sekcije dje je definisan + stara val
      // trazim sekciju sa tim num-om (sekciju iz tog fajla)

      for(LinkerSection *sec : lif->sectionTable->sectionTable) {
        if(sec->num == ls->section) {
          newSymbolValue += sec->base;   sect = sec;
          break;
        }
      } 
      ls->value = newSymbolValue;

      // cout << "stara vr simbola " << ls->name << " iz sekcije " << sect->name << " " << old << hex << "   nova vrednost simbola " << newSymbolValue << endl;
      
    }
  }
  for(LinkerInputFile *lif1 : files) {
    for(LinkerSymbol * symExtern : lif1->symbolTable->symbolTable) {
      if(symExtern->defined || !symExtern->global) { continue; } // jer mi sad trebaju samo extern simboli

      for(LinkerInputFile *lif2 : files) {
        if(lif2->filename == lif1->filename) { continue; }
        for(LinkerSymbol *defSym : lif2->symbolTable->symbolTable) {
          
          if(defSym->global && defSym->name == symExtern->name) {
            symExtern->value = defSym->value;

            // cout << "fajl: " << lif1->filename << "  razresena vrednost extern simbola: " << symExtern->name
            // << " val: " << symExtern->value << " iz fajla " << lif2->filename << endl; 
          }
        }
      }
    }
  }
  return 0;
}
  

int Linker::resolveRelocations() {
  for(LinkerInputFile *lif : files) {
    for(LinkerSection *ls : lif->sectionTable->sectionTable) {
      LinkerSection* relaSection = ls->relaSection;
      if(relaSection != nullptr) {
        for(LinkerRelocation *rela : relaSection->relocations) {
          useRelocation(rela, ls, lif->symbolTable);
        }
      } 
    }
  }
  return 0;
}


int Linker::writeLinkerOutputFiles() {
  // txt fajl
  ofstream outputFile("tests/" + outputFileName + ".txt"); 
  // binarni fajl:
  ofstream binaryFile("tests/" + outputFileName); 

  if (outputFile.is_open() && binaryFile.is_open()) {  

    outputFile << "Hex dump for " << outputFileName << endl
    << "----------------------------------" << endl
    << "Address :                            " << endl
    << "----------------------------------" << endl;

    unsigned int startAddress = -1;
    LinkerSection *currSectionToPrint = nullptr;
    vector<LinkerSection*> sectionsToPrint;

    for(LinkerInputFile *lif : files) {
      for(LinkerSection *ls : lif->sectionTable->sectionTable) {
        sectionsToPrint.push_back(ls);
      }
    }

    while(!sectionsToPrint.empty()) {
      startAddress = -1;
      for(LinkerSection *ls : sectionsToPrint) {
        if(startAddress == -1 || startAddress > ls->base) {
          startAddress = ls->base;
          currSectionToPrint = ls;
        } 
      }
      // za txt fajl
      for (int i = 0; i < currSectionToPrint->length; i += 8) {
        outputFile << hex << setw(8) << setfill('0') << currSectionToPrint->base + i << ":  ";
        for (int j = 0; j < 8; ++j) {
          if (i + j < currSectionToPrint->length) {
            outputFile << hex << setw(2) << setfill('0')  << +(currSectionToPrint->code[i + j]) << " ";
          } else {
            outputFile << "   ";
          }
        }
        outputFile << endl; 
      }

      binaryFile.write(reinterpret_cast<const char *>(&currSectionToPrint->length), sizeof(int));
      
      // pri citanju iz binarnog fajla proveravam koliko ima upisanog sadrzaja...
      for (int i = 0; i < currSectionToPrint->length; i += 8) {
        unsigned int address = currSectionToPrint->base + i;
        // upisujem adresu kao int:
        binaryFile.write(reinterpret_cast<const char *>(&address), sizeof(address));

        // upis sadrzaja sekcije: (PO 2 INSTRUKCIJE)
        int dataSize = (i + 8 <= currSectionToPrint->length) ? 8 : (currSectionToPrint->length - i);
        binaryFile.write(reinterpret_cast<const char*>(currSectionToPrint->code + i), dataSize);
      }
      
      for (auto it = sectionsToPrint.begin(); it != sectionsToPrint.end(); ++it) {
        if ((*it)->base == startAddress) {
          sectionsToPrint.erase(it);
          break; 
        }
      }
    }
    binaryFile.close(); 
    outputFile.close();   
  } else {
    cout << "Failed to create linker output files." << endl;   
  }     
  return 0;
}



/*****************************************************************************************/
//                    LINKER UTIL 
/*****************************************************************************************/

void Linker::useRelocation(Linker::LinkerRelocation *rela, Linker::LinkerSection *ls, Linker::LinkerSymbolTable *table) {
  // cout << endl << "razresavam relokaciju: " << rela->symbolName << " " << dec << rela->offset << hex << 
  // " iz sekcije: " << ls->name << endl;
  
  LinkerSymbol* symbol = nullptr;
  unsigned int valueToWriteIn; // addend + val.sym | val.sekcije
  bool isGLobalSymbol = false;

  // u linkeru u tabeli simbola nemam sekcije...
  for(LinkerSymbol * s : table->symbolTable) { 
    if(rela->symbolName == s->name) {
      symbol = s;
      // cout << "radi se o globalnom simbolu " << s->name << endl;
      isGLobalSymbol = true;
      valueToWriteIn = s->value + rela->addend;
      break;
    }
  }
  if(symbol == nullptr) {
    valueToWriteIn = ls->base + rela->addend;
  }
  // ako je simbol sekcija, uzimam njen base, sabiram sa addendom
  // ako je simbol simbol, uzimam njegovu vrednost i sabiram sa addendom

  switch(rela->type) {
    case R_X86_64_12S:
      // ovi su na najnizih 12 bita...
      // cout << "R_X86_64_32S:" <<endl;
      ls->code[rela->offset] = valueToWriteIn & 0xFF;
      ls->code[rela->offset + 1] = (ls->code[rela->offset + 1] & 0xF0) | ((valueToWriteIn >> 8) & 0x0F);
      break;

    case R_X86_64_PC32:
    // cout << "PC RELA: " << valueToWriteIn << " " << ls->base << " " << rela->offset << " valueToWriteIn - ls->base - rela->offset "
    // << valueToWriteIn - ls->base - rela->offset << endl;
      // ovi su na najnizih 12 bita...  
      // cout << "R_X86_64_PC32:" <<endl;
      valueToWriteIn = valueToWriteIn - ls->base; // - rela->offset;
      ls->code[rela->offset] = valueToWriteIn & 0xFF;
      ls->code[rela->offset + 1] = (ls->code[rela->offset + 1] & 0xF0) | ((valueToWriteIn >> 8) & 0x0F);
      break;

    case R_X86_64_32S:
      // na 32 bita...
      // cout << "R_X86_64_32S:" <<endl;
      uint8_t val1 = (valueToWriteIn & 0xFF);
      uint8_t val2 = ((valueToWriteIn >> 8) & 0xFF);
      uint8_t val3 = ((valueToWriteIn >> 16) & 0xFF);
      uint8_t val4 = ((valueToWriteIn >> 24) & 0xFF);
      ls->code[rela->offset] = val1;
      ls->code[rela->offset + 1] = val2;
      ls->code[rela->offset + 2] = val3;
      ls->code[rela->offset + 3] = val4;
      break;
  }
  // cout << "vrednost koju upisujem: " << valueToWriteIn << " tip relok : " << rela->type << endl;
}


Linker::LinkerSection* Linker::LinkerSectionTable::getSectionByName(string name) {
  for(Linker::LinkerSection *l : sectionTable) {
    if(l->name == name) return l;
  }
  return nullptr;
}


Linker::PlaceSection* Linker::getPlacedSection(string name) {
  for(PlaceSection* ps : sectionPlaces) {
    if(ps->section == name) {
      return ps;
    }
  }
  return nullptr;
}
 