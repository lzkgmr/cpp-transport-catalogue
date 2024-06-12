#include "svg.h"

namespace svg {

  using namespace std::literals;

  void Object::Render(const RenderContext &context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << std::endl;
  }

  // ---------- Circle ------------------

  Circle &Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
  }

  Circle &Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
  }

  void Circle::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
  }

  // ---------- Polyline ------------------

  Polyline &Polyline::AddPoint(Point point) {
    list_of_points.push_back(point);
    return *this;
  }

  void Polyline::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<polyline points=\""sv;
    bool are_first_coords = true;
    for (auto memb: list_of_points) {
      if (are_first_coords) {
        out << memb.x << ","sv << memb.y;
        are_first_coords = false;
      } else {
        out << " "sv << memb.x << ","sv << memb.y;
      }
    }
    out << "\"";
    RenderAttrs(context.out);
    out << "/>"sv;
  }

  // ----------- Text --------------------

  // Задаёт координаты опорной точки (атрибуты x и y)
  Text &Text::SetPosition(Point pos) {
    start_point_ = pos;
    return *this;
  }

  // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
  Text &Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
  }

  // Задаёт размеры шрифта (атрибут font-size)
  Text &Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
  }

  // Задаёт название шрифта (атрибут font-family)
  Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
  }

  // Задаёт толщину шрифта (атрибут font-weight)
  Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
  }

  // Задаёт текстовое содержимое объекта (отображается внутри тега text)
  Text &Text::SetData(std::string data) {
    text_ = std::move(data);
    return *this;
  }

  void Text::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<text"sv;
    RenderAttrs(context.out);

    out << " x=\""sv << start_point_.x << "\" y=\""sv << start_point_.y << "\" dx=\""sv << offset_.x << "\" dy=\""sv
        << offset_.y << "\" font-size=\""sv << font_size_ << "\""sv;
    if (font_family_ != "") {
      out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (font_weight_ != "") {
      out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">"sv;
    for (char letter: text_) {
      if (letter == '"') {
        out << "&quot;";
        break;
      }
      if (letter == '\'') {
        out << "&apos;";
        break;
      }
      if (letter == '<') {
        out << "&lt;";
        break;
      }
      if (letter == '>') {
        out << "&gt;";
        break;
      }
      if (letter == '&') {
        out << "&amp;";
        break;
      }

      out << letter;
    }

    out << "</text>"sv;
  }

  //----------------- Document ------------------

  void Document::AddPtr(std::unique_ptr<Object> &&obj) {
    all_obj.push_back(move(obj));
  }

  void Document::Render(std::ostream &out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;
    int count = 2;
    for (const auto &memb: all_obj) {
      RenderContext rc(out, count, count);
      memb.get()->Render(rc);
    }

    out << "</svg>" << std::endl;
  }

} // namespace svg