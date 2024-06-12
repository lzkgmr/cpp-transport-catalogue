#pragma once

#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"
#include "graph.h"
#include "router.h"

namespace transport_catalogue {
  InputBusData ParseBusInfo(const json::Dict& dict_bus_info);

  Stop ParseStopInfo(const json::Dict& dict_stop_info);

  struct Requests {
    json::Array stops_requests;
    json::Array bus_requests;
  };

  void ParseAndExecuteRequests(const json::Array& base_req, TransportCatalogue &catalogue);

  void
  ExecuteRequests(const json::Array &stat_req, std::ostream &output, TransportCatalogue &catalogue, SvgInfo &properties,
                  const json::Node& route_prop);

  void ProcessRequest(std::istream &input, std::ostream &output, TransportCatalogue &catalogue);

  json::Dict SerializeBusDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue);

  json::Dict SerializeStopDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue);

  json::Dict SerializeMapDataToJSON(const json::Node& request_node, const std::string &map_rend_string);

  json::Dict SerializeRouteDataToJSON(const json::Node& request_node, TransportCatalogue &catalogue, const json::Node &route_prop,
                               graph::Router<double> &result_router);
  json::Dict ProcessBusOrStop(std::string_view type, bool& is_first_request, const json::Node& request_node, TransportCatalogue& catalogue);
}