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

struct BusStat {
  std::string_view name;
  size_t stops_on_route;
  int unique_stops;
  double route_length;
  double curvature;
};

class TransportCatalogue {
public:
  void AddStop(const std::string& name, Coordinates coordinates);

  const Stop* FindStop(std::string_view name) const;

  std::set<std::string> GetBusesWithStop(std::string_view name) const;

  void AddBus(std::string&& name, const std::vector<std::string_view>& stops);

  Bus FindBus(std::string_view name) const;

  BusStat GetBusStat(std::string_view name) const;

  void SetDistance(std::string_view stop_from, std::string_view stop_to, int distance);

private:
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
