#include "json_reader.h"
#include "router.h"
#include <sstream>

namespace transport_catalogue {

  InputBusData ParseBusInfo(const json::Dict& dict_bus_info) {
    std::vector<std::string> stops;
    bool is_roundtrip = true;
    for (auto &memb: dict_bus_info.at("stops").AsArray()) {
      stops.push_back(memb.AsString());
    }
    if (!dict_bus_info.at("is_roundtrip").AsBool()) {
      is_roundtrip = false;
      std::vector<std::string> temp_vect = stops;
      temp_vect.pop_back(); // обрезаем последний элемент, чтобы автобус поехал на предпосл остановку
      std::reverse(temp_vect.begin(), temp_vect.end());
      for (auto &memb: temp_vect) {
        stops.push_back(move(memb));
      }
    }

    return {dict_bus_info.at("name").AsString(), stops, is_roundtrip};
  }

  Stop ParseStopInfo(const json::Dict& dict_stop_info) {
    std::unordered_map<std::string, int64_t> stops_to_dists;

    if (dict_stop_info.count("road_distances")) {
      for (auto [stop, dist]: dict_stop_info.at("road_distances").AsMap()) {

        stops_to_dists.insert({stop, dist.AsInt()});
      }
    }
    geo::Coordinates coords = {dict_stop_info.at("latitude").AsDouble(), dict_stop_info.at("longitude").AsDouble()};

    return {dict_stop_info.at("name").AsString(), coords, stops_to_dists};
  }

  void ProcessRequest(std::istream &input, std::ostream &output, TransportCatalogue &catalogue) {

    auto doc = json::Load(input);
    json::Dict result_dict = doc.GetRoot().AsMap();

    json::Array base_req = result_dict.at("base_requests").AsArray();
    json::Array stat_req = result_dict.at("stat_requests").AsArray();
    json::Node rander_sett = result_dict.at("render_settings");
    json::Node routing_properties = result_dict.at("routing_settings");
    SvgInfo svg_properties = ParsePropLine(rander_sett);
    ParseAndExecuteRequests(base_req, catalogue);
    ExecuteRequests(stat_req, output, catalogue, svg_properties, routing_properties);
  }

  void ParseAndExecuteRequests(const json::Array& base_req, TransportCatalogue &catalogue) {
    Requests requests;
    // Обработка заполнения базы
    for (auto &node: base_req) {
      if (node.AsMap().at("type").AsString() == "Stop") {
        requests.stops_requests.push_back(node);
        catalogue.AddStop(ParseStopInfo(node.AsMap()));
      } else {
        requests.bus_requests.push_back(node);
      }
    }

    for (auto &stop_node: requests.stops_requests) {
      auto stop_info = ParseStopInfo(std::move(stop_node).AsMap());

      if (stop_info.stops_to_dists.size() != 0) {
        for (auto &pair: stop_info.stops_to_dists) {
          catalogue.SetDistance(pair.second, catalogue.FindStop(stop_info.name), catalogue.FindStop(pair.first));
        }
      }
    }

    for (auto &bus_node: requests.bus_requests) {
      std::vector<Stop *> stops;
      auto bus_info = ParseBusInfo(std::move(bus_node).AsMap());
      for (auto &stop: bus_info.stops) {
        stops.push_back(catalogue.FindStop(stop));
      }
      catalogue.AddBus({bus_info.name, stops, bus_info.is_roundtrip});
      for (auto stop: stops) {
        catalogue.GetStopsToBusesMap()[stop].insert(std::string_view{catalogue.GetBusesDeque().back().name});
      }
    }
  }

  void
  ExecuteRequests(const json::Array &stat_req, std::ostream &output, TransportCatalogue &catalogue, SvgInfo &properties,
                  const json::Node& route_prop) {
    // Создание роутера 1 раз, чтобы потом к нему обращаться
    auto settings = route_prop.AsMap();
    const auto speed = settings.at("bus_velocity").AsInt() * 16.6666667;
    const auto wait_time = settings.at("bus_wait_time").AsDouble();
    TransportRouter transport_router(catalogue, speed, wait_time);
    auto result_graph = transport_router.GetGraph();
    graph::Router<double> result_router(result_graph);

    bool isfirstreq = true;
    json::Array array;
    for (auto &request_node: stat_req) {
      std::string type = request_node.AsMap().at("type").AsString();
      if (type == "Bus" || type == "Stop") {
        array.emplace_back(ProcessBusOrStop(type, isfirstreq, request_node, catalogue));
      } else if (type == "Route") {
        json::Dict result;
        if (isfirstreq) {
          result = SerializeRouteDataToJSON(request_node, catalogue, route_prop, result_router);
          isfirstreq = false;
        } else {
          result = SerializeRouteDataToJSON(request_node, catalogue, route_prop, result_router);
        }
        array.emplace_back(result);
      } else {
        MapRenderer renderer(catalogue, properties);
        svg::Document &result_doc = renderer.Render();
        std::ostringstream out;
        result_doc.Render(out);
        std::string map_rend_string = out.str();
        json::Dict result = SerializeMapDataToJSON(request_node, map_rend_string);
        if (isfirstreq) {
          array.emplace_back(result);
          isfirstreq = false;
        } else {
          array.emplace_back(result);
        }
      }
    }
    json::Print(json::Document{array}, output);
  }

  json::Dict SerializeMapDataToJSON(const json::Node& request_node, const std::string &map_rend_string) {

    auto request_id = request_node.AsMap().at("id");

    auto result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("map").Value(
        map_rend_string).EndDict().Build();
    return result.AsMap();
  }

  json::Dict SerializeBusDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue) {

    std::string name_of_the_bus = request_node.AsMap().at("name").AsString();
    auto request_id = request_node.AsMap().at("id");

    if (catalogue.FindBus(name_of_the_bus)) {
      auto data_of_bus = catalogue.GetBusInfo(name_of_the_bus);

      auto result = json::Builder{}.StartDict().Key("stop_count").Value(
          static_cast<int>(data_of_bus.numb_of_stops)).Key("request_id").Value(request_id).Key(
          "unique_stop_count").Value(static_cast<int>(data_of_bus.numb_of_unique_stops)).Key("route_length").Value(
          static_cast<int>(data_of_bus.true_length)).Key("curvature").Value(
          static_cast<double>(data_of_bus.true_length / data_of_bus.length)).EndDict().Build();

      return result.AsMap();
    } else {
      auto result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("error_message").Value(
          "not found").EndDict().Build();

      return result.AsMap();
    }
  }

  json::Dict SerializeStopDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue) {

    std::string name_of_the_stop = request_node.AsMap().at("name").AsString();
    auto request_id = request_node.AsMap().at("id");

    if (catalogue.FindStop(name_of_the_stop)) {
      json::Array pas_bus;
      for (auto bus: catalogue.GetStopInfo(name_of_the_stop).passing_buses) {
        pas_bus.emplace_back(std::string{bus});
      }
      auto result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("buses").Value(
          pas_bus).EndDict().Build();
      return result.AsMap();
    } else {
      auto result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("error_message").Value(
          "not found").EndDict().Build();

      return result.AsMap();
    }
  }

  json::Dict SerializeRouteDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue, const json::Node &route_prop, graph::Router<double> &result_router) {
    json::Node result;

    int request_id = request_node.AsMap().at("id").AsInt();
    std::string_view first_stop = request_node.AsMap().at("from").AsString();
    std::string_view last_stop = request_node.AsMap().at("to").AsString();
    auto settings = route_prop.AsMap();
    auto result_route = result_router.BuildRoute(catalogue.GetStopId(first_stop), catalogue.GetStopId(last_stop));

    if (result_route) {

      double wait_time = settings.at("bus_wait_time").AsDouble();
      json::Array items;

      for (auto &item: result_route.value().edges) {
        auto curr_edge = result_router.GetGraph().GetEdge(item);

        auto wait_item = json::Builder{}.StartDict().Key("type").Value("Wait").Key("stop_name").Value(
            catalogue.GetStopFromId(curr_edge.from)).Key("time").Value(wait_time).EndDict().Build();

        double bus_travel_time = curr_edge.weight - route_prop.AsMap().at("bus_wait_time").AsDouble();
        auto bus_item = json::Builder{}.StartDict().Key("type").Value("Bus").Key("bus").Value(curr_edge.bus_name).Key(
            "span_count").Value(curr_edge.span_count).Key("time").Value(bus_travel_time).EndDict().Build();

        items.push_back(std::move(wait_item));
        items.push_back(std::move(bus_item));
      }

      result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("total_time").Value(
          result_route.value().weight).Key("items").Value(std::move(items)).EndDict().Build();
    } else {
      result = json::Builder{}.StartDict().Key("request_id").Value(request_id).Key("error_message").Value(
          "not found").EndDict().Build();
    }

    return result.AsMap();
  }

  json::Dict ProcessBusOrStop(std::string_view type, bool& is_first_request, const json::Node& request_node, TransportCatalogue& catalogue) {
    json::Dict result;
    if (is_first_request) {
      result = type == "Bus" ? SerializeBusDataToJSON(request_node, catalogue) : SerializeStopDataToJSON(request_node, catalogue);
      is_first_request = false;
    } else {
      result = type == "Bus" ? SerializeBusDataToJSON(request_node, catalogue) : SerializeStopDataToJSON(request_node, catalogue);
    }
    return result;
  }

}