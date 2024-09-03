#ifndef elf_types_hpp
#define elf_types_hpp

typedef unsigned int Elf32_Addr;
typedef unsigned int Elf32_Word;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Off;
typedef unsigned int Elf32_Xword;
typedef signed int Elf32_Sxword;

enum RelocationType {
  // RELA_ABS_32, // 32 bita    -- samo upisujem vrednost simbola
  // RELA_ABS_12,
  // RELA_PC      // PC relativno, 12 bita    -- upisujem bazenLocation - locationCounter - 4
  R_X86_64_32S,
  R_X86_64_12S,
  R_X86_64_PC32
};

enum SectionType {
  SHT_NULL,       /* Section header table entry unused */
  SHT_PROGBITS,   /* Program data */
  SHT_SYMTAB,     /* Symbol table */
  SHT_STRTAB,     /* String table */
  SHT_RELA,       /* Relocation entries with addends */
  SHT_DYNAMIC = 6,/* Dynamic linking information */
  SHT_NOTE,       /* Notes */
  SHT_NOBITS,     /* Program space with no data (bss) */
  SHT_REL         /* Relocation entries, no addends */
};


#endif