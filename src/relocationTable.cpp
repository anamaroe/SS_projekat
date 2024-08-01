#include "../inc/relocationTable.hpp"


void RelocationTable::addRelaEntry(RelatableEntry* relaRecord) {
  relocationTable.push_back(relaRecord);
}

RelatableEntry* RelocationTable::getRelaEntry(int index) {
  return relocationTable[index];
}