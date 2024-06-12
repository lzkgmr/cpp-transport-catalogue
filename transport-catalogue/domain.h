#pragma once

#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "geo.h"

namespace transport_catalogue {

  struct InputBusData {
    std::string name;
    std::vector <std::string> stops;
    bool is_roundtrip;
  };

  struct Stop {
    std::string name;
    geo::Coordinates coordinates;
    std::unordered_map <std::string, int64_t> stops_to_dists;
  };

  struct Bus {
    std::string name;
    std::vector<Stop *> stops;
    bool is_roundtrip;
  };

  struct BusInfo {
    size_t numb_of_stops;
    size_t numb_of_unique_stops;
    double length;
    int64_t true_length;
  };

  struct StopInfo {
    std::set <std::string_view> passing_buses;
  };
}