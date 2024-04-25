  #include <iostream>

  #include "input_reader.h"
  #include "stat_reader.h"

  using namespace std;

  int main() {
    TransportCatalogue catalogue;
    RunInput(std::cin, catalogue);
    RunStat(std::cin, std::cout, catalogue);
  }
