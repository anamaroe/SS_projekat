%{
  #include <iostream>
  #include <stdlib.h>
  #include "../inc/assembler.hpp"

  using namespace std;

  extern int yylex(void);
  extern int yyparse();
  extern FILE *yyin;
  extern void yyerror(const char*);

  extern Assembler* assembler;
%}

/* Output file names */
%output "misc/parser.cpp"
%defines "misc/parser.hpp"

%union {
  int ival;
  char *sval;
}

%token <ival> NUM
%token <ival> REG
%token <ival> CSR
%token <sval> SYMBOL
%token <sval> LABEL
%token <sval> ENDL

%token GLOBAL
%token EXTERN
%token SECTION
%token WORD
%token SKIP
%token END
%token HALT
%token INTERRUPT
%token IRET
%token CALL
%token RET
%token JMP
%token BEQ
%token BNE
%token BGT
%token PUSH
%token POP
%token XCHG
%token ADD
%token SUB
%token MUL
%token DIV
%token NOT
%token AND
%token OR
%token XOR
%token SHL
%token SHR
%token LD
%token ST
%token CSRRD
%token CSRWR

%%
file:
  program END { assembler->processEnd(); }
  ;

program:
  program line 
  |
  line
  ;

line:
  LABEL ENDL { assembler->processLabelDefinition((char*)$1); free($1); } 
  |
  HALT ENDL { assembler->processHalt(); }
  |
  INTERRUPT ENDL { assembler->processInt(); }
  |
  IRET ENDL { assembler->processIret(); } 
  |
  CALL call_operand ENDL 
  |
  RET ENDL { assembler->processRet(); }
  |
  JMP jmp_operand ENDL  
  |
  BEQ REG ',' REG ',' branch_operand { assembler->processBeq($2, $4); } 
  |
  BNE REG ',' REG ',' branch_operand { assembler->processBne($2, $4); } 
  |
  BGT REG ',' REG ',' branch_operand { assembler->processBgt($2, $4); } 
  |
  PUSH REG ENDL { assembler->processStack(Assembler::PUSH, $2); }
  |
  POP REG ENDL { assembler->processStack(Assembler::POP, $2); }
  |
  XCHG REG ',' REG ENDL { assembler->processXchg($2, $4); }
  |
  ADD REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::ADD, $2, $4); }
  |
  SUB REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::SUB, $2, $4); }
  |
  MUL REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::MUL, $2, $4); }
  |
  DIV REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::DIV, $2, $4); }
  |
  NOT REG ENDL         { assembler->processArithmeticLogic(Assembler::NOT, $2, $2); }
  |
  AND REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::AND, $2, $4); }
  |
  OR REG ',' REG ENDL  { assembler->processArithmeticLogic(Assembler::OR, $2, $4); }
  |
  XOR REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::XOR, $2, $4); }
  |
  SHL REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::SHL, $2, $4); }
  |
  SHR REG ',' REG ENDL { assembler->processArithmeticLogic(Assembler::SHR, $2, $4); }
  |
  LD ld_operand ',' REG ENDL { assembler->processLoad($4); }
  |
  ST REG ',' st_operand ENDL{ assembler->processStore($2); }
  |
  CSRRD CSR ',' REG ENDL { assembler->processCsr(Assembler::READ, $2, $4); }
  | 
  CSRWR REG ',' CSR ENDL { assembler->processCsr(Assembler::WRITE, $2, $4); }
  |
  GLOBAL global_list ENDL 
  |
  EXTERN extern_list ENDL 
  |
  SECTION SYMBOL ENDL { assembler->processSection((char*)$2); free($2); }
  |
  WORD word_list ENDL 
  |
  SKIP NUM ENDL { assembler->processSkip((int)$2); }
  |
  ENDL
  ;

global_list:
  global_list ',' global_symbol
  |
  global_symbol
  ;

global_symbol:
  SYMBOL { assembler->processGlobal((char*)$1); free($1); }
  ;
  
extern_list:
  extern_list ',' extern_symbol
  |
  extern_symbol
  ;

extern_symbol:
  SYMBOL { assembler->processExtern((char*)$1); free($1); }
  ;

word_list:
  word_list ',' word_symbol
  |
  word_symbol
  ;

word_symbol:
  SYMBOL { assembler->processWordSymbol((char*)$1); free($1); }
  | 
  NUM { assembler->processWordLiteral($1); }
  ;

call_operand:
  SYMBOL { assembler->processCallSymbol((char*)$1); free($1); }
  | 
  NUM { assembler->processCallLiteral($1); }
  ;

jmp_operand:
  SYMBOL { assembler->processJmpSymbol((char*)$1); free($1); }
  |
  NUM { assembler->processJmpLiteral($1); }
  ;

branch_operand:
  SYMBOL { assembler->processBranchSymbol((char*)$1); free($1); } // treba da vrati vrednost: $$ = mk_nesto(); ???
  |
  NUM { assembler->processBranchLiteral((int)$1); }
  ;

ld_operand:
  '$' NUM { assembler->loadImmedLiteral($2); } // IMMED
  |
  '$' SYMBOL { assembler->loadImmedSymbol((char*)$2); free($2); } // IMMED
  |
  NUM { assembler->loadMemdirLiteral($1); } // MEMDIR
  |
  SYMBOL { assembler->loadMemdirSymbol($1); free($1); } // MEMDIR
  |
  REG { assembler->loadRegdir($1); } // REGDIR
  |
  '[' REG ']' { assembler->loadRegind($2); } // REGIND
  |
  '[' REG '+' NUM ']' { assembler->loadRegindOffLiteral($2, $4);} // REGINDOFF
  |
  '[' REG '+' SYMBOL ']' { assembler->loadRegindOffSymbol($2, (char*)$4); free($4); } // REGINDOFF
  ;

st_operand:
  NUM { assembler->storeMemdirLiteral($1); } // MEMDIR
  |
  SYMBOL { assembler->storeMemdirSymbol((char*)$1); free($1); } // MEMDIR
  |
  REG { assembler->storeRegdir($1); } // REGDIR
  |
  '[' REG ']' { assembler->storeRegind($2); } // REGIND
  |
  '[' REG '+' NUM ']' { assembler->storeRegindOffLiteral($2, $4); } // REGINDOFF
  |
  '[' REG '+' SYMBOL ']' { assembler->storeRegindOffSymbol($2, (char*)$4); free($4); } // REGINDOFF
  ;


%%