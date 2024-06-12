#include "map_renderer.h"
#include <set>

namespace transport_catalogue {
  bool SphereProjector::IsZero(double value) {
    return std::abs(value) < EPSILON;
  }

  svg::Color ParseColor(const json::Node &node) {
    svg::Color result;
    if (node.IsString()) {
      result = node.AsString();
    } else if (node.AsArray().size() == 3) {
      result = svg::Rgb{static_cast<uint8_t>(node.AsArray()[0].AsInt()),
                        static_cast<uint8_t>(node.AsArray()[1].AsInt()),
                        static_cast<uint8_t>(node.AsArray()[2].AsInt())};
    } else {

      result = svg::Rgba{static_cast<uint8_t>(node.AsArray()[0].AsInt()),
                         static_cast<uint8_t>(node.AsArray()[1].AsInt()),
                         static_cast<uint8_t>(node.AsArray()[2].AsInt()),
                         node.AsArray()[3].AsDouble()};
    }
    return result;
  }

  SvgInfo ParsePropLine(const json::Node &node) {
    SvgInfo res;
    res.width = node.AsMap().at("width").AsDouble();
    res.height = node.AsMap().at("height").AsDouble();
    res.padding = node.AsMap().at("padding").AsDouble();
    res.line_width = node.AsMap().at("line_width").AsDouble();
    res.stop_radius = node.AsMap().at("stop_radius").AsDouble();
    res.bus_label_font_size = node.AsMap().at("bus_label_font_size").AsInt();
    res.bus_label_offset = {node.AsMap().at("bus_label_offset").AsArray()[0].AsDouble(),
                            node.AsMap().at("bus_label_offset").AsArray()[1].AsDouble()};

    res.stop_label_font_size = node.AsMap().at("stop_label_font_size").AsInt();
    res.stop_label_offset = {node.AsMap().at("stop_label_offset").AsArray()[0].AsDouble(),
                             node.AsMap().at("stop_label_offset").AsArray()[1].AsDouble()};
    res.underlayer_color = ParseColor(node.AsMap().at("underlayer_color"));
    res.underlayer_width = node.AsMap().at("underlayer_width").AsDouble();

    auto color_array = node.AsMap().at("color_palette").AsArray();
    for (const auto &color: color_array) {
      res.color_palette.push_back(ParseColor(color));
    }
    return res;
  }

  MapRenderer::MapRenderer(TransportCatalogue &catalogue, SvgInfo prop) :catalogue_(catalogue), prop_(std::move(prop)) {}

  svg::Document &MapRenderer::Render() {
    static svg::Document result;
    auto all_buses = catalogue_.GetAllBuses();

    // Формирование вектора всех остановок для передачи конструктору SphereProjector
    std::vector<geo::Coordinates> coords_of_all_stops_buses;
    for (const auto &bus: all_buses) {
      for (auto stop: bus.stops) {
        coords_of_all_stops_buses.push_back(stop->coordinates);
      }
    }

    // Процесс сортировки маршрутов по возрастанию названий
    std::vector<Bus> temp_vector;

    for (const auto &bus: all_buses) {
      temp_vector.push_back(bus);
    }
    std::sort(temp_vector.begin(), temp_vector.end(), [](Bus &lhs, Bus &rhs) { return lhs.name < rhs.name; });

    int color_iterator = 0;
    for (auto &bus: temp_vector) {
      result.Add(std::move(DrawThePolyline(coords_of_all_stops_buses, bus, color_iterator)));
    }
    color_iterator = 0;
    for (auto &bus: temp_vector) {
      std::vector<svg::Text> result_names_routes = std::move(
          DrawRouteNames(coords_of_all_stops_buses, bus, color_iterator));
      if (!result_names_routes.empty()) {
        for (auto &text: result_names_routes) {
          result.Add(std::move(text));
        }
      }
    }

    DrawStops(coords_of_all_stops_buses, result);

    DrawStopNames(coords_of_all_stops_buses, result);
    return result;
  }

  void MapRenderer::DrawStops(const std::vector<geo::Coordinates> &coords, svg::Document &result_doc) {
    std::vector<svg::Circle> result;
    auto projector = SphereProjector(coords.begin(), coords.end(), prop_.width, prop_.height, prop_.padding);

    auto all_unique_stops = catalogue_.GetUniqueStops();
    result.reserve(all_unique_stops.size());
    for (auto unique_stop: all_unique_stops) {
      geo::Coordinates coords_of_stop = catalogue_.FindStop(std::string{unique_stop})->coordinates;
      svg::Point pt = projector(coords_of_stop);
      svg::Circle stop;
      stop.SetCenter(pt)
          .SetRadius(prop_.stop_radius)
          .SetFillColor("white");
      result.emplace_back(std::move(stop));
    }

    for (auto &circle: result) {
      result_doc.Add(std::move(circle));
    }
  }

  void MapRenderer::DrawStopNames(const std::vector<geo::Coordinates> &coords, svg::Document &result_doc) {
    std::vector<svg::Text> result;
    auto projector = SphereProjector(coords.begin(), coords.end(), prop_.width, prop_.height, prop_.padding);

    auto all_unique_stops = catalogue_.GetUniqueStops();
    result.reserve(all_unique_stops.size());
    for (auto stop: all_unique_stops) {
      geo::Coordinates coords_of_stop = catalogue_.FindStop(std::string{stop})->coordinates;
      svg::Point pt = projector(coords_of_stop);

      svg::Text add_text;
      add_text.SetPosition(pt)
          .SetOffset({prop_.stop_label_offset.dx, prop_.stop_label_offset.dy})
          .SetFontSize(prop_.stop_label_font_size)
          .SetFontFamily("Verdana")
          .SetData(catalogue_.FindStop(std::string{stop})->name)
          .SetFillColor(prop_.underlayer_color)
          .SetStrokeColor(prop_.underlayer_color)
          .SetStrokeWidth(prop_.underlayer_width)
          .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
          .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

      result.emplace_back(std::move(add_text));

      svg::Text main_text;
      main_text.SetPosition(pt)
          .SetOffset({prop_.stop_label_offset.dx, prop_.stop_label_offset.dy})
          .SetFontSize(prop_.stop_label_font_size)
          .SetFontFamily("Verdana")
          .SetData(catalogue_.FindStop(std::string{stop})->name)
          .SetFillColor("black");

      result.emplace_back(std::move(main_text));
    }

    for (auto &text: result) {
      result_doc.Add(std::move(text));
    }
  }

  std::vector<svg::Text> MapRenderer::DrawRouteNames(const std::vector<geo::Coordinates> &coords, Bus bus, int &color_iterator) {
    if (!bus.stops.empty()) {
      std::vector<svg::Text> result;
      result.reserve(4);
      auto projector = SphereProjector(coords.begin(), coords.end(), prop_.width, prop_.height, prop_.padding);

      svg::Point pt = projector(bus.stops[0]->coordinates);
      svg::Text add_name;
      add_name.SetPosition(pt)
          .SetOffset({prop_.bus_label_offset.dx, prop_.bus_label_offset.dy})
          .SetFontSize(prop_.bus_label_font_size)
          .SetFontFamily("Verdana")
          .SetFontWeight("bold")
          .SetData(bus.name)
          .SetFillColor(prop_.underlayer_color)
          .SetStrokeColor(prop_.underlayer_color)
          .SetStrokeWidth(prop_.underlayer_width)
          .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
          .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

      result.emplace_back(std::move(add_name));

      svg::Text name;
      name.SetPosition(pt)
          .SetOffset({prop_.bus_label_offset.dx, prop_.bus_label_offset.dy})
          .SetFontSize(prop_.bus_label_font_size)
          .SetFontFamily("Verdana")
          .SetFontWeight("bold")
          .SetData(bus.name)
          .SetFillColor(prop_.color_palette[color_iterator % prop_.color_palette.size()]);

      result.emplace_back(std::move(name));

      if (!bus.is_roundtrip) {
        svg::Point pt_2 = projector(bus.stops[(bus.stops.size() - 1) / 2]->coordinates);
        if ((pt_2.x != pt.x) && (pt_2.y != pt.y)) {

          svg::Text second_add_name;
          second_add_name.SetPosition(pt_2)
              .SetOffset({prop_.bus_label_offset.dx, prop_.bus_label_offset.dy})
              .SetFontSize(prop_.bus_label_font_size)
              .SetFontFamily("Verdana")
              .SetFontWeight("bold")
              .SetData(bus.name)
              .SetFillColor(prop_.underlayer_color)
              .SetStrokeColor(prop_.underlayer_color)
              .SetStrokeWidth(prop_.underlayer_width)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

          result.emplace_back(std::move(second_add_name));

          svg::Text second_name;
          second_name.SetPosition(pt_2)
              .SetOffset({prop_.bus_label_offset.dx, prop_.bus_label_offset.dy})
              .SetFontSize(prop_.bus_label_font_size)
              .SetFontFamily("Verdana")
              .SetFontWeight("bold")
              .SetData(bus.name)
              .SetFillColor(prop_.color_palette[color_iterator % prop_.color_palette.size()]);

          result.emplace_back(std::move(second_name));
        }
      }
      color_iterator++;
      return result;
    } else
      color_iterator++;
    return {};
  }

  svg::Polyline MapRenderer::DrawThePolyline(const std::vector<geo::Coordinates> &coords, const Bus &bus, int &color_iterator) {
    svg::Polyline result;
    result.SetFillColor("none")
        .SetStrokeWidth(prop_.line_width)
        .SetStrokeColor(prop_.color_palette[color_iterator % prop_.color_palette.size()])
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    auto projector = SphereProjector(coords.begin(), coords.end(), prop_.width, prop_.height, prop_.padding);

    // Теперь строим точки
    for (auto stop: bus.stops) {
      svg::Point pt = projector(stop->coordinates);
      result.AddPoint(pt);
    }
    color_iterator++;
    return result;
  }
}