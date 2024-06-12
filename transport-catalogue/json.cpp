#include "json.h"
#include <stdexcept>

using namespace std;

namespace json {
  bool Node::IsInt() const {
    return std::holds_alternative<int>(value_);
  }

  bool Node::IsDouble() const {
    return std::holds_alternative<int>(value_) || std::holds_alternative<double>(value_);
  }

  bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(value_);
  }

  bool Node::IsBool() const {
    return std::holds_alternative<bool>(value_);
  }

  bool Node::IsString() const {
    return std::holds_alternative<std::string>(value_);
  }

  bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(value_);
  }

  bool Node::IsArray() const {
    return std::holds_alternative<Array>(value_);
  }

  bool Node::IsMap() const {
    return std::holds_alternative<Dict>(value_);
  }

  namespace {

    Node LoadNode(istream &input);

    Node LoadArray(istream &input) {
      Array result;
      char c;
      for (; input >> c && c != ']';) {
        if (c != ',') {
          input.putback(c);
        }
        result.push_back(LoadNode(input));
      }
      if (c != ']') {
        throw json::ParsingError("Bad data for array");
      }

      return Node(move(result));
    }

    class ParsingError : public std::runtime_error {
    public:
      using runtime_error::runtime_error;
    };

    using Number = std::variant<int, double>;

    Number LoadNumber(std::istream &input) {
      using namespace std::literals;

      std::string parsed_num;

      auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
          throw ParsingError("Failed to read number from stream"s);
        }
      };

      auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
          throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
          read_char();
        }
      };

      if (input.peek() == '-') {
        read_char();
      }
      if (input.peek() == '0') {
        read_char();
      } else {
        read_digits();
      }

      bool is_int = true;
      if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
      }

      if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
          read_char();
        }
        read_digits();
        is_int = false;
      }

      try {
        if (is_int) {
          try {
            return std::stoi(parsed_num);
          }
          catch (...) {
          }
        }
        return std::stod(parsed_num);
      }
      catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
      }
    }

    std::string LoadStringPars(std::istream &input) {
      using namespace std::literals;

      auto it = std::istreambuf_iterator<char>(input);
      auto end = std::istreambuf_iterator<char>();
      std::string s;
      while (true) {
        if (it == end) {
          // Поток закончился до того, как встретили закрывающую кавычку?
          throw json::ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
          // Встретили закрывающую кавычку
          ++it;
          break;
        } else if (ch == '\\') {
          // Встретили начало escape-последовательности
          ++it;
          if (it == end) {
            // Поток завершился сразу после символа обратной косой черты
            throw json::ParsingError("String parsing error");
          }
          const char escaped_char = *(it);
          // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
          switch (escaped_char) {
            case 'n':
              s.push_back('\n');
              break;
            case 't':
              s.push_back('\t');
              break;
            case 'r':
              s.push_back('\r');
              break;
            case '"':
              s.push_back('"');
              break;
            case '\\':
              s.push_back('\\');
              break;
            default:
              // Встретили неизвестную escape-последовательность
              throw json::ParsingError("Unrecognized escape sequence \\"s + escaped_char);
          }
        } else if (ch == '\n' || ch == '\r') {
          // Строковый литерал внутри- JSON не может прерываться символами \r или \n
          throw json::ParsingError("Unexpected end of line"s);
        } else {
          // Просто считываем очередной символ и помещаем его в результирующую строку
          s.push_back(ch);
        }
        ++it;
      }

      return s;
    }

    Node LoadString(istream &input) {
      return Node(std::move(LoadStringPars(input)));
    }

    Node LoadDict(istream &input) {
      Dict result;
      char c;

      for (; input >> c && c != '}';) {
        if (c == ',') {
          input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
      }
      if (c != '}' && c != ':') {
        throw json::ParsingError("Bad data for dictionary");
      }

      return Node(move(result));
    }

    Node LoadInt(istream &input) {

      auto num = LoadNumber(input);
      if (std::get_if<int>(&num)) {
        return Node(std::get<int>(num));
      }
      return Node(std::get<double>(num));
    }

    Node LoadNode(istream &input) {
      char c;
      input >> c;

      if (c == '[') {
        return LoadArray(input);
      } else if (c == '{') {
        return LoadDict(input);
      } else if (c == '"') {
        return LoadString(input);
      } else if (c == 'n') {
        char null_[4];
        input.putback(c);
        input.read(null_, 4);
        string line = {null_};
        if (line.find("null") == std::string::npos) {
          throw json::ParsingError("Bad data for null");
        }
        return Node();
      } else if (c == 't') {
        char true_[4];
        input.putback(c);
        input.read(true_, 4);
        string line = {true_};

        if (line.find("true") == std::string::npos) {
          throw json::ParsingError("Bad data bool");
        }
        return Node(true);
      } else if (c == 'f') {
        char false_[5];
        input.putback(c);
        input.read(false_, 5);
        string line = {false_};
        if (line.find("false") == std::string::npos) {
          throw json::ParsingError("Bad data bool");
        }
        return Node(false);
      } else if (c == ']') {
        throw json::ParsingError("Bad data for array");
      } else if (c == '}') {
        throw json::ParsingError("Bad data for dictionary");
      } else {
        input.putback(c);
        return LoadInt(input);
      }
    }

  } // namespace

  bool Node::AsBool() const {
    if (!IsBool()) {
      throw std::logic_error("Wrong type");
    } else {
      return std::get<bool>(value_);
    }
  }

  double Node::AsDouble() const {
    if (!IsDouble()) {
      throw std::logic_error("Wrong type");
    }
    if (!IsPureDouble()) {
      return static_cast<double>(std::get<int>(value_));
    }

    return std::get<double>(value_);
  }

  const Array &Node::AsArray() const {
    if (!IsArray()) {
      throw std::logic_error("Wrong type");
    } else {
      return std::get<Array>(value_);
    }
  }

  const Dict &Node::AsMap() const {
    if (!IsMap()) {
      throw std::logic_error("Wrong type");
    } else {
      return std::get<Dict>(value_);
    }
  }

  int Node::AsInt() const {
    if (!IsInt()) {
      throw std::logic_error("Wrong type");
    } else {
      return std::get<int>(value_);
    }
  }

  const string &Node::AsString() const {
    if (!IsString()) {
      throw std::logic_error("Wrong type");
    } else {
      return std::get<std::string>(value_);
    }
  }

  Document::Document(Node root)
      : root_(move(root)) {
  }

  const Node &Document::GetRoot() const {
    return root_;
  }

  Document Load(istream &input) {
    return Document{LoadNode(input)};
  }

  void PrintNode(const Node &node, std::ostream &out);

  void PrintValue(std::nullptr_t, std::ostream &out) {
    out << "null"sv;
  }

  void PrintValue(const string &value, std::ostream &out) {
    out << '\"';
    string a;
    for (size_t i = 0; i != value.size(); ++i) {

      switch (value[i]) {
        case '\n':
          out << '\\' << 'n';
          break;
        case '\r':
          out << '\\' << 'r';
          break;
        default:

          if (value[i] == '\\' || value[i] == '\"') {
            out << "\\"s;
            a += "\\"s;
          }
          out << value[i];
          a += value[i];
          break;
      }
    }
    out << '\"' << std::flush;
  }

  void PrintValue(const Dict &value, std::ostream &out) {
    out << "{\n";
    bool first = true;
    for (const auto &elem: value) {
      if (first) {
        first = false;
      } else {
        out << ",\n";
      }
      out << "\"" << elem.first << "\": ";
      PrintNode(elem.second, out);
    }
    out << "\n}" << std::flush;
  }

  void PrintValue(const Array &value, std::ostream &out) {
    out << "[\n";
    bool first = true;
    for (const auto &elem: value) {
      if (first) {
        first = false;
      } else {
        out << ",\n";
      }
      PrintNode(elem, out);
    }
    out << "\n]" << std::flush;
  }

  void PrintValue(const bool &value, std::ostream &out) {
    if (value) {
      out << "true" << std::flush;
    } else {
      out << "false" << std::flush;
    }
  }

  void PrintNode(const Node &node, std::ostream &out) {
    std::visit(
        [&out](const auto &value) { PrintValue(value, out); },
        node.GetValue());
  }

  void Print(const Document &doc, std::ostream &output) {
    PrintNode(doc.GetRoot(), output);
  }

  bool Node::operator==(const Node &rhs) const {
    return (this->value_ == rhs.GetValue());
  }

  bool Node::operator!=(const Node &rhs) const {
    return !(this->value_ == rhs.GetValue());
  }

  bool Document::operator==(const Document &rhs) const {
    return this->root_.GetValue() == rhs.GetRoot().GetValue();
  }

  bool Document::operator!=(const Document &rhs) const {
    return !(this->root_.GetValue() == rhs.GetRoot().GetValue());
  }
}
