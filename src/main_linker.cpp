#include "../inc/linker.hpp"

Linker* linker = nullptr;

int main(int argc, char *argv[]) {

  bool hasHex = false;
  string outputFileName = "";
  vector<string> inputFiles;
  vector<Linker::PlaceSection*> places;

  for(int i = 1; i < argc; i++) {
    string argument(argv[i]);

    if(!strcmp(argv[i], "-hex")) {
      hasHex = true;
      continue;
    }

    else if(!strcmp(argv[i], "-o")) {
      if(argv[i + 1] != nullptr && argv[i + 1][0] != '-') {
        outputFileName = argv[i + 1];
        i++;
        continue;
      } 
    }

    else if(argument.find("-place=")!=string::npos) {
      int j = 7;  // na indexu 7 argumenta pocinje ime sekcije
      string sectionName;
      string saddress;

      while(argv[i][j] != '\0') {
        if(argv[i][j] != '@') {
          // citam ime sekcije
          sectionName += argv[i][j++];

        } else {
          // stigli do @ - citam adresu
          j++;
          while(argv[i][j] != '\0') {
            saddress += argv[i][j++];
          }
        }
      }
      // da li je ta sekcija vec navedena u -place=...:
      for(Linker::PlaceSection *p : places) {
        if(p->section == sectionName) {
          cout << "Section already placed!" << endl;
          return -1;
        }
      }
      long address = stol(saddress, nullptr, 16);
      Linker::PlaceSection *ps = new Linker::PlaceSection(address, sectionName);
      places.push_back(ps);
    } 

    else if(argument.find(".o")!=string::npos) {
      bool alreadyThere = false;
      for(string name : inputFiles) {
        if(name == argv[i]) {
          alreadyThere = true;
        }
      }
      if(!alreadyThere) {
        inputFiles.push_back(argv[i]);
      }
    }

    else {
      cout << "Unknown command line option! " << argv[i] << endl;
      return -1;
    }

  } 

  if(inputFiles.empty()) {
    cout << "Input files not listed!" << endl;
    return -1;
  }

  if(!hasHex) {
    cout << "Without the -hex option specified, the linker will not generate an output file." << endl;
    return 0;
  }

	linker = new Linker(hasHex, outputFileName, inputFiles, places);

  int ret = linker->link();

  if(ret == -1) {
    cout << "Linker error" << endl;
    return ret;
  }

	delete linker;

  return 0;
}