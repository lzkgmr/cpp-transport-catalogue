#pragma once

#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include "geo.h"
#include "domain.h"

namespace transport_catalogue {
  class TransportCatalogue {
  public:
    void AddStop(const Stop &stop);

    Stop *FindStop(const std::string &name_of_stop);

    void AddBus(const Bus &bus);

    Bus *FindBus(const std::string &name_of_bus);

    BusInfo GetBusInfo(const std::string &name_of_bus);

    StopInfo GetStopInfo(const std::string &name_of_stop);

    std::unordered_map<Stop *, std::set<std::string_view>> &GetStopsToBusesMap();

    std::deque<Bus> &GetBusesDeque();

    const std::deque<Bus> &GetBusesDequeConst() const;

    void SetDistance(int64_t dist, Stop *from, Stop *to);

    int64_t GetDistance(Stop *from, Stop *to) const;

    const std::deque<Bus> &GetAllBuses();

    std::set<std::string_view> &GetUniqueStops();

    size_t GetStopId(std::string_view stop_name) const;

    std::string GetStopFromId(size_t stop_id) const;

    size_t GetStopsCount() const;

  private:
    struct StopsHasher {
      size_t operator()(std::pair<Stop *, Stop *> elem) const {
        return s_hasher(elem.first) + 37 * s_hasher(elem.second);
      }

    private:
      std::hash<Stop *> s_hasher;
    };

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::set<std::string_view> unique_stops;
    std::unordered_map<std::string_view, Stop *> stopname_to_stop_;
    std::unordered_map<std::string_view, Bus *> busname_to_bus_;
    std::unordered_map<Stop *, std::set<std::string_view>> stopname_to_buses;
    std::unordered_map<std::string_view, bool> isbusroundtrip;
    std::unordered_map<std::pair<Stop *, Stop *>, int64_t, StopsHasher> dist_betw_stops_;
    std::unordered_map<std::string_view, bool> roundtrip_of_buses;
    std::unordered_map<std::string_view, size_t> stop_name_to_id;
    std::unordered_map<size_t, std::string> id_to_stopname;
    size_t stop_count = 0;
  };

}