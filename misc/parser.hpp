/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_MISC_PARSER_HPP_INCLUDED
# define YY_YY_MISC_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    NUM = 258,
    REG = 259,
    CSR = 260,
    SYMBOL = 261,
    LABEL = 262,
    ENDL = 263,
    GLOBAL = 264,
    EXTERN = 265,
    SECTION = 266,
    WORD = 267,
    SKIP = 268,
    END = 269,
    HALT = 270,
    INTERRUPT = 271,
    IRET = 272,
    CALL = 273,
    RET = 274,
    JMP = 275,
    BEQ = 276,
    BNE = 277,
    BGT = 278,
    PUSH = 279,
    POP = 280,
    XCHG = 281,
    ADD = 282,
    SUB = 283,
    MUL = 284,
    DIV = 285,
    NOT = 286,
    AND = 287,
    OR = 288,
    XOR = 289,
    SHL = 290,
    SHR = 291,
    LD = 292,
    ST = 293,
    CSRRD = 294,
    CSRWR = 295
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 20 "misc/parser.y"

  int ival;
  char *sval;

#line 103 "misc/parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MISC_PARSER_HPP_INCLUDED  */
