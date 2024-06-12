#pragma once

#include <stack>
#include <variant>
#include "json.h"

namespace json {
  class ArrayItemContext;

  class DictItemContext;

  class BaseContext;

  class Builder {
    json::Node root_;
    std::stack<Node *, std::vector<Node *>> nodes_stack_;

  public:
    Builder() = default;

    ~Builder() = default;

    Builder &Key(std::string key);

    Builder &Value(Node value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder &EndDict();

    Builder &EndArray();

    Node Build();
  };

  class KeyItemContext;

  class BaseContext {
  public:
    BaseContext(Builder &builder);

    Node Build();

    BaseContext Value(const Node &value);

    KeyItemContext Key(const std::string &key);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    BaseContext EndArray();

    BaseContext EndDict();

  protected:
    Builder &builder_;
  };

  class DictItemContext : public BaseContext {
  public:
    DictItemContext(BaseContext base);

    Node Build() = delete;

    KeyItemContext Key(const std::string &key);

    BaseContext Value(Node value) = delete;

    DictItemContext StartDict() = delete;

    ArrayItemContext StartArray() = delete;

    BaseContext EndArray() = delete;
  };

  class KeyItemContext : public BaseContext {
  public:
    KeyItemContext(DictItemContext dict);

    DictItemContext Value(const Node &value);

    Node Build() = delete;

    KeyItemContext Key(const std::string &key) = delete;

    BaseContext EndArray() = delete;

    BaseContext EndDict() = delete;
  };

  class ArrayItemContext : public BaseContext {
  public:
    ArrayItemContext(BaseContext base);

    Node Build() = delete;

    ArrayItemContext Value(const Node &value);

    KeyItemContext Key(std::string key) = delete;

    BaseContext EndDict() = delete;
  };

}
