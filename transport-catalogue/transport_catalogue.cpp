#include "transport_catalogue.h"

void TransportCatalogue::AddStop(std::string&& name, Coordinates coordinates) {
  Stop stop;
  stop.name = std::move(name);
  stop.cordinates = coordinates;
  stops_.push_back(stop);
  stopname_to_stop[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddBus(std::string &&name, const std::vector<std::string_view> &stops) {
  Bus bus;
  bus.name = std::move(name);
  std::vector<const Stop *> stops_ptr;
  stops_ptr.reserve(stops.size());
  int amount = 0;
  for (int i = 0; i < stops.size(); ++i) {
    if (stopname_to_stop.count(stops[i])) {
      ++amount;
      stops_ptr.push_back(stopname_to_stop.at(stops[i]));
      if (i != stops.size() - 1) {
        bus.geo_distance += ComputeDistance(stopname_to_stop.at(stops[i])->cordinates,
                                            stopname_to_stop.at(stops[i + 1])->cordinates);
      }
      if (count(stops_ptr.begin(), stops_ptr.end(), stopname_to_stop.at(stops[i])) == 1) {
        bus.unique_stops++;
      }
    }
  }
  stops_ptr.resize(amount);
  bus.stops = std::move(stops_ptr);
  for (int i = 0; i < bus.stops.size() - 1; ++i) {
    if (stops_to_distances.count({bus.stops[i], bus.stops[i + 1]})) {
      bus.route_length += stops_to_distances.at({bus.stops[i], bus.stops[i + 1]});
    } else if (stops_to_distances.count({bus.stops[i + 1], bus.stops[i]})) {
      bus.route_length += stops_to_distances.at({bus.stops[i + 1], bus.stops[i]});
    }
  }
  bus.curvature = bus.route_length / bus.geo_distance;
  buses_.push_back(bus);
  busname_to_bus[buses_.back().name] = &buses_.back();
}

Bus TransportCatalogue::FindBus(std::string_view name) const {
  Bus bus = {};
  if (busname_to_bus.count(name)) {
    return *(busname_to_bus.at(name));
  }
  return bus;
}

bool TransportCatalogue::FindStop(std::string_view name) const {
  return stopname_to_stop.count(name);
}

std::set<std::string> TransportCatalogue::GetBusesWithStop(std::string_view name) const {
  std::set<std::string> buses;
  assert(stopname_to_stop.count(name));
  for (const Bus& bus: buses_) {
    if (std::count(bus.stops.begin(), bus.stops.end(), stopname_to_stop.at(name))) {
      buses.insert(bus.name);
    }
  }
  return buses;
}

void TransportCatalogue::SetDistances(std::string_view stop, const std::unordered_map<std::string, int>& distances) {
  if (stopname_to_stop.count(stop) == 0) return;
  auto stop_ptr = stopname_to_stop.at(stop);
  if (distances.empty()) {
    return;
  }
  for (const auto& [stop_name, num]: distances) {
    SetDistance(stop_ptr, stop_name, num);
  }
}

void TransportCatalogue::SetDistance(const Stop* stop_from, std::string_view stop_to, int distance) {
  if (stopname_to_stop.count(stop_to)) {
    std::pair<const Stop*, const Stop*> p(stop_from, stopname_to_stop.at(stop_to));
    stops_to_distances[p] = distance;
  }
}

size_t TransportCatalogue::StrHasher::operator()(const std::string_view &name) const {
  std::string id(name);
  return hasher_(id);
}

size_t TransportCatalogue::PtrHasher::operator()(std::pair<const Stop *, const Stop *> p) const {
  return hasher_(reinterpret_cast<const void *>(100500 * std::hash<const void *>{}(p.first) +
                                                42 * std::hash<const void *>{}(p.second)));
}

size_t TransportCatalogue::StopHasher::operator()(const Stop *stop) const {
  return hasher_(stop);
}
