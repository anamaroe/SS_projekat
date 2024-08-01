#include "../inc/assembler.hpp"
#include <iomanip>

Assembler::Assembler() {
  locationCounter = 0;
  currSection = -1;
  currSectionName = "";
  endFound = false;
  symbolTable = new SymbolTable();
  sectionTable = new SectionTable();
}

Assembler::~Assembler() {
  delete symbolTable;
  delete sectionTable;
}

// to do
int Assembler::assemble(string arg) {

  cout << "ime fajla za citanje: "+arg;
  FILE *file = fopen(arg.c_str(), "r");
  
  if(file == nullptr) {
    cout << "file je nullptr";
    return -1;
  }

  yyin = file;
  int parser = yyparse();
  if(parser != 0) {
    return -1;
  }
  fclose(file);

  

  // prodji kroz backpatch liste

  // pravi bazene
  
  // ovde ce biti jos nesto

  // ... popunjavanje tabele relokacija



  // pravim sekciju za tabelu simbola i za tabelu stringova

  Section *SYM_TAB = new Section("SYM_TAB", currSection + 1);
  SYM_TAB->length = sizeof(Symbol) * symbolTable->tableSize();
  SYM_TAB->type = SHT_SYMTAB;
  // elfFILEOffset POSLE
  SYM_TAB->entrySize = sizeof(Symbol);
  if(symbolTable->tableSize() > 0) {
    SYM_TAB->info = symbolTable->symbolTable.back()->getNumber();
  } else {
    SYM_TAB->info = 0;
  }
  sectionTable->add(SYM_TAB);

  Section *STR_TAB = new Section("STR_TAB", currSection + 2);
  STR_TAB->fillStringTableContent(sectionTable);
  STR_TAB->length = (sectionTable->tableSize() + 1) * sizeof(string); 
  STR_TAB->strTableIndex = -1; // samo kod ove, string sekcije je tako
  STR_TAB->type = SHT_STRTAB;
  // elf offset
  STR_TAB->entrySize = sizeof(string);

  // dodala str table indexa svim sekcijama
  int currStringTableIndex = 0;
  for(Section *s : sectionTable->sectionTable) {
    s->strTableIndex = currStringTableIndex++;
  }
  sectionTable->add(STR_TAB);
  
  SYM_TAB->bestieSection = STR_TAB->sectionTableIndex;
 
  // ovo treba skloniti kasnije
  writeSymbolTable();
  writeSectionTable();


  // proba: pisanje lepog fajla, pisem sekcije

  string fileName;
  int pos = arg.find('.');
  if(pos != std::string::npos) {
    fileName = arg.substr(0, pos) + ".txt";
  } else {
    // error
  } 

  std::ofstream outputFile(fileName); 

  if (outputFile.is_open()) {    
    outputFile << "ELF header:\n"; 
    outputFile << "------------------------------------------------------------------------------------------------- " << endl;
    outputFile << "Magic: 7f 45 4c 46 02 01 01 00 00 00 00 00 00 00\n";
    outputFile << "Class:                         ELF64\n";
    outputFile << "Data:                          2's complement, little endian\n";
    outputFile << "Version:                       Current version\n";
    outputFile << "OS/ABI:                        UNIX System V ABI\n";
    outputFile << "ABI version:                   0\n";
    outputFile << "Type:                          1  - Relocatable file\n";
    outputFile << "Machine:                       62 - AMD x86-64 architecture\n";
    outputFile << "Entry point address:           0x40000000\n";
    outputFile << "Program Header Table:          0\n"; 
    outputFile << "Section Header Table:          \n"; // TO DO - offset  -- mozda je bolje da ovaj ofset bude do sht u binarnom fajlu
    outputFile << "Flags:                         0\n";
    outputFile << "ELF header size:               0x0040\n";  
    outputFile << "PHT entry size:                0\n";  
    outputFile << "PHT entries count:             0\n"; 
    outputFile << "SHT entry size:                " << sizeof(Section) << "\n";
    outputFile << "SHT entries count:             " << sectionTable->tableSize() << "\n";  
    outputFile << "String table index:            " << STR_TAB->sectionTableIndex << "\n\n";   
 
    outputFile << "Section Header Table:  \n";
    outputFile << "------------------------------------------------------------------------------------------------- " << endl;
    outputFile << setw(9) << "name" << setw(18) << "sh_name"
              << setw(12) << "sh_type" << setw(15) << "sh_offset"
              << setw(10) << "sh_size" << setw(10) << "sh_link"
              << setw(10) << "sh_info" << setw(13) << "sh_entsize\n";
    outputFile << "------------------------------------------------------------------------------------------------- " << endl;
    // sekcije ukratko, lep ispis
    for(Section *s : this->sectionTable->sectionTable) {
      s->ELF_WriteSection(outputFile);
    } 
    outputFile << "------------------------------------------------------------------------------------------------- " << endl;
    // ispis sadrzaja sekcija
    outputFile << endl;
    for(Section *s : this->sectionTable->sectionTable) {
      s->ELF_WriteSectionContent(outputFile, symbolTable);
    }
 
    outputFile.close();  

    std::cout << "Text has been written to the file." << std::endl;  
  } else {
    std::cout << "Failed to create the file." << std::endl;   
  } 

  return 0;

}

bool Assembler::isLiteralBig(int num) {
  if(num > ((1<<11) - 1) || num < - (1 << 11)) {  
    return true;
  }
  return false;
}

// to do
void Assembler::writeRelocTables() {
  int size = sectionTable->sectionTable.size(), i = 0;
  while(i++ < size) {
    sectionTable->sectionTable[i]->writeRelaTable();
  }
}

void Assembler::writeSymbolTable() {
  symbolTable->writeSymTable();
}

void Assembler::writeSectionTable() {
  sectionTable->writeSecTable();
}

// to do
void Assembler::writeToFile(char *) {

}


/*****************************************************************************************/
//                    LABEL
/*****************************************************************************************/

/*
  gotovo - mozda jos samo resolve label reference
*/
void Assembler::processLabelDefinition(char *labelName) { 
  if (currSection == -1) {
    cout << "Label outside any section!" << endl;
    return;
  }
  Symbol* sym = symbolTable->getSymbol(labelName);
  
  if(sym == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    Symbol* symb = new Symbol(labelName, currSection, locationCounter, false, true, -1);
    symbolTable->add(symb);
    return;
  }
  if(sym->symbolName == (string)labelName) {  
    if(sym->defined == true) {
      cout << "Multiple definition of a symbol! " << labelName << endl;
      return;
    } else {
      // naisli smo pre na simbol ali nismo znali sta je, sad ga definisemo
      sym->defined = true;
      sym->value = locationCounter;
      sym->sectionNumber = currSection;
      // sym->resolveLabelReference(locationCounter, sectionTable); /* jos ovo treba ispitati */
      return;
    }
  }
}

/*****************************************************************************************/
//                    DIRECTIVES
/*****************************************************************************************/

/*
  gotovo
*/
void Assembler::processGlobal(char* sym) {
  Symbol* s = symbolTable->getSymbol(sym);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    Symbol* symb = new Symbol(sym, currSection, 0, true, false, -1);
    symbolTable->add(symb);
    return;
  } else { // ovde vrv nikad nece uci
    s->isGlobal = true;
    return;
  }
}

/*
  gotovo
*/
void Assembler::processExtern(char* sym) {
  Symbol * s = symbolTable->getSymbol(sym);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    Symbol* symb = new Symbol(sym, -1, 0, true, false, -1); 
    symbolTable->add(symb);
    return;
  }
  if(s->sectionNumber != -1) { // ovde vrv nikad nece uci
    cout << "Symbol " << sym << " is already defined";
    return;
  }
}

/*
  ovo je gotovo
*/
void Assembler::processSection(char* name) {

  if((string)name == currSectionName) return; // nema sta, samo nastavi
  for(Section* s : sectionTable->sectionTable) {
    if(s->name == name) {
      // nastavi sekciju
      locationCounter = s->length;
      return;
    }
  }

  // kraj stare
  if(currSection != -1) {
    sectionTable->getSection(currSection)->length += locationCounter; 
    symbolTable->getSymbol(currSection)->size += locationCounter;
  }

  // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
  Symbol* newSection = new Symbol(name, 0, 0, false, true, 0);
  newSection->sectionNumber = newSection->number;
  symbolTable->add(newSection);

  locationCounter = 0;
  currSection = newSection->getNumber();
  currSectionName = name;

  Section* section = new Section(name, currSection);
  section->length = 0;
  sectionTable->add(section);
}

void Assembler::processWordSymbol(char* s) { // ovo se nigde ne pojavljuje?
  if(currSection == -1) {
    cout << "Word directive outside any section.";
    return;
  }
  Symbol* sym = symbolTable->getSymbol(s);
  if(sym == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    Symbol* symb = new Symbol(s, currSection, 0, false, false, -1);
    symbolTable->add(symb);
    symb->addBackpatchAddr(locationCounter, currSection);
    sectionTable->getSection(currSection)->addInstruction(0, locationCounter);
  } else {
    if(sym->defined) {
      sectionTable->getSection(currSection)->addInstruction(sym->value, locationCounter);
    } else {
      // frw ref
      sym->addBackpatchAddr(locationCounter, currSection);
      sectionTable->getSection(currSection)->addInstruction(0, locationCounter);
    }
  }
  locationCounter += 4;
}

/*
  gotovo
*/
void Assembler::processWordLiteral(int val) {
  if(currSection == -1) {
    cout << "Word directive outside any section.";
    return;
  }
  sectionTable->getSection(currSection)->addInstruction(val, locationCounter);
  locationCounter += 4;
}

/*
  gotovo
*/
void Assembler::processSkip(int num) {
  if(currSection == -1) {
    cout << "Skip directive is not in a section" << endl;
    return;
  }
  sectionTable->getSection(currSectionName)->skip(locationCounter, num);
  locationCounter += num;
}

/*
  gotovo
*/
void Assembler::processEnd() {
  if(currSection == -1) {
    cout << "End directive does not end any section." << endl;
    return;
  }
  sectionTable->getSection(currSection)->length += locationCounter; 
  symbolTable->getSymbol(currSection)->size += locationCounter;
  endFound = true;
}


/*****************************************************************************************/
//                    INSTRUCTIONS 
/*****************************************************************************************/


void Assembler::processHalt() {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  sectionTable->getSection(currSectionName)->addInstruction(OpHalt, locationCounter);
  locationCounter += 4;
}

void Assembler::processInt() {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  sectionTable->getSection(currSectionName)->addInstruction(OpInt, locationCounter);
  locationCounter += 4;
}

void Assembler::processIret() {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  // pop pc
  unsigned int instruction1 = OpPop | 0x00F00000 | 0x000E0000 | 0x004;

  // pop status
  unsigned int instruction2 = OpLd_CsrMemGprAddD | 0x00000000 | 0x000E0000 | 0x004;

  sectionTable->getSection(currSectionName)->addInstruction(instruction1, locationCounter);
  locationCounter += 4;
  sectionTable->getSection(currSectionName)->addInstruction(instruction2, locationCounter);
  locationCounter += 4;
}

void Assembler::processArithmeticLogic(ArLoSInstr instr, int gprS, int gprD) {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction = (gprD << 20) & 0x00F00000 | (gprD << 16) & 0x000F0000 | (gprS << 12) & 0x0000F000;

  switch (instr) {
  case ADD:
    instruction |= OpAdd;
    break;
  case SUB:
    instruction |= OpSub;
    break;
  case MUL:
    instruction |= OpMul;
    break;
  case DIV:
    instruction |= OpDiv;
    break;
  case NOT:
    instruction |= OpNot;
    break;
  case AND:
    instruction |= OpAnd;
    break;
  case OR:
    instruction |= OpOr;
    break;
  case XOR:
    instruction |= OpXor;
    break;
  case SHL:
    instruction |= OpShl;
    break;
  case SHR:
    instruction |= OpShr;
    break;
  }
  // dodaj u sadrzaj sekcije u kojoj se instrukcija nalazi
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
  // locationCounter se uvecava nakon obrade 
  locationCounter += 4;
}

void Assembler::processXchg(int reg1, int reg2) {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction = OpXchg | (reg1 << 16) & 0x000F0000 | (reg2 << 12) & 0x0000F000;
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processStack(StackInstr instr, int reg) {
  /* sp = r14 */
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction;
  switch(instr) {
    case PUSH: 
      instruction = OpPush | 0x00E00000 | (reg << 12) & 0x00F00000 | 0xFFC;
      break;
    case POP: 
      instruction = OpPop | 0x000E0000 | (reg << 20) & 0x00F00000 | 0x004;
      break;
  }
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processCsr(CsrInstr instr, int reg1, int reg2) {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction;
  switch(instr) {
    case READ:  
      instruction = OpCsrrd | (reg2 << 20) & 0x00F00000 | (reg1 << 16) & 0x000F0000;
      break;
    case WRITE: 
      instruction = OpCsrwr | (reg1 << 20) & 0x00F00000 | (reg2 << 16) & 0x000F0000;
      break;
  }
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processRet() {
  if (currSection == -1) {
    cout << "Instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction = OpRet | 0x000E0000 | 0x004;
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}


/*****************************************************************************************/
//                    BRANCHES 
/*****************************************************************************************/

void Assembler::processBranchNum(unsigned int opSmall, unsigned int opBig, int reg1, int reg2, int litValue) {
  if(!isLiteralBig(litValue)) {
    unsigned int instr = opSmall | (reg1 << 16) | (reg2 << 12) | (0x0 << 20) | (litValue & 0xFFF); 
    sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
    locationCounter += 4;
  } else {
    // prvo proverim ima li u temp poolu
    BigLiteral *bl = sectionTable->getSection(currSection)->findLiteralInTempPool(litValue);
    if(bl == nullptr) {
      BigLiteral *b = new BigLiteral(litValue);
      b->fixupAdr.push_back(locationCounter); // na loccnt (12b) od pocetka, ubacicu pomeraj 
      sectionTable->getSection(currSection)->temporaryPool.push_back(b);
    } else {
      bl->fixupAdr.push_back(locationCounter);
    }     
    unsigned int instr = opBig | (reg1 << 16) | (reg2 << 12) | (0xF << 20) | (0x000); // 0F -> pc
    sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
    locationCounter += 4;
  }
}

void Assembler::processBranchSym(unsigned int opSmall, unsigned int opBig, int reg1, int reg2) {
  Symbol* s = symbolTable->getSymbol(cur_branch_op_sym);
  if(s != nullptr) {
    if(s->defined) {
      processBranchNum(opSmall, opBig, reg1, reg2, s->value);
      return;
    } else {
      // u ts, nije definisan:
      // da li je simbol vec u temp pool u?
      BigLiteral* bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(s);
      if(bl != nullptr) {
        // dodaj backpatch
        bl->fixupAdr.push_back(locationCounter);
      } else {
        // dodaj simbol u temp pool
        BigLiteral* newBL = new BigLiteral(s);
        newBL->fixupAdr.push_back(locationCounter);
        sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
      }    
    }
  } else {
    Symbol* s = new Symbol(cur_branch_op_sym, currSection, 0, false, false, 0);
    symbolTable->add(s);
    BigLiteral* newBL = new BigLiteral(s);
    newBL->fixupAdr.push_back(locationCounter);
    sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
  }
  unsigned int instr = opBig | (reg1 << 16) | (reg2 << 12) | (0xF << 20) | (0x000); 
  sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
  locationCounter += 4;
  /*
    kad se pravi bazen na kraju, ako sym i dalje nije def, moze se        !!!!!
    u okviru samog simbola u njegovoj backpatch listi                     !!!!!
      (sva mesta u kodu gde treba da pise vr simbola)                     !!!!!
    upisati adresu u okviru bazena literala na koje treba da se           !!!!!
      smesti vr simbola
  */
  
}

void Assembler::processBeq(int reg1, int reg2) {
  if(currSection == -1) {
    cout << "Branch outside any section." << endl;
    return;
  }
  switch(branchOpType) {
    case NUM:
      processBranchNum(OpBeqRegdirD, OpBeqMemdirD, reg1, reg2, cur_branch_op_num);
      cur_branch_op_num = 0;
      break;
    case STR: 
      processBranchSym(OpBeqRegdirD, OpBeqMemdirD, reg1, reg2);
      cur_branch_op_sym = "";
      break;
  }
  branchOpType = B_NO;
}

void Assembler::processBne(int reg1, int reg2) {
  if(currSection == -1) {
    cout << "Branch outside any section." << endl;
    return;
  }
  switch(branchOpType) {
    case NUM:
      processBranchNum(OpBneRegdirD, OpBneMemdirD, reg1, reg2, cur_branch_op_num);
      cur_branch_op_num = 0;
      break;
    case STR: 
      processBranchSym(OpBneRegdirD, OpBneMemdirD, reg1, reg2);
      cur_branch_op_sym = "";
      break;
  }
  branchOpType = B_NO;
}

void Assembler::processBgt(int reg1, int reg2) {
  if(currSection == -1) {
    cout << "Branch outside any section." << endl;
    return;
  }
  switch(branchOpType) {
    case NUM:
      processBranchNum(OpBgtRegdirD, OpBgtMemdirD, reg1, reg2, cur_branch_op_num);
      cur_branch_op_num = 0;
      break;
    case STR: 
      processBranchSym(OpBgtRegdirD, OpBgtMemdirD, reg1, reg2);
      cur_branch_op_sym = "";
      break;
  }
  branchOpType = B_NO;
}

void Assembler::processBranchSymbol(char* s) {
  cur_branch_op_sym = s;
  branchOpType = STR;
}

void Assembler::processBranchLiteral(int i) {
  cur_branch_op_num = i;
  branchOpType = NUM;
}


/*****************************************************************************************/
//                    CALL & JMP 
/*****************************************************************************************/

void Assembler::processCallSymbol(char* s) {
  if(currSection == -1) {
    cout << "Call instruction outside any section.";
    return;
  }
  Symbol* sym = symbolTable->getSymbol(s);
  if(sym != nullptr) {
    // uzmi val
    if(sym->defined){
      processCallLiteral(sym->value);
      return;
    } else {
      BigLiteral* bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(sym);
      if(bl != nullptr) {
        bl->fixupAdr.push_back(locationCounter);
      } else {
        BigLiteral* newBL = new BigLiteral(sym);
        newBL->fixupAdr.push_back(locationCounter);
        sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
      }
    }
  } else {
    Symbol* s = new Symbol(cur_branch_op_sym, currSection, 0, false, false, -1);
    symbolTable->add(s);
    BigLiteral* newBL = new BigLiteral(s);
    newBL->fixupAdr.push_back(locationCounter);
    sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
  }
  unsigned int instruction = OpJsrMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc  
  sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processCallLiteral(int i) {
  if(currSection == -1) {
    cout << "Call outside any section." << endl;
    return;
  }
  unsigned int instruction = 0;
  if(isLiteralBig(i)) {
    BigLiteral *bl = sectionTable->getSection(currSection)->findLiteralInTempPool(i);
    if(bl == nullptr) {
      BigLiteral *b = new BigLiteral(i);
      b->fixupAdr.push_back(locationCounter); // na loccnt (12b) od pocetka, ubacicu pomeraj do literala u bazenu
      sectionTable->getSection(currSection)->temporaryPool.push_back(b);
    } else {
      bl->fixupAdr.push_back(locationCounter);
    }
    instruction = OpJsrMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc
  } else {
    instruction = OpJsrRegdirD | (i & 0x00000FFF);
  }
  sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processJmpLiteral(int i) {
  if (currSection == -1) {
    cout << "Jump instruction outside any section!" << endl;
    return;
  }
  unsigned int instruction = 0;
  if(isLiteralBig(i)) {
    BigLiteral *bl = sectionTable->getSection(currSection)->findLiteralInTempPool(i);
    if(bl != nullptr) {
      BigLiteral *b = new BigLiteral(i);
      b->fixupAdr.push_back(locationCounter); // na loccnt (12b) od pocetka, ubacicu pomeraj do literala u bazenu
      sectionTable->getSection(currSection)->temporaryPool.push_back(b);
    } else {
      bl->fixupAdr.push_back(locationCounter);
    }
    instruction = OpJmpMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc
  } else {
    instruction = OpJmpRegdirD | (i & 0x00000FFF);
  }
  sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}

void Assembler::processJmpSymbol(char *s) {
  if (currSection == -1) {
    cout << "Jump instruction outside any section!" << endl;
    return;
  }
  Symbol* sym = symbolTable->getSymbol(s);
  if(sym != nullptr) {
    // uzmi val
    if(sym->defined){
      processJmpLiteral(sym->value);
      return;
    } else {
      // u ts, nije definisan:
      BigLiteral* bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(sym);
      if(bl != nullptr) {
        // dodaj backpatch
        bl->fixupAdr.push_back(locationCounter);
      } else {
        // dodaj simbol u temp pool
        BigLiteral* newBL = new BigLiteral(sym);
        newBL->fixupAdr.push_back(locationCounter);
        sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
      }
    }
  } else {
    Symbol* s = new Symbol(cur_branch_op_sym, currSection, 0, false, false, -1);
    symbolTable->add(s);
    sectionTable->getSection(currSection)->temporaryPool.push_back(new BigLiteral(s));
  }
  unsigned int instruction = OpJmpMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc  
  sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
  locationCounter += 4;
}


/*****************************************************************************************/
//                    LOAD 
/*****************************************************************************************/

/*
  proverava da li je literal u temp pool-u trenutne sekcije, dodaje ako nije
*/
void Assembler::helperLoadBigLiteral(int bigLit) {
  BigLiteral *b = sectionTable->getSection(currSection)->findLiteralInTempPool(bigLit);
  if(b == nullptr) {
    // nema ga, dodaj
    b = new BigLiteral(bigLit);
    sectionTable->getSection(currSection)->temporaryPool.push_back(b);
  }
  b->fixupAdr.push_back(locationCounter);
}

/*
  provera ima li simbola u ts, dodaje ako nema
  proverava je li simbol u trmp poolu, dodaje ako nije
*/
void Assembler::helperLoadBigLiteralSymbol(string name) {
  Symbol* s = symbolTable->getSymbol(name);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    s = new Symbol(cur_load_sym_op, currSection, -1, false, false, 0);
    symbolTable->add(s);
  }
  if(s->defined == false) {
    BigLiteral *b = sectionTable->getSection(currSection)->findLitSymbolInTempPool(s);
    if(b == nullptr) {
      b = new BigLiteral(s);
      sectionTable->getSection(currSection)->temporaryPool.push_back(b);
    }
    b->fixupAdr.push_back(locationCounter);
  }
  if(s->defined == true && isLiteralBig(s->value)) {
    BigLiteral *bl = sectionTable->getSection(currSection)->findLiteralInTempPool(s->value);
    if(bl == nullptr) bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(s);
    if(bl == nullptr) {
      bl = new BigLiteral(s);
      sectionTable->getSection(currSection)->temporaryPool.push_back(bl);
    }
    bl->fixupAdr.push_back(locationCounter);
  }
}

void Assembler::processLoad(int reg) {
  if(currSection == -1) {
    cout << "Load instruction outside any section!" << endl;
    return;
  }
  bool bigLiteral = isLiteralBig(cur_load_num_op);
  BigLiteral* b = nullptr;
  Symbol* s = nullptr;
  unsigned int instruction;

  switch(loadType) {
    case L_IMMED_N: /* DONE */
      //cout << "load imm lit: ld " << cur_load_num_op << " %" << reg << endl;
      if(bigLiteral) { 
        // prvo provera ima li ga u tom bazenu:
        
        helperLoadBigLiteral(cur_load_num_op);
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0 << 12) | (0x000);
      } else {
        instruction = OpLd_GprD | (reg << 20) | (cur_load_num_op & 0x00FFF);
      }
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_IMMED_S: /* DONE */
      /* doda u ts, u pool, ako treba */
      helperLoadBigLiteralSymbol(cur_load_sym_op); // moze ovde 
      s = symbolTable->getSymbol(cur_load_sym_op); // nikad nije nullptr

      if(s->defined && !isLiteralBig(s->value)) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0x0 << 16) | (0x0 << 12) | (s->value & 0xFFF);
      } else if(s->defined && isLiteralBig(s->value)) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0000); // bice prepravljeno
      } else if(!s->defined) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0000); // bice prepravljeno
      }
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_MEMDIR_N: /* DONE */
      if(bigLiteral) {
        helperLoadBigLiteral(cur_load_num_op);
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0000);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (reg << 16) | (0x0000);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      } else {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0x0 << 16) | (0x0 << 12) | (cur_load_num_op & 0xFFF);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      }
      break;
    case L_MEMDIR_S: /* DONE */
      helperLoadBigLiteralSymbol(cur_load_sym_op); /* dodato u temp pool sta treba */
      s = symbolTable->getSymbol(cur_load_sym_op); // nikad nije nullptr
      if(s->defined && !isLiteralBig(s->value)) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0x0 << 16) | (0x0 << 12) | (s->value & 0xFFF);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      } else {
        // ako je definisan i vrednost > 12b ILI ako je nedefinisan:
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0000);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (reg << 16) | (0x0000);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      }
      break;
    case L_REGDIR: /* DONE */
      instruction = OpLd_GprD | (reg << 20) | (cur_load_reg_op << 16) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_REGIND: /* DONE */
      instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (cur_load_reg_op << 16) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_REGINDOFF_N: /* DONE */
      if(bigLiteral) {
        cout << "Error: displacement value is too big: " << cur_load_num_op << endl;
        return;
      }
      instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (cur_load_reg_op << 16) | (0x0 << 12) | (cur_load_num_op & 0xFFF);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_REGINDOFF_S: /* DONE */
      s = symbolTable->getSymbol(cur_load_sym_op);
      if(s == nullptr) {
        cout << "Symbol must be defined." << endl;
        return;
      }
      if(s->defined == false || isLiteralBig(s->value)) {
        cout << "Symbol is undefined or too big." << endl;
        return;
      }
      instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (s->value << 16) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_NOTYPE:
    default: 
      cout << "Error in load instruction." << endl; 
  }
  loadType = L_NOTYPE;
}

void Assembler::loadImmedLiteral(int i) { 
  cur_load_num_op = i; 
  loadType = L_IMMED_N;
}

void Assembler::loadImmedSymbol(char* s) {
  cur_load_sym_op = s;
  loadType = L_IMMED_S;
}

void Assembler::loadMemdirLiteral(int i) {
  cur_load_num_op = i;
  loadType = L_MEMDIR_N;
}

void Assembler::loadMemdirSymbol(char* s) {
  cur_load_sym_op = s;
  loadType = L_MEMDIR_S;
}

void Assembler::loadRegdir(int r) {
  cur_load_reg_op = r;
  loadType = L_REGDIR;
}

void Assembler::loadRegind(int r) {
  cur_load_reg_op = r;
  loadType = L_REGIND;
}

void Assembler::loadRegindOffLiteral(int r, int i) {
  cur_load_reg_op = r;
  cur_load_num_op = i;
  loadType = L_REGINDOFF_N;
}

void Assembler::loadRegindOffSymbol(int r, char* s) {
  cur_load_reg_op = r;
  cur_load_sym_op = s;
  loadType = L_REGINDOFF_S;
}



/*****************************************************************************************/
//                    STORE 
/*****************************************************************************************/

void Assembler::processStore(int reg) {
  if(currSection == -1) {
    cout << "Store instruction outside any section!" << endl;
    return;
  }
  bool bigLiteral = isLiteralBig(cur_store_num_op);
  BigLiteral* b = nullptr;
  Symbol* s = nullptr;
  unsigned int instruction;

  switch(storeType) {
    case S_MEMDIR_N:
      if(!bigLiteral) {
        instruction = OpSt_TwoGprsSumD | (0x0 << 20) | (0x0 << 16) | (reg << 12) | (cur_store_num_op & 0xFFF);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      } else {
        helperLoadBigLiteral(cur_store_num_op); // doda u temp pool ako treba
        instruction = OpSt_MemTwoGprsSumD | (reg << 12) | (0xF << 20); // Disp ce biti prepravljeno
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      }
      break;
    case S_MEMDIR_S: /* DONE */
      helperLoadBigLiteralSymbol(cur_store_sym_op); /* dodato u temp pool sta treba */
      s = symbolTable->getSymbol(cur_store_sym_op); // nikad nije nullptr
      if(s->defined && !isLiteralBig(s->value)) {
        instruction = OpSt_TwoGprsSumD | (0x0 << 20) | (0x0 << 16) | (reg << 12) | (s->value & 0xFFF);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      } else {
        // ako je definisan i vrednost > 12b ILI ako je nedefinisan:
        instruction = OpSt_MemTwoGprsSumD | (0xF << 20) | (0x0 << 16) | (reg << 12) | (0x000);
        sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
        locationCounter += 4;
      }
      break;
    case S_REGDIR:
      instruction = OpLd_GprD | (reg << 16) | (cur_load_reg_op << 20) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break; 
    case S_REGIND:
      instruction = OpSt_TwoGprsSumD | (reg << 12) | (cur_load_reg_op << 20) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case S_REGINDOFF_N:
      if(bigLiteral) {
        cout << "Error: displacement value is too big: " << cur_store_num_op << endl;
        return;
      }
      instruction = OpSt_TwoGprsSumD | (reg << 12) | (cur_store_reg_op << 20) | (cur_store_num_op & 0xFFF);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case S_REGINDOFF_S:
      s = symbolTable->getSymbol(cur_store_sym_op);
      if(s == nullptr) {
        cout << "Symbol must be defined." << endl;
        return;
      }
      if(s->defined == false || isLiteralBig(s->value)) {
        cout << "Symbol is undefined or too big." << endl;
        return;
      }
      instruction = OpSt_TwoGprsSumD | (reg << 12) | (cur_store_reg_op << 20) | (s->value & 0xFFF);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case S_NOTYPE:
    default: 
      cout << "Error in store instruction." << endl; 
  }
  storeType = S_NOTYPE;
}

void Assembler::storeMemdirLiteral(int i) {
  cur_store_num_op = i;
  storeType = S_MEMDIR_N;
}

void Assembler::storeMemdirSymbol(char* s) {
  cur_store_sym_op = s;
  storeType = S_MEMDIR_S;
}

void Assembler::storeRegdir(int reg) {
  cur_store_reg_op = reg;
  storeType = S_REGDIR;
}

void Assembler::storeRegind(int reg) {
  cur_store_reg_op = reg;
  storeType = S_REGIND;
}

void Assembler::storeRegindOffLiteral(int reg, int num) {
  cur_store_num_op = num;
  cur_store_reg_op = reg;
  storeType = S_REGINDOFF_N;
}

void Assembler::storeRegindOffSymbol(int reg, char* sym) {
  cur_store_sym_op = sym;
  cur_store_reg_op = reg;
  storeType = S_REGINDOFF_S;
}