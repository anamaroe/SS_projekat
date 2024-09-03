#include "../inc/emulator.hpp"

Emulator* emulator = nullptr;

int main(int argc, char *argv[]) {
  if(argc != 2) {
    cout << "Emulator error: Incorrect command line arguments";
  }
  
  emulator = new Emulator(argv[1]);

  emulator->emulate();

  delete emulator;
}