#ifndef assembler_hpp
#define assembler_hpp

#include "symbolTable.hpp"
#include "sectionTable.hpp"
#include "relocationTable.hpp"
#include "../misc/parser.hpp"
#include "../misc/lexer.hpp"
#include <iostream>
#include <fstream>    
using namespace std;


class Assembler {

private:
  SymbolTable *symbolTable;
  SectionTable *sectionTable;

  int locationCounter;
  int currSection;        
  string currSectionName;
  bool endFound;

public: 
  enum ArLoSInstr   { ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR };
  enum StackInstr   { PUSH, POP };
  enum CsrInstr     { READ, WRITE };
  enum LoadType     { L_NOTYPE, L_IMMED_N, L_IMMED_S, L_MEMDIR_N, L_MEMDIR_S, L_REGDIR, L_REGIND, L_REGINDOFF_N, L_REGINDOFF_S };
  enum StoreType    { S_NOTYPE, S_MEMDIR_N, S_MEMDIR_S, S_REGDIR, S_REGIND, S_REGINDOFF_N, S_REGINDOFF_S };
  enum BranchOpType { B_NO, NUM, STR };

  void processSection(char *);
  void processSkip(int);
  void processGlobal(char *);
  void processExtern(char *);
  void processWordSymbol(char*);
  void processWordLiteral(int); 
  void processEnd();
  void processLabelDefinition(char *);
  void processArithmeticLogic(ArLoSInstr, int, int);
  void processXchg(int, int);
  void processStack(StackInstr, int);
  void processCsr(CsrInstr, int, int);
  void processHalt();
  void processInt();
  void processIret();
  void processRet();

  void processCallSymbol(char*);
  void processCallLiteral(int); 
  void processJmpLiteral(int); 
  void processJmpSymbol(char *);

  BranchOpType branchOpType = B_NO;
  int cur_branch_op_num = 0;
  string cur_branch_op_sym = "";
  void processBranchNum(unsigned int, unsigned int, int, int, int);
  void processBranchSym(unsigned int, unsigned int, int, int);
  void processBeq(int, int);
  void processBne(int, int); 
  void processBgt(int, int);
  void processBranchSymbol(char*);   
  void processBranchLiteral(int);  
  
  LoadType loadType = L_NOTYPE;
  int cur_load_num_op = 0;
  int cur_load_reg_op = 0;
  string cur_load_sym_op= "";
  void processLoad(int);
  void loadImmedLiteral(int i);         // IMMED
  void loadImmedSymbol(char*);          // IMMED
  void loadMemdirLiteral(int);          // MEMDIR
  void loadMemdirSymbol(char*);         // MEMDIR
  void loadRegdir(int);                 // REGDIR
  void loadRegind(int);                 // REGIND
  void loadRegindOffLiteral(int, int);  // REGINDOFF
  void loadRegindOffSymbol(int, char*); // REGINDOFF

  StoreType storeType = S_NOTYPE;
  int cur_store_num_op = 0;
  int cur_store_reg_op = 0;
  string cur_store_sym_op = "";
  void processStore(int);
  void storeMemdirLiteral(int);          // MEMDIR
  void storeMemdirSymbol(char*);         // MEMDIR
  void storeRegdir(int);                 // REGDIR
  void storeRegind(int);                 // REGIND
  void storeRegindOffLiteral(int, int);  // REGINDOFF
  void storeRegindOffSymbol(int, char*); // REGINDOFF

public:

  Assembler();
  ~Assembler();

  int assemble(string);
  
  /*
  proverava da li je literal u temp pool-u trenutne sekcije, dodaje ako nije
  */
  void helperLoadBigLiteral(int bigLit);
  
  /*
  proverava da li je simbol u tab_sim i u temp pool-u trenutne sekcije, dodaje ako nije
  */
  void helperLoadBigLiteralSymbol(string);

  bool isLiteralBig(int);

  void writeRelocTables();
  void writeSymbolTable();
  void writeSectionTable();

  void writeToFile(char *);
};

extern Assembler* assembler;

enum OpCodes {

  OpHalt = 0x00000000,
  OpInt  = 0x10000000,

  OpJsrRegdirD = 0x20000000,
  OpJsrMemdirD = 0x21000000,

  OpJmpRegdirD = 0x30000000,
  OpBeqRegdirD = 0x31000000,
  OpBneRegdirD = 0x32000000,
  OpBgtRegdirD = 0x33000000,
  OpJmpMemdirD = 0x38000000,
  OpBeqMemdirD = 0x39000000,
  OpBneMemdirD = 0x3A000000,
  OpBgtMemdirD = 0x3B000000,

  OpXchg = 0x40000000,
  OpAdd  = 0x50000000,
  OpSub  = 0x51000000,
  OpMul  = 0x52000000,
  OpDiv  = 0x53000000,
  OpNot  = 0x60000000,
  OpAnd  = 0x61000000,
  OpOr   = 0x62000000,
  OpXor  = 0x63000000,
  OpShl  = 0x70000000,
  OpShr  = 0x71000000,

  /* store gprS, operandDest */
  OpSt_TwoGprsSumD    = 0x80000000, // mem32[gpr[A]+gpr[B]+D]<=gpr[C];
  OpSt_MemTwoGprsSumD = 0x82000000, // mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
  OpSt_GprD           = 0x81000000, // gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];

  /* load operandSrc, gprD */
  OpCsrrd             = 0x90000000,  // gpr[A]<=csr[B];
  OpLd_GprD           = 0x91000000,  // gpr[A]<=gpr[B]+D;
  OpLd_MemTwoGprsSumD = 0x92000000,  // gpr[A]<=mem32[gpr[B]+gpr[C]+D];
  OpLd_MemGprAddD     = 0x93000000,  // gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
  OpCsrwr             = 0x94000000,  // csr[A]<=gpr[B];
  OpLd_CsrCsrOrD      = 0x95000000,  // csr[A]<=csr[B]|D;
  OpLd_CsrMemSumGprsD = 0x96000000,  // csr[A]<=mem32[gpr[B]+gpr[C]+D];
  OpLd_CsrMemGprAddD  = 0x97000000,  // csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;

  /* push & pop */
  OpPush = 0x81000000,  // d = -4   store: gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
  OpPop  = 0x93000000,  // d = +4   load : gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;

  OpRet  = 0x93000000   // kao pop
};

#endif