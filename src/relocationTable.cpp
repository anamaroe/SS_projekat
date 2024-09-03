#include "../inc/relocationTable.hpp"


void RelocationTable::addRelaEntry(RelatableEntry* relaRecord) {
  relocationTable.push_back(relaRecord);
}

unsigned int RelatableEntry::getOffset() { return offset; }

int RelatableEntry::getSymbolRefNum() { return symbolRefNum; }

int RelatableEntry::getAddend() { return addend; }

RelocationType RelatableEntry::getType() { return type; }