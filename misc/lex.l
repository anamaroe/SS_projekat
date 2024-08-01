%{
  #include "./parser.hpp"
  #include <iostream>
  #include <sstream>

  using namespace std;
  void yyerror(const char*);

  int line_num;

  char *getLabelName(char* yytext) {
    char *semi = strchr(yytext, ':');
    *semi = '\0';
    char *labelName = strdup(yytext);
    *semi = ':';
    return labelName;
  }
%}

%option outfile="misc/lexer.cpp" header-file="misc/lexer.hpp"

%option noyywrap

%%
[\t]                    {}
[\t]*#[^\n]+            { /* comment */ }
[" "]*                  {}

\[                      { return '['; }
\]                      { return ']'; }
\,                      { return ','; }
\$                      { return '$'; }
\+                      { return '+'; }

"%r0"                   { yylval.ival = 0; return REG; }
"%r1"                   { yylval.ival = 1; return REG; }
"%r2"                   { yylval.ival = 2; return REG; }
"%r3"                   { yylval.ival = 3; return REG; }
"%r4"                   { yylval.ival = 4; return REG; }
"%r5"                   { yylval.ival = 5; return REG; }
"%r6"                   { yylval.ival = 6; return REG; }
"%r7"                   { yylval.ival = 7; return REG; }
"%r8"                   { yylval.ival = 8; return REG; }
"%r9"                   { yylval.ival = 9; return REG; }
"%r10"                  { yylval.ival = 10; return REG; }
"%r11"                  { yylval.ival = 11; return REG; }
"%r12"                  { yylval.ival = 12; return REG; }
"%r13"                  { yylval.ival = 13; return REG; }
"%r14"                  { yylval.ival = 14; return REG; }
"%r15"                  { yylval.ival = 15; return REG; }
"%sp"                   { yylval.ival = 14; return REG; }
"%pc"                   { yylval.ival = 15; return REG; }

"%status"               { yylval.ival = 0; return CSR; }
"%handler"              { yylval.ival = 1; return CSR; }
"%cause"                { yylval.ival = 2; return CSR; }

".global"               { return GLOBAL; }
".extern"               { return EXTERN; }
".section"              { return SECTION; }
".word"                 { return WORD; }
".skip"                 { return SKIP; }
".end"(.|\n)*           { return END; }

"halt"                  { return HALT; }
"int"                   { return INTERRUPT; }
"iret"                  { return IRET; }
"call"                  { return CALL; }
"ret"                   { return RET; }
"jmp"                   { return JMP; }
"beq"                   { return BEQ; }
"bne"                   { return BNE; }
"bgt"                   { return BGT; }
"push"                  { return PUSH; }
"pop"                   { return POP; }
"xchg"                  { return XCHG; }
"add"                   { return ADD; }
"sub"                   { return SUB; }
"mul"                   { return MUL; }
"div"                   { return DIV; }
"not"                   { return NOT; }
"and"                   { return AND; }
"or"                    { return OR; }
"xor"                   { return XOR; }
"shl"                   { return SHL; }
"shr"                   { return SHR; }
"ld"                    { return LD; }
"st"                    { return ST; }
"csrrd"                 { return CSRRD; }
"csrwr"                 { return CSRWR; }

[0-9]                   { yylval.ival = atoi(yytext); return NUM; }
[1-9][0-9]+             { yylval.ival = atoi(yytext); return NUM; }
0[0-7]+                 { yylval.ival = strtol(yytext, NULL, 8);  return NUM; }
0[xX][0-9a-fA-F]+       { yylval.ival = strtol(yytext, NULL, 16); return NUM; }

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.sval = strdup(yytext); return SYMBOL; }
[a-zA-Z_][a-zA-Z0-9_]*: { yylval.sval = getLabelName(yytext); return LABEL; }

[\t]*[\n]               { line_num++; return ENDL; }

%%

void yyerror(const char *s) {
  cout << "Error! Message: " << s << endl;
  exit(-1);
}