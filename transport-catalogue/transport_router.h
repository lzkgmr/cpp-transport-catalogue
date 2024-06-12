#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "graph.h"
#include "numeric"
#include "optional"
#include "router.h"

using namespace graph;

class TransportRouter {

public:
  TransportRouter(const transport_catalogue::TransportCatalogue &catalogue, double speed, double wait_time)
      : catalogue_(catalogue), speed_(speed), wait_time_(wait_time) {
    graph_ = BuildGraph();
  }

  const graph::DirectedWeightedGraph<double> &GetGraph() {
    return graph_;
  }

  template<typename Weight>
  std::optional<typename Router<Weight>::RouteInfo> GetRoute(graph::VertexId from, graph::VertexId to) const {
    graph::Router<Weight> router(&graph_);
    return router.BuildRoute(from, to);
  }

private:
  graph::DirectedWeightedGraph<double> BuildGraph();

  void AddRoundtripBusToGtaph(graph::DirectedWeightedGraph<double> &result, const transport_catalogue::Bus &bus);

  void AddDirectBusToGraph(graph::DirectedWeightedGraph<double> &result, const transport_catalogue::Bus &bus);

  const transport_catalogue::TransportCatalogue &catalogue_;
  double speed_;
  double wait_time_;
  graph::DirectedWeightedGraph<double> graph_;
};