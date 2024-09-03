#include "../inc/emulator.hpp"


Emulator::~Emulator() {
  if (memory != nullptr) {
    unsigned long memSpace = 1UL << 32;
    munmap(memory, memSpace);
  }
} 

int Emulator::emulate() {
  using EmulatorMethod = int (Emulator::*)();

  EmulatorMethod steps[] = {
    &Emulator::cpuInit,
    &Emulator::readFile,
    &Emulator::executeProgram,
    &Emulator::writeEmulatorOutput
  };

  for (auto step : steps) {
      int ret = (this->*step)();
      if (ret == -1) return ret;
  }
  // cout << "Emulator finished." << endl;
  return 0;
}



/*****************************************************************************************/
//                    EMULATOR METHODS 
/*****************************************************************************************/


int Emulator::cpuInit() {
  unsigned long memorySpace = 1UL << 32;
  this->memory = static_cast<uint8_t*>(mmap(nullptr, memorySpace, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (memory == MAP_FAILED) {
    cout << "Emulator error: mmap failed" << endl;
    return -1;
  }
  for (int i = 0; i < 16; ++i) {
    reg[i] = 0;
    if(i < 3) { csr[i] = 0; }
  }
  reg[PC] = 0x40000000; 
  return 0;
}


int Emulator::readFile() {
  string fileName = "tests/" + this->inputFile; 
  ifstream binaryFile(fileName, std::ios::binary);
  if(!binaryFile) {
    cout << "Can't open file in emulator: " << fileName << endl;
    return -1;
  }
  while (binaryFile.peek() != EOF) {  

    int sectionLength = 0;
    binaryFile.read(reinterpret_cast<char*>(&sectionLength), sizeof(int));

    // gde ima citavih 8B
    int iterations = sectionLength / 8; 
    // ako u zadnjoj turi ima jedna upisana instr. a ne dve (4B)
    int remainingBytes = sectionLength % 8; 

    unsigned int address = 0;

    for (int i = 0; i < iterations; ++i) { 
      
      binaryFile.read(reinterpret_cast<char*>(&address), sizeof(unsigned int));
      uint8_t code[8] = {0};
      binaryFile.read(reinterpret_cast<char*>(code), sizeof(code));
      for (int j = 0; j < 8; ++j) {
        memory[address + j] = code[j];
      }
    }

    if (remainingBytes > 0) {
      binaryFile.read(reinterpret_cast<char*>(&address), sizeof(unsigned int));
      // cout << "jos 4B instr::: ADRESA: " << address << ": ";
      uint8_t code[8] = {0}; // Maksimalno 8 bajtova
      binaryFile.read(reinterpret_cast<char*>(code), remainingBytes);

      for (int j = 0; j < remainingBytes; ++j) { 
        memory[address + j] = code[j];
      }
    }
  }
  binaryFile.close();
  return 0;
}


int Emulator::executeProgram() {

  bool halted = false;

  while(!halted) {
    // cout << "pc: " <<hex<< reg[PC] << endl;
    // cout << "sp: "  << std::setw(8) << std::setfill('0') << std::hex << reg[SP] << endl;

    uint8_t byte1 = memory[(unsigned int)(reg[PC] + 3)];  
    uint8_t byte2 = memory[(unsigned int)(reg[PC] + 2)];
    uint8_t byte3 = memory[(unsigned int)(reg[PC] + 1)];
    uint8_t byte4 = memory[(unsigned int)(reg[PC] + 0)];

    updatePC();

    uint8_t opCode = (byte1 >> 4) & 0xF;
    uint8_t mod    = byte1 & 0xF;
    uint8_t regA   = (byte2 >> 4) & 0xF;
    uint8_t regB   = byte2 & 0xF;
    uint8_t regC   = (byte3 >> 4) & 0xF;

    int displacement = 0x00000FFF & (((byte3 & 0x0F) << 8) | (byte4 & 0xFF));

    if((byte3 >> 3) & 0x01) {
      // broj je negativan
      displacement = 0xFFFFF000 | ((byte3 & 0x0F) << 8) | (byte4 & 0xFF);
    }

    switch(opCode) {
      case 0:
        halted = true;
        break;
      case 1: 
        handler->processInt(this);
        break;
      case 2:
        handler->processCall(this, mod, regA, regB, displacement);
        break;
      case 3:
        handler->processJump(this, mod, regA, regB, regC, displacement);
        break;
      case 4:
        handler->processXchg(this, regB, regC);
        break;
      case 5:
        handler->processArithmet(this, mod, regA, regB, regC);
        break;
      case 6:
        handler->processLogic(this, mod, regA, regB, regC);
        break;
      case 7:
        handler->processShift(this, mod, regA, regB, regC);
        break;
      case 8:
        handler->processStore(this, mod, regA, regB, regC, displacement);
        break;
      case 9:
        handler->processLoad(this, mod, regA, regB, regC, displacement);
        break;
      default:
        cout << "nepoznati opcode";
        handler->processIllegalInstruction(this);
        break;
    }
    reg[0] = 0;
    if(processInterrupts()) {
      return -1;
    }
  }
  return 0;
}


int Emulator::writeEmulatorOutput() {
  cout << "-----------------------------------------------------------------" << endl 
       << "Emulated processor executed halt instruction"                      << endl
       << "Emulated processor state:"                                         << endl;
  for(int i = 0; i < 16; i++) {
    if(i < 10) cout << " ";
    cout << "r" << dec << i << "=0x" << hex << setw(8) << setfill('0') << reg[i] << "\t";
    if(i % 4 == 3) cout << endl;
  }  
  return 0;
}



/*****************************************************************************************/
//                    EMULATOR UTIL 
/*****************************************************************************************/


void Emulator::Handler::processInt(Emulator* emu) {
  emu->push(STATUS);
  emu->push(PC);
  emu->csr[CAUSE] = 4;
  emu->csr[STATUS] = emu->csr[STATUS] & ~0x1;
  emu->reg[PC] = emu->csr[HANDLER];
}
    
void Emulator::Handler::processCall(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, int displ) {
  switch(mod) {
    case 0:
      emu->push(PC);
      emu->reg[PC] = emu->reg[A] + emu->reg[B] + displ;
      break;
    case 1:
      emu->push(PC);
      emu->reg[PC] = emu->read32BitsFromMemory(emu->reg[A] + emu->reg[B] + displ);
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processJump(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C, int displ) {
  switch(mod) {
    case 0:
      emu->reg[PC] = emu->reg[A] + displ;
      break;
    case 1:
      if(emu->reg[B] == emu->reg[C]) {
        emu->reg[PC] = emu->reg[A] + displ;
      }
      break;
    case 2:
      if(emu->reg[B] != emu->reg[C]) {
        emu->reg[PC] = emu->reg[A] + displ;
      }
      break;
    case 3:
      if(emu->reg[B] > emu->reg[C]) {
        emu->reg[PC] = emu->reg[A] + displ;
      }
      break;
    case 8:
      emu->reg[PC] = emu->read32BitsFromMemory(emu->reg[A] + displ);
      break;
    case 9:
      if(emu->reg[B] == emu->reg[C]) {
        emu->reg[PC] = emu->read32BitsFromMemory(emu->reg[A] + displ);
      }
      break;
    case 10:
      if(emu->reg[B] != emu->reg[C]) {
        emu->reg[PC] = emu->read32BitsFromMemory(emu->reg[A] + displ);
      }
      break;
    case 11:
      if(emu->reg[B] > emu->reg[C]) {
        emu->reg[PC] = emu->read32BitsFromMemory(emu->reg[A] + displ);
      }
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processXchg(Emulator* emu, uint8_t B, uint8_t C) {
  int temp = emu->reg[B];
  emu->reg[B] = emu->reg[C];
  emu->reg[C] = temp;
}


void Emulator::Handler::processArithmet(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C) {
  switch(mod) {
    case 0:
      emu->reg[A] = emu->reg[B] + emu->reg[C];
      break;
    case 1:
      emu->reg[A] = emu->reg[B] - emu->reg[C];
      break;
    case 2:
      emu->reg[A] = emu->reg[B] * emu->reg[C];
      break;
    case 3:
      emu->reg[A] = (int)(emu->reg[B] / emu->reg[C]);
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processLogic(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C) {
  switch(mod) {
    case 0:
      emu->reg[A] = ~emu->reg[B];
      break;
    case 1:
      emu->reg[A] = emu->reg[B] & emu->reg[C];
      break;
    case 2:
      emu->reg[A] = emu->reg[B] | emu->reg[C];
      break;
    case 3:
      emu->reg[A] = emu->reg[B] ^ emu->reg[C];
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processShift(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C) {
  switch(mod) {
    case 0:
      emu->reg[A] = emu->reg[B] << emu->reg[C];
      break;
    case 1:
      emu->reg[A] = emu->reg[B] >> emu->reg[C];
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processStore(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C, int displ) {
  int addressFromMemory = 0;
  switch(mod) {
    case 0:
      emu->write32BitsToMemory(emu->reg[A] + emu->reg[B] + displ, emu->reg[C]);
      break;
    case 1:
      // MMMM==0b0001: gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
      emu->reg[A] += displ;
      emu->write32BitsToMemory(emu->reg[A], emu->reg[C]);
      break;
    case 2:
      // MMMM==0b0010: mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
      addressFromMemory = emu->read32BitsFromMemory(emu->reg[A] + emu->reg[B] + displ);
      emu->write32BitsToMemory(addressFromMemory, emu->reg[C]);
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processLoad(Emulator* emu, uint8_t mod, uint8_t A, uint8_t B, uint8_t C, int displ) {
  int addressFromMemory = 0;
  switch(mod) {
    case 0: 
      emu->reg[A] = emu->csr[B];
      break;
    case 1:
      emu->reg[A] = emu->reg[B] + displ;
      break;
    case 2:
      emu->reg[A] = emu->read32BitsFromMemory(emu->reg[B] + emu->reg[C] + displ);
      break;
    case 3:
      emu->reg[A] = emu->read32BitsFromMemory(emu->reg[B]);
      emu->reg[B] += displ;
      break;
    case 4:
      emu->csr[A] = emu->reg[B];
      break;
    case 5:
      emu->csr[A] = emu->csr[B] | displ;
      break;
    case 6:
      emu->csr[A] = emu->read32BitsFromMemory(emu->reg[B] + emu->reg[C] + displ);
      break;
    case 7:
      emu->csr[A] = emu->read32BitsFromMemory(emu->reg[B]);
      emu->reg[B] += displ;
      break;
    default:
      processIllegalInstruction(emu);
      break;
  }
}

void Emulator::Handler::processIllegalInstruction(Emulator* emu) {
  emu->csr[CAUSE] = 1;
}

void Emulator::push(int r) {
  reg[SP] -= 0x00000004;
  int value;
  switch(r) {
    case STATUS:
      value = csr[STATUS];
      break;
    case PC:
      value = reg[PC];
  }
  memory[(unsigned int)(reg[SP] + 3)] = (value >> 24) & 0xFF;   
  memory[(unsigned int)(reg[SP] + 2)] = (value >> 16) & 0xFF;    
  memory[(unsigned int)(reg[SP] + 1)] = (value >> 8)  & 0xFF;    
  memory[(unsigned int)(reg[SP] + 0)] = (value >> 0)  & 0xFF;   
}

int Emulator::pop() {
  uint8_t byte1 = memory[(unsigned int)(reg[SP] + 3)]; 
  uint8_t byte2 = memory[(unsigned int)(reg[SP] + 2)];
  uint8_t byte3 = memory[(unsigned int)(reg[SP] + 1)];
  uint8_t byte4 = memory[(unsigned int)(reg[SP] + 0)]; 
  reg[SP] += 4;
  return (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;;
}

int Emulator::read32BitsFromMemory(int address) { 
  uint8_t byte1 = memory[(unsigned int)(address + 3)]; // najvisi bajt
  uint8_t byte2 = memory[(unsigned int)(address + 2)];
  uint8_t byte3 = memory[(unsigned int)(address + 1)];
  uint8_t byte4 = memory[(unsigned int)(address + 0)]; // najnizi bajt
  return (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
}

void Emulator::write32BitsToMemory(int address, int what) {
  memory[(unsigned int)(address + 3)] = (what >> 24) & 0xFF;
  memory[(unsigned int)(address + 2)] = (what >> 16) & 0xFF;
  memory[(unsigned int)(address + 1)] = (what >> 8)  & 0xFF;
  memory[(unsigned int)(address + 0)] = (what >> 0)  & 0xFF;
}

int Emulator::processInterrupts() {
  if(csr[CAUSE] == 4) {
    // softverski prekid - obradice se
    return 0;
  }
  if(csr[CAUSE] == 1) {
    cout << "neisparavna instrukcija" << endl;
    return -1;
  }
  return 0;
}