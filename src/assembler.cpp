#include "../inc/assembler.hpp"
#include <iomanip>

Assembler::Assembler() {
  locationCounter = 0;
  currSection = -1;
  currSectionName = "";
  symbolTable = new SymbolTable();
  sectionTable = new SectionTable();
}

Assembler::~Assembler() {
  delete symbolTable;
  delete sectionTable;
}

int Assembler::assemble(string arg) {

  FILE *file = fopen((std::string("tests/") + arg).c_str(), "r");
  
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
  // idem kroz flink - tu se mogu naci samo (sada) poznati ili nepoznati simboli
  // pravim     rela zapise
  symbolTable->backpatchWordDirectiveAddresses(sectionTable);

  // pravi bazene.. + relokacije za bazen
  // neke relokacije prave se tokom prolaza
  generatePools();

  // kreiranje rela sekcija, za svaku sekciju koja ima relokacije
  generateRelaSections();

  // pravim sekciju za tabelu simbola i za tabelu stringova
  createStringAndSymbolSections(); 

  // pravljenje fajlova
  createAssemblerOutputFiles(arg);
 
  return 0;
}

bool Assembler::isLiteralBig(int num) {
  if(num > ((1<<11) - 1) || num < - (1 << 11)) {  
    return true;
  }
  return false;
}


/*****************************************************************************************/
//                    LABEL
/*****************************************************************************************/

void Assembler::processLabelDefinition(char *labelName) { 
  if (currSection == -1) {
    cout << "Label outside any section!" << endl;
    return;
  }
  Symbol* sym = symbolTable->getSymbol(labelName);
  
  if(sym == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined) 
    Symbol* symb = new Symbol(labelName, currSection, locationCounter, false, true);
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
      return;
    }
  }
}



/*****************************************************************************************/
//                    DIRECTIVES
/*****************************************************************************************/

void Assembler::processGlobal(char* sym) {
  Symbol* s = symbolTable->getSymbol(sym);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined) 
    Symbol* symb = new Symbol(sym, currSection, 0, true, false);
    symbolTable->add(symb);
    return;
  } else { // ovde vrv nikad nece uci
    s->isGlobal = true;
    return;
  }
}

void Assembler::processExtern(char* sym) {
  Symbol * s = symbolTable->getSymbol(sym);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined) 
    Symbol* symb = new Symbol(sym, -1, 0, true, false); 
    symbolTable->add(symb);
    return;
  }
  if(s->sectionNumber != -1) { // ovde vrv nikad nece uci
    cout << "Symbol " << sym << " is already defined";
    return;
  }
}

void Assembler::processSection(char* name) {
  if((string)name == currSectionName) return; 
  for(Section* s : sectionTable->sectionTable) {
    if(s->name == name) {
      // update sekcije koja se zavrsila:
      if(currSection != -1) {
        sectionTable->getSection(currSection)->length += locationCounter; 
        symbolTable->getSymbol(currSection)->size += locationCounter;
      }
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

  // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined) 
  Symbol* newSection = new Symbol(name, 0, 0, false, true);
  newSection->sectionNumber = newSection->number;
  symbolTable->add(newSection);

  locationCounter = 0;
  currSection = newSection->getNumber();
  currSectionName = name;

  Section* section = new Section(name, currSection);
  section->length = 0;
  section->type = SHT_PROGBITS;
  sectionTable->add(section);
}

void Assembler::processWordSymbol(char* s) {  
  if(currSection == -1) {
    cout << "Word directive outside any section.";
    return;
  }
  Symbol* sym = symbolTable->getSymbol(s);
  if(sym == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined) 
    Symbol* symb = new Symbol(s, -1, 0, false, false);
    symbolTable->add(symb);
    symb->addBackpatchAddr(locationCounter, currSection);
    sectionTable->getSection(currSection)->addInstruction(0, locationCounter);
  } else {
    if(sym->defined) {
      sectionTable->getSection(currSection)->addInstruction(sym->value, locationCounter);
    } else {
      sym->addBackpatchAddr(locationCounter, currSection);
      sectionTable->getSection(currSection)->addInstruction(0, locationCounter);
    }
  }
  locationCounter += 4;
}

void Assembler::processWordLiteral(int val) {
  if(currSection == -1) {
    cout << "Word directive outside any section.";
    return;
  }
  sectionTable->getSection(currSection)->addInstruction(val, locationCounter);
  locationCounter += 4;
}

void Assembler::processSkip(int num) {
  if(currSection == -1) {
    cout << "Skip directive is not in a section" << endl;
    return;
  }
  sectionTable->getSection(currSectionName)->skip(locationCounter, num);
  locationCounter += num;
}

void Assembler::processEnd() {
  if(currSection == -1) {
    cout << "End directive does not end any section." << endl;
    return;
  }
  sectionTable->getSection(currSection)->length += locationCounter; 
  symbolTable->getSymbol(currSection)->size += locationCounter;
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
  // clean cause reg
  unsigned int instruction0 = OpCsrwr | 0x00200000;

  // pop handler
  unsigned int instruction1 = OpLd_CsrMemSumGprsD | 0x000E0004;

  // pop pc
  unsigned int instruction2 = OpLd_MemGprAddD | 0x00FE0000 | 0x008;

  sectionTable->getSection(currSectionName)->addInstruction(instruction0, locationCounter);
  locationCounter += 4;
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
  sectionTable->getSection(currSectionName)->addInstruction(instruction, locationCounter);
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
      instruction = OpPush | 0x00E00000 | ((reg << 12) & 0x0000F000) | 0xFFC;
      break;
    case POP: 
      instruction = OpPop | 0x000E0000 | ((reg << 20) & 0x00F00000) | 0x004;
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
  unsigned int instruction = OpRet | 0x00FE0004;
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
  unsigned int instr;
  if(s != nullptr && s->defined && !isLiteralBig(s->value)) {

    if(currSection == s->sectionNumber) {
      unsigned int offset = s->value - locationCounter - 4;
      instr = opSmall | (reg1 << 16) | (reg2 << 12) | (0x0 << 20) | (offset & 0xFFF); 
      sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
      locationCounter += 4;
      return;
    }

    unsigned int instr = opSmall | (reg1 << 16) | (reg2 << 12) | (0x0 << 20); 
    sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
    locationCounter += 4; 

    // ovde pravim relokacioni, pc! 
    RELA_makePC_Relocation(locationCounter - 4, s);

    return;
  }
  if(s == nullptr) {
    s = new Symbol(cur_branch_op_sym, -1, 0, false, false);
    symbolTable->add(s);
  }
  BigLiteral *bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(s);
  if(bl == nullptr) {
    BigLiteral *b = new BigLiteral(s);
    b->isSymbolValue = true;
    b->fixupAdr.push_back(locationCounter); // na loccnt (12b) od pocetka, ubacicu pomeraj 
    sectionTable->getSection(currSection)->temporaryPool.push_back(b);
  } else {
    bl->fixupAdr.push_back(locationCounter);
  }     
  instr = opBig | (reg1 << 16) | (reg2 << 12) | (0xF << 20) | (0x000); // 0F -> pc
  sectionTable->getSection(currSection)->addInstruction(instr, locationCounter);
  locationCounter += 4; 
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

void Assembler::processCallLiteral(int i) {
  if(currSection == -1) {
    cout << "Call outside any section." << endl;
    return;
  }
  unsigned int instruction = 0;
  if(isLiteralBig(i)) {
    BigLiteral *bl = sectionTable->getSection(currSection)->findLiteralInTempPool(i);
    if(bl == nullptr) {
      bl = new BigLiteral(i); 
      bl->fixupAdr.push_back(locationCounter); // na loccnt (12b) od pocetka, ubacicu pomeraj do literala u bazenu
      sectionTable->getSection(currSection)->temporaryPool.push_back(bl);
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

void Assembler::processCallSymbol(char* s) {
  if(currSection == -1) {
    cout << "Call instruction outside any section.";
    return;
  }
  unsigned int instruction;
  Symbol* sym = symbolTable->getSymbol(s);

  if(sym != nullptr && sym->defined && !isLiteralBig(sym->value)) {

    if(currSection == sym->sectionNumber) {

      unsigned int offset = sym->value - locationCounter - 4;
      instruction = OpJsrRegdirD | (0x00F00000) | (offset & 0x00000FFF);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      return;
      
    }

    instruction = OpJsrRegdirD | (0x00F00000);
    sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
    locationCounter += 4;

    // ovde pravim relokacioni, pc! 
    RELA_makePC_Relocation(locationCounter - 4, sym);

    return;
  }
  if(sym == nullptr) {
    sym = new Symbol(s, -1, 0, false, false);
    symbolTable->add(sym);
  }
  BigLiteral* bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(sym);
  if(bl != nullptr) {
    bl->fixupAdr.push_back(locationCounter);
  } else {
    BigLiteral* bl = new BigLiteral(sym);
    bl->isSymbolValue = true;
    bl->fixupAdr.push_back(locationCounter);
    sectionTable->getSection(currSection)->temporaryPool.push_back(bl);
  }
  instruction = OpJsrMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc  
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
  unsigned int instruction;
  Symbol* sym = symbolTable->getSymbol(s);

  if(sym != nullptr && sym->defined && !isLiteralBig(sym->value)) {
    // novo:
    if(currSection == sym->sectionNumber) {

      unsigned int offset = sym->value - locationCounter -4;
      instruction = OpJmpRegdirD | (0x00F00000) | (offset & 0x00000FFF);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      return;

    }  

    instruction = OpJmpRegdirD | (0x00F00000);
    sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
    locationCounter += 4;
    cout << "PRAVIM relokaciju u JMP symbol..." << endl; 
    RELA_makePC_Relocation(locationCounter - 4, sym);
    
    return;
  }
  if(sym == nullptr) {
    sym = new Symbol(s, -1, 0, false, false);
    symbolTable->add(sym);
  }
  BigLiteral* bl = sectionTable->getSection(currSection)->findLitSymbolInTempPool(sym);
  if(bl != nullptr) {
    bl->fixupAdr.push_back(locationCounter);
  } else {
    BigLiteral* newBL = new BigLiteral(sym);
    newBL->isSymbolValue = true;
    newBL->fixupAdr.push_back(locationCounter);
    sectionTable->getSection(currSection)->temporaryPool.push_back(newBL);
  }
  instruction = OpJmpMemdirD | (0x0 << 16) | (0x0 << 12) | (0xF << 20) | (0x000); // 0F -> pc 
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
  proverava je li simbol u temp pool-u, dodaje ako nije
*/
void Assembler::helperLoadBigLiteralSymbol(string name) {
  
  Symbol* s = symbolTable->getSymbol(name);
  if(s == nullptr) {
    // Symbol(char* name, short sectionNum, int value, bool isGlobal, bool defined, int size) 
    s = new Symbol(cur_load_sym_op, -1, -1, false, false);
    symbolTable->add(s);
  }
  bool addedToPool = false;
  if(!s->defined || isLiteralBig(s->value)) {
    BigLiteral *b = sectionTable->getSection(currSection)->findLitSymbolInTempPool(s);
    if(b == nullptr) {
      b = new BigLiteral(s);
      b->isSymbolValue = true;
      sectionTable->getSection(currSection)->temporaryPool.push_back(b);
    }
    b->fixupAdr.push_back(locationCounter);
    addedToPool = true;
  }
  if(!addedToPool) {
    // znaci vrednost simbola je definisana i mala
    // pravim relokacioni zapis na ovom mestu:

    RELA_makeABS_12_Relocation(locationCounter, s);
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
    case L_IMMED_N: 
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
    case L_IMMED_S: 
      /* doda u ts, u pool, ako treba */
      helperLoadBigLiteralSymbol(cur_load_sym_op); // moze ovde 
      s = symbolTable->getSymbol(cur_load_sym_op); // nikad nije nullptr

      if(s->defined && !isLiteralBig(s->value)) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0x0 << 16) | (0x0 << 12) | (s->value & 0xFFF);
      } else if(!s->defined || isLiteralBig(s->value)) {
        instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (0xF << 16) | (0x0000); // bice prepravljeno
      } 
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_MEMDIR_N:  
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
      s = symbolTable->getSymbol(cur_load_sym_op);  
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
    case L_REGDIR: 
      instruction = OpLd_GprD | (reg << 20) | (cur_load_reg_op << 16) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_REGIND:  
      instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (cur_load_reg_op << 16) | (0x0000);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break;
    case L_REGINDOFF_N: 
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
      instruction = OpLd_MemTwoGprsSumD | (reg << 20) | (cur_load_reg_op << 16) | (s->value & 0xFFF);
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
    case S_MEMDIR_S:  
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
      instruction = OpLd_GprD | (reg << 16) | (cur_store_reg_op << 20);
      sectionTable->getSection(currSection)->addInstruction(instruction, locationCounter);
      locationCounter += 4;
      break; 
    case S_REGIND:
      instruction = OpSt_TwoGprsSumD | (reg << 12) | (cur_store_reg_op << 20) | (0x0000);
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



/*****************************************************************************************/
//                    ASSEMBLER HELPERS 
/*****************************************************************************************/


void Assembler::fillStringTableContent(Section* strTable) {
  int index = 0;
  for(Symbol *symbol: symbolTable->symbolTable) {
    strTable->STR_TAB_Section_Names.push_back(symbol->symbolName);
    symbol->stringTableIndex = index++;
    for(Section *section: sectionTable->sectionTable) {
      if(symbol->symbolName == section->name) {
        section->strTableIndex = symbol->stringTableIndex;
      } 
    }
  } 
}


// NIJE GOTOVO
void Assembler::createStringAndSymbolSections() {
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
  fillStringTableContent(STR_TAB);
  STR_TAB->length = (symbolTable->tableSize() + 1) * sizeof(string); 
  STR_TAB->strTableIndex = -1; // samo kod ove, string sekcije je tako
  STR_TAB->type = SHT_STRTAB;
  // elf offset
  STR_TAB->entrySize = sizeof(string);
  sectionTable->add(STR_TAB);
  
  SYM_TAB->bestieSection = STR_TAB->sectionTableIndex;
}

int Assembler::getStringTableIndex() {
  for(Section *s : sectionTable->sectionTable) {
    if(s->type == SHT_STRTAB) return s->sectionTableIndex;
  }
  return -1;
}

int Assembler::getSymbolTableIndex() {
  for(Section *s : sectionTable->sectionTable) {
    if(s->type == SHT_SYMTAB) return s->sectionTableIndex;
  }
  return -1;
}

void Assembler::generatePools() {

  for(Section *section : sectionTable->sectionTable) {

    for(BigLiteral *bigLiteral : section->temporaryPool) {
      // sekcija ima length, ukupno, i ima poolSize

      // upisem literal u sekciju, pa patchujem
      // upisujem literalnu vrednost;
      // ako se radi o vrednosti simbola, pravim i relokacioni zapis
      // moguce je da je vrednost simbola ostala nedefinisana (extern simbol), pa upisujem 0x00000000
      bool isSymbolValue = bigLiteral->isSymbolValue;
      int value;
      if(isSymbolValue) { 
        //cout << bigLiteral->sym->symbolName << endl;
        if(bigLiteral->sym->defined) {
          value = bigLiteral->sym->value;
        } else {
          // simbol je extern 
          value = 0x00000000;
        }
      } else {
        // literalna vrednost:
        value = bigLiteral->literal;
        //cout << hex << value << dec << endl;
      }

      // upisivanje u bazen:
      section->addInstruction(value, section->length);
      
      // prepravljanje instrukcije da skace na adr u bazenu:
      int pomerajDoBazena;

      for(int fixingAddress : bigLiteral->fixupAdr) {

        pomerajDoBazena = section->length - fixingAddress - 4;

        // upis ovog pomeraja na 12 bita na fixingAdresu
        section->code[fixingAddress] = pomerajDoBazena & 0xFF;
        pomerajDoBazena = pomerajDoBazena >> 8;
        section->code[fixingAddress + 1] = (section->code[fixingAddress + 1] & 0xF0) | (pomerajDoBazena & 0x0F);
      }
      
      // za svaki literal u bazenu koji je vrednost simbola pravi se 32b relokacioni zapis:
      if(isSymbolValue) {
        RELA_makeABS_32_Relocation(section->length, bigLiteral->sym, section);
      }

      section->length += 4;
      section->poolSize += 4;     

    }
  }
}

void Assembler::generateRelaSections() {
  
  int lastSymbolTableNumber = symbolTable->getMaxNumber();

  for(Section * section : sectionTable->sectionTable) {

    if(section == nullptr) break;
    if(section->relocationTable == nullptr) break;
    if(section->relocationTable->relocationTable.empty()) break;

    string relaSectionName = ".rela." + section->name;
    Section * relaSection = new Section(relaSectionName, ++lastSymbolTableNumber);
    relaSection->bestieSection = getSymbolTableIndex();
    relaSection->type = SHT_RELA;
    relaSection->entrySize = 4 * sizeof(int); // 16
    relaSection->info = section->sectionTableIndex;
  
    sectionTable->add(relaSection);

    // upisujem sadrzaj:

    for(RelatableEntry *relocation : section->relocationTable->relocationTable) {
      // offset, symNum, addend, relaType
      relaSection->addInstruction(relocation->getOffset(), relaSection->length);
      relaSection->length += 4;
      relaSection->addInstruction(relocation->getSymbolRefNum(), relaSection->length);
      relaSection->length += 4;
      relaSection->addInstruction(relocation->getAddend(), relaSection->length);
      relaSection->length += 4;
      unsigned int relaType;
      switch(relocation->getType()) {
        case R_X86_64_32S : relaType = 0; break;
        case R_X86_64_12S : relaType = 1; break;
        case R_X86_64_PC32: relaType = 2; break;
      }
      relaSection->addInstruction(relaType, relaSection->length); 
      // cout << "upis tipa relok: " << relocation->getType()<< endl << "reala type " << relaType << endl; 
      relaSection->length += 4;
    } 
  }
}

/*
  verovatno cu izbaciti upisaivanje indexa u str_tab u binarnom fajlu
  jer sam u bin. fajl vec upisala imena - stringove
*/
void Assembler::createAssemblerOutputFiles(string arg) {

  string fileName;
  int pos = arg.find('.');
  if(pos != std::string::npos) {
    fileName = arg.substr(0, pos); 
  } else {
    // error
  } 

  string txtFileName = "tests/" + fileName + ".txt";

  std::ofstream outputFile(txtFileName); 
  if (outputFile.is_open()) {    
    outputFile << "ELF header:" << endl <<
    "------------------------------------------------------------------------------------------------- " << endl <<
    "Magic: 7f 45 4c 46 02 01 01 00 00 00 00 00 00 00"                    << endl <<
    "Class:                             ELF64"                            << endl <<
    "Data:                              2's complement, little endian"    << endl <<
    "Version:                           Current version"                  << endl <<
    "OS/ABI:                            UNIX System V ABI"                << endl <<
    "ABI version:                       0"                                << endl <<
    "Type:                              1  - Relocatable file"            << endl <<
    "Machine:                           62 - AMD x86-64 architecture"     << endl <<
    "Entry point address:               0x40000000"                       << endl <<
    "Program Header Table:              NULL"                             << endl << 
    "Section Header Table:              0"                                << endl <<
    "Flags:                             0"                                << endl <<
    "ELF header size:                   0x40"                             << endl <<
    "PHT entry size:                    NULL"                             << endl <<
    "PHT entries count:                 NULL"                             << endl <<
    "SHT entry size:                    " << sizeof(Section)              << endl <<
    "SHT entries count:                 " << sectionTable->tableSize()    << endl <<
    "String table index:                " << getStringTableIndex()        << endl << endl;   
 
    // JOS SECTION HEADER OFFSET - NAKON PISANJA BINARNOG
    outputFile << "Section Header Table: " << endl <<
    "------------------------------------------------------------------------------------------------- " << endl <<
    "     name       :   sh_name  :  sh_type   :  sh_offset : sh_size : sh_link : sh_info : sh_entsize " << endl <<
    "------------------------------------------------------------------------------------------------- " << endl;
    
    for(Section *s : this->sectionTable->sectionTable) {
      s->ELF_WriteSectionHeader(outputFile);
    } 
    outputFile << "------------------------------------------------------------------------------------------------- " << endl << endl;
    
    for(Section *s : this->sectionTable->sectionTable) {
      s->ELF_WriteSectionContent(outputFile, symbolTable);
    }
    outputFile.close();   
  } else {
    std::cout << "Failed to create output text file." << std::endl;   
  }  

  string objFileName = "tests/" + fileName + ".o";
  ofstream binaryFile(objFileName, ios::binary);

  uint32_t symTabSize = symbolTable->tableSize() - sectionTable->getNumProgbitsSections(); // bez sekcija!
  uint32_t progbitsSec = sectionTable->getNumProgbitsSections();
  uint32_t relaSec = sectionTable->getNumRelaSections();

  // upisujem broj simbola na prvo mesto
  binaryFile.write(reinterpret_cast<const char *>(&symTabSize), sizeof(uint32_t));

  // upisujem broj PROGBITS sekcija na drugo mesto
  binaryFile.write(reinterpret_cast<const char *>(&progbitsSec), sizeof(uint32_t));

  // upisujem broj RELA sekcija na trece mesto
  binaryFile.write(reinterpret_cast<const char *>(&relaSec), sizeof(uint32_t));

  int stringLength = 0;
  // upis sadrzaja tabele simbola: NE PISEM SEKCIJE
  for(Symbol * entry : symbolTable->symbolTable) { 
    int g = entry->isGlobal ? 1 : 0;
    int d = entry->defined ? 1 : 0;
    
    if(entry->number == entry->sectionNumber) {
      continue;
    }
    // 1) num, 2) str_tab_ind, 3) section, 4) isGlobal, 5) defined, 6) value, 7) len_name, 8) name
    binaryFile.write(reinterpret_cast<const char *>(&entry->number), sizeof(int));
    binaryFile.write(reinterpret_cast<const char *>(&entry->stringTableIndex), sizeof(int));
    binaryFile.write(reinterpret_cast<const char *>(&entry->sectionNumber), sizeof(short));
    binaryFile.write(reinterpret_cast<const char *>(&g), sizeof(int));  
    binaryFile.write(reinterpret_cast<const char *>(&d), sizeof(int));
    binaryFile.write(reinterpret_cast<const char *>(&entry->value), sizeof(unsigned int));

    stringLength = entry->symbolName.length();
    binaryFile.write(reinterpret_cast<const char *>(&stringLength), sizeof(int)); 
    
    binaryFile.write(entry->symbolName.c_str(), stringLength);

  }
  // upis sadrzaja i tabele sekcija:
  for(Section * section : sectionTable->sectionTable) {
    if(section->type == SHT_PROGBITS) {
      // 1) num, 2) len, 3) code, 4) len name, 5) name
      binaryFile.write(reinterpret_cast<const char *>(&section->sectionTableIndex), sizeof(int));
      binaryFile.write(reinterpret_cast<const char *>(&section->length), sizeof(unsigned int)); // da li mi treba
      binaryFile.write(reinterpret_cast<const char *>(section->code), section->length);
      stringLength = section->name.length();
      binaryFile.write(reinterpret_cast<const char *>(&stringLength), sizeof(int)); // da li mi treba
      binaryFile.write(section->name.c_str(), stringLength);
      // cout << "A: upisana progbits sekcija: " << section->name << endl;
    } 
  }

  // upis relokacija:
  for(Section * section : sectionTable->sectionTable) {
    if(section->type == SHT_RELA) {
      // 1) num, 2) len, 3) code, 4) len name, 5) name
      // code: offset, symbol, addend, type
      binaryFile.write(reinterpret_cast<const char *>(&section->sectionTableIndex), sizeof(int));
      binaryFile.write(reinterpret_cast<const char *>(&section->length), sizeof(unsigned int)); // da li mi treba
      binaryFile.write(reinterpret_cast<const char *>(section->code), section->length);
      stringLength = section->name.length();
      binaryFile.write(reinterpret_cast<const char *>(&stringLength), sizeof(int)); // da li mi treba
      binaryFile.write(section->name.c_str(), stringLength);
      // cout << "Upisana relokacija: " << "len: " << section->length << " naziv " << section->name << " duzina imena " << stringLength << endl;
      // cout << "A: upisana relokaciona sekcija: " << section->name << endl;
    } 
  }
  
  binaryFile.close();
}



/*****************************************************************************************/
//                    RELOCATION HELPERS
/*****************************************************************************************/

void Assembler::RELA_makeABS_32_Relocation(unsigned int locationCounter, Symbol* symbol, Section *section) {
  // section - sekcija za koju se pravi relokacioni zapis
  // cout << "RELA_makeABS_32_Relocation: ";
  int symNum, addend = 0;
  string symName;
  if(symbol->isGlobal) {
    symNum = symbol->number;
    symName = symbol->symbolName;
  } else {
    symNum = section->sectionTableIndex;
    symName = section->name;
    addend = symbol->value;
  }
  // cout << "sym: " << symName << " addend: "<< addend << " globalni: " << symbol->isGlobal << endl;
  RelatableEntry * rela = new RelatableEntry(locationCounter, symNum, addend, R_X86_64_32S, symName);
  section->relocationTable->addRelaEntry(rela); 
}

// ovde je addend uvek 0? uvek se upisuje direktna vrednost
// ovo je kad nema bazena i kada se na najnizih 12b instrukcije upisuje vrednost simbola
// za ovu od 12 b je okej da se koristi currSection
void Assembler::RELA_makeABS_12_Relocation(unsigned int locationCounter, Symbol* symbol) {
  //  cout << "RELA_makeABS_12_Relocation: " << endl;
  int symNum, addend = 0;
  string symName;
  if(symbol->isGlobal) {
    symNum = symbol->number;
    symName = symbol->symbolName;
  } else {
    // ne curr section! samo se dodaje u relokacione zapise te sekcije!
    Section * symbolsSection = sectionTable->getSection(symbol->sectionNumber);
    symNum = symbolsSection->sectionTableIndex;
    symName = symbolsSection->name;
    addend = symbol->value;
  }
  // cout << "sym: " << symName << " addend: "<< addend << " globalni: " << symbol->isGlobal << endl;
  RelatableEntry * rela = new RelatableEntry(locationCounter, symNum, addend, R_X86_64_12S, symName);
  sectionTable->getSection(currSection)->relocationTable->addRelaEntry(rela);
}

// ovde vec znam vrednost simbola...
void Assembler::RELA_makePC_Relocation(unsigned int locationCounter, Symbol* symbol) {
  // cout << "RELA_makePC_Relocation " << endl;
  // cout << "SIMBOL: " << symbol->symbolName << " " << symbol->value << endl;
  int symNum, addend = 0;
  string symName;
  if(symbol->isGlobal) {
    symNum = symbol->number;
    symName = symbol->symbolName;
    addend = -4;
  } else {
    symNum = currSection;
    symName = currSectionName;
    addend = symbol->value - 4;
  } 
  // cout << "sym: " << symName << " addend: " << addend<< " globalni: " << symbol->isGlobal <<
  //   "adresa za prepravljanje: " << locationCounter<< endl;
  RelatableEntry * rela = new RelatableEntry(locationCounter, symNum, addend, R_X86_64_PC32, symName);
  sectionTable->getSection(currSection)->relocationTable->addRelaEntry(rela);
}