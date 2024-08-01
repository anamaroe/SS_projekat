#ifndef elf_types_hpp
#define elf_types_hpp

typedef unsigned int Elf32_Addr;
typedef unsigned int Elf32_Word;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Off;
typedef unsigned int Elf32_Xword;
typedef signed int Elf32_Sxword;


enum FlinkType { ABS, PCREL };

enum SymbolType { 
  STT_NOTYPE,   // unspecified type
  STT_OBJECT,   // data object
  STT_FUNC,     // code object
  STT_SECTION,  // section
  STT_COMMON = 5// common data object
};

enum SymbolVisibility {
  STV_DEFAULT,  // default
  STV_INTERNAL, // processor specific hidden class
  STV_HIDDEN,   // sym unavailable in other modules
  STV_PROTECTED // not exported
};

enum RelaType {
  R_X86_64_PC32 = 2,  // pc relative 32 bit signed
  R_X86_64_PLT32 = 4, // 32 bit plt address
  R_X86_64_32S = 11   // direct 32 bit sign extended
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

enum SectionFlags {
  SHF_WRITE = (1 << 0),      /* Writable */
  SHF_ALLOC = (1 << 1),      /* Occupies memory during execution */
  SHF_EXECINSTR = (1 << 2),  /* Executable */
  SHF_MERGE = (1 << 4),      /* Might be merged */
  SHF_STRINGS = (1 << 5),    /* Contains nul-terminated strings */
  SHF_INFO_LINK = (1 << 6),  /* 'sh_info' contains SHT index */
  SHF_GROUP = (1 << 9),      /* Section is member of a group. */
  SHF_TLS = (1 << 10),       /* Section hold thread-local data. */
  SHF_MASKOS = 0x0ff00000,   /* OS-specific. */
  SHF_MASKPROC = 0xf0000000  /* Processor-specific */
};


#endif