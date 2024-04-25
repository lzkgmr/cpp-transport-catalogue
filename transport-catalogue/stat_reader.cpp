#include "stat_reader.h"

void BusInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
         std::ostream& output) {
    Bus bus = transport_catalogue.FindBus(request);
    if (bus.name.empty()) {
        output << "Bus " << request << ": not found" << std::endl;
    } else {
        output << "Bus " << request << ": " << bus.stops.size() << " stops on route, " << bus.unique_stops << " unique stops, "
        << bus.route_length << " route length, " << bus.curvature << " curvature" << std::endl;
    }
}

void StopInfo(const TransportCatalogue& transport_catalogue, std::string_view request,
              std::ostream& output) {
    if (!transport_catalogue.FindStop(request)) {
        output << "Stop " << request << ": not found" << std::endl;
        return;
    }
    std::set<std::string> buses;
    transport_catalogue.FindStopInfo(request, buses);
    if (buses.empty()) {
        output << "Stop " << request << ": no buses" << std::endl;
    } else {
        output << "Stop " << request << ": buses ";
        auto it = buses.begin();
        while (it != buses.end()) {
            output << *it;
            if (++it != buses.end()) output << " ";
            else output << std::endl;
        }
    }
}

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    std::string_view request_str(request.substr(request.find_first_of(" ") + 1));
    if (request[0] == 'B') {
        BusInfo(transport_catalogue, request_str, output);
    } else {
        StopInfo(transport_catalogue, request_str, output);
    };
}

void RunStat(std::istream& in, std::ostream& out, TransportCatalogue& catalogue) {
  int stat_request_count;
  in >> stat_request_count >> std::ws;
  for (int i = 0; i < stat_request_count; ++i) {
    std::string line;
    getline(in, line);
    ParseAndPrintStat(catalogue, line, out);
  }
}
