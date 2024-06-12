#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

  class Node;

  using Dict = std::map<std::string, Node>;
  using Array = std::vector<Node>;

  class ParsingError : public std::runtime_error {
  public:
    using runtime_error::runtime_error;
  };

  class Node {
  public:
    Node() = default;

    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    template<typename Type>
    Node(Type value) : value_(value) {}

    bool IsInt() const;

    bool IsDouble() const;

    bool IsPureDouble() const;

    bool IsBool() const;

    bool IsString() const;

    bool IsNull() const;

    bool IsArray() const;

    bool IsMap() const;

    bool AsBool() const;

    double AsDouble() const;

    const Array &AsArray() const;

    const Dict &AsMap() const;

    int AsInt() const;

    const std::string &AsString() const;

    bool operator==(const Node &rhs) const;

    bool operator!=(const Node &rhs) const;

    const Value &GetValue() const {
      return value_;
    }

    Value &GetValue() {
      return value_;
    }

  private:
    Value value_;
  };

  template<typename Value>
  void PrintValue(const Value &value, std::ostream &out) {
    out << value << std::flush;
  }

  class Document {
  public:
    explicit Document(Node root);

    const Node &GetRoot() const;

    bool operator==(const Document &rhs) const;

    bool operator!=(const Document &rhs) const;

  private:
    Node root_;
  };

  Document Load(std::istream &input);

  void Print(const Document &doc, std::ostream &output);

} // namespace json