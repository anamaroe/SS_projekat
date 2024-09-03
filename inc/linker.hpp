#ifndef linker_hpp
#define linker_hpp

#include "symbolTable.hpp"
#include "sectionTable.hpp"
#include <iostream>
#include <fstream>  
#include <vector>
#include <string.h>  
using namespace std;


class Linker {

public:
  
  class LinkerSymbol {
  public:
    typedef enum { SYMBOL, SECTION } SymbolType;
    string name;
    int num;
    int section;
    int global;
    SymbolType type; // if num == sectionNum ---> sekcija
    unsigned int value;
    int defined;
    int str_tab_index;
    LinkerSymbol(int n, int i, short sec, int g, int d, unsigned int v, string nameSym) : 
      num(n), str_tab_index(i), section(sec), global(g), defined(d), value(v), name(nameSym) {}
  }; 

  class LinkerRelocation {
  public:
    int offset, addend, symbolNum;
    string symbolName;  
    RelocationType type;
    LinkerRelocation(int o, int a, int s, RelocationType t) : offset(o), addend(a), symbolNum(s), type(t) {}
  };

  class LinkerSection {
  public:
    typedef enum { PROGBITS, RELA } SectionType;
    string name;
    int num;
    int length;
    unsigned int base = 0;      
    uint8_t *code;  // kod rela sekcije prazno
    SectionType type;
    vector<LinkerRelocation*> relocations;     // samo kod sekcija tipa RELA 
    LinkerSection* relaSection = nullptr;         // kod PROGBITS sekcija, njihove RELA sekcije
    LinkerSection(int l, string n, SectionType t) : length(l), name(n), type(t) {} 
  };

  class LinkerSymbolTable { 
  public: 
    vector<LinkerSymbol*> symbolTable; 
  };

  class LinkerSectionTable {  
  public: 
    vector<LinkerSection*> sectionTable; 
    LinkerSection* getSectionByName(string name);
  };

  class LinkerInputFile {
  public:
    LinkerSymbolTable *symbolTable;
    LinkerSectionTable *sectionTable;
    string filename;
    LinkerInputFile(string n) : filename(n) {
      symbolTable = new LinkerSymbolTable();
      sectionTable = new LinkerSectionTable();
    }
  };

  class PlaceSection { 
  public:
    long place;
    string section;
    int length = 0;
    vector<LinkerSection*> sectionsWSameName; // koristim pri smestanju sekcija; da bi istoimene sekcije bile odmah jedna iza druge
    PlaceSection(long p, string s) : place(p), section(s) {}
  }; 


private:
  bool hasHex;

  string outputFileName;
  
  vector<string> inputFiles;
  
  vector<PlaceSection*> sectionPlaces; //  za svaku sekciju po jedan ovaj; kljuc je ime sekcije;


  PlaceSection* getPlacedSection(string); // dohvatam iz section places - moze biti vise sekcija tog naziva? da, u vektoru

  void useRelocation(LinkerRelocation *rela, LinkerSection *ls, LinkerSymbolTable *table);

  vector<LinkerInputFile*> files;  

  
public:
  Linker(bool hex, string outFile, vector<string> inFiles, vector<PlaceSection*> sectionPlaces);

  int readInputFiles();       

  int mapSections();       

  int checkForUndefAndMultipleDefs();
  
  int resolveSymbols();     
  
  int resolveRelocations(); 

  int writeLinkerOutputFiles();

  int link();

  ~Linker();

};



extern Linker *linker;

#endif