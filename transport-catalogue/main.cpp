#include "json_reader.h"
#include <iostream>

int main() {
  using namespace transport_catalogue;
  TransportCatalogue catal;
  ProcessRequest(std::cin, std::cout, catal);
}