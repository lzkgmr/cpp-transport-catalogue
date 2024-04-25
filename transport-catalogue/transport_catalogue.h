#pragma once

#include <deque>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <functional>
#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include <map>
#include <cassert>

#include "geo.h"

struct Bus;

struct Stop {
  std::string name;
  Coordinates cordinates;
};

struct Bus {
  std::string name;
  std::vector<const Stop*> stops;
  int unique_stops = 0;
  double route_length = 0;
  double geo_distance = 0;
  double curvature = 0;
};

class TransportCatalogue {
public:
  void AddStop(std::string&& name, Coordinates coordinates);

  bool FindStop(std::string_view name) const;

  std::set<std::string> GetBusesWithStop(std::string_view name) const;

  void AddBus(std::string&& name, const std::vector<std::string_view>& stops);

  Bus FindBus(std::string_view name) const;

  void SetDistances(std::string_view stop, const std::unordered_map<std::string, int>& distances);

private:
  void SetBusLength(Bus* bus);

  void SetDistance(const Stop* stop_from, std::string_view stop_to, int distance);

  class StrHasher {
  public:
    size_t operator()(const std::string_view& name) const;
  private:
    std::hash<std::string> hasher_;
  };

  class PtrHasher {
  public:
    size_t operator()(std::pair<const Stop*, const Stop*>) const;
  private:
    std::hash<const void*> hasher_;
  };

  class StopHasher {
  public:
    size_t operator()(const Stop* stop) const;
  private:
    std::hash<const void*> hasher_;
  };

  std::deque<Stop> stops_;
  std::deque<Bus> buses_;
  std::unordered_map<std::string_view, const Stop*, StrHasher> stopname_to_stop;
  std::unordered_map<std::string_view, const Bus*, StrHasher> busname_to_bus;
  std::unordered_map<std::pair<const Stop*, const Stop*>, int, PtrHasher> stops_to_distances;
};
