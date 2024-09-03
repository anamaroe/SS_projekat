#ifndef emulator_hpp
#define emulator_hpp

#include <iostream>
#include <fstream>  
#include <vector>
#include <string.h>  
#include <sys/mman.h>
#include <iomanip>
using namespace std;
 


class Emulator {

  string inputFile;

  int reg[16];

  int csr[3];

  uint8_t* memory = nullptr;

  class Handler {
  public:
    void processInt(Emulator*);
    void processCall(Emulator*, uint8_t, uint8_t, uint8_t, int);
    void processJump(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t, int);
    void processXchg(Emulator*, uint8_t, uint8_t);
    void processArithmet(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t);
    void processLogic(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t);
    void processShift(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t);
    void processStore(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t, int);
    void processLoad(Emulator*, uint8_t, uint8_t, uint8_t, uint8_t, int);
    void processIllegalInstruction(Emulator*);
  };

  Handler* handler;

public:
  static constexpr int SP = 14;     
  
  static constexpr int PC = 15;    
  
  static constexpr int STATUS = 0;   
  
  static constexpr int HANDLER = 1; 
  
  static constexpr int CAUSE = 2;    
  
  Emulator(string file) : inputFile(file) { handler = new Handler(); }

  int getPC() { return reg[PC]; }

  void updatePC() { reg[PC] += 4; }

  void push(int);

  int pop();

  int getSP() { return reg[SP]; }

  void incSP() { reg[SP] += 4; }

  void decSP() { reg[SP] -= 4; }

  int read32BitsFromMemory(int address);

  void write32BitsToMemory(int address, int what);

  int processInterrupts();

  int cpuInit();  

  int readFile(); 

  int executeProgram();

  int writeEmulatorOutput();

  int emulate();

  ~Emulator();

};

extern Emulator* emulator;

#endif