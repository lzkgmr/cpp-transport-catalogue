#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

void BusInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
             std::ostream& output);

void StopInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
              std::ostream& output);

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output);
void RunStat(std::istream& in, std::ostream& out, TransportCatalogue& catalogue);
