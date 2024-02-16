#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"
// не могу придумать, как должны называться пространства имен и где они должны быть. дайте совет пожалуйста
void BusInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
             std::ostream& output);

void StopInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
              std::ostream& output);

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output);
