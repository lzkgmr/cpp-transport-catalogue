#include "json_builder.h"

#include <utility>

json::BaseContext::BaseContext(Builder &builder) : builder_(builder) {}

json::Node json::BaseContext::Build() {
  return builder_.Build();
}

json::BaseContext json::BaseContext::Value(const Node &value) {
  builder_.Value(value);
  return *this;
}

json::DictItemContext json::BaseContext::StartDict() {
  builder_.StartDict();
  return {*this};
}

json::ArrayItemContext json::BaseContext::StartArray() {
  builder_.StartArray();
  return {*this};
}

json::DictItemContext::DictItemContext(BaseContext base) : BaseContext(base) {
}

json::KeyItemContext json::DictItemContext::Key(const std::string &key) {
  builder_.Key(key);
  return {*this};
}

json::KeyItemContext json::BaseContext::Key(const std::string &key) {
  builder_.Key(key);
  return {*this};
}

json::BaseContext json::BaseContext::EndDict() {
  builder_.EndDict();
  return this->builder_;
}

json::KeyItemContext::KeyItemContext(DictItemContext dict) : BaseContext(dict) {}

json::DictItemContext json::KeyItemContext::Value(const Node &value) {
  builder_.Value(value);
  return {*this};
}

json::ArrayItemContext::ArrayItemContext(BaseContext base) : BaseContext(base) {}

json::ArrayItemContext json::ArrayItemContext::Value(const Node &value) {
  builder_.Value(value);
  return {*this};
}

json::BaseContext json::BaseContext::EndArray() {
  builder_.EndArray();
  return this->builder_;
}

json::Builder &json::Builder::Key(std::string key) {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid Key(), object is done");
  }
  if (!nodes_stack_.top()->IsMap()) {
    throw std::logic_error("invalid Key()");
  }
  nodes_stack_.push(new Node(std::move(key)));

  return *this;
}

json::Builder &json::Builder::Value(Node value) {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid Value(), object is done");
  }
  if (root_ == nullptr) {
    root_ = std::move(value);
  } else if (nodes_stack_.top()->IsString()) {
    std::string temp = nodes_stack_.top()->AsString();
    delete nodes_stack_.top();
    nodes_stack_.pop();
    std::get<Dict>(nodes_stack_.top()->GetValue()).insert({temp, std::move(value)});
  } else if (nodes_stack_.top()->IsArray()) {
    std::get<Array>(nodes_stack_.top()->GetValue()).emplace_back(value);
  } else {
    throw std::logic_error("invalid Value()");
  }

  return *this;
}

json::DictItemContext json::Builder::StartDict() {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid StartDict(), object is done");
  }
  if (root_ == nullptr) {
    root_ = *new Dict;
    nodes_stack_.push(&root_);
  } else if (nodes_stack_.top()->IsString()) {
    std::string temp = nodes_stack_.top()->AsString();
    delete nodes_stack_.top();
    nodes_stack_.pop();
    std::get<Dict>(nodes_stack_.top()->GetValue()).insert({temp, *new Dict});
    nodes_stack_.push(&std::get<Dict>(nodes_stack_.top()->GetValue()).at(temp));
  } else if (nodes_stack_.top()->IsArray()) {
    std::get<Array>(nodes_stack_.top()->GetValue()).emplace_back(*new Dict);
    nodes_stack_.push((Node *) &std::get<Dict>(std::get<Array>(nodes_stack_.top()->GetValue()).back().GetValue()));
  } else {
    throw std::logic_error("invalid StartDict()");
  }

  return {*this};
}

json::ArrayItemContext json::Builder::StartArray() {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid StartArray(), object is done");
  }
  if (root_ == nullptr) {
    root_ = *new Array;
    nodes_stack_.push(&root_);
  } else if (nodes_stack_.top()->IsString()) {
    std::string temp = nodes_stack_.top()->AsString();
    delete nodes_stack_.top();
    nodes_stack_.pop();
    std::get<Dict>(nodes_stack_.top()->GetValue()).insert({temp, *new Array});
    nodes_stack_.push(&std::get<Dict>(nodes_stack_.top()->GetValue()).at(temp));
  } else if (nodes_stack_.top()->IsArray()) {
    std::get<Array>(nodes_stack_.top()->GetValue()).emplace_back(*new Array);
    nodes_stack_.push((Node *) &std::get<Array>(std::get<Array>(nodes_stack_.top()->GetValue()).back().GetValue()));
  } else {
    throw std::logic_error("invalid StartArray()");
  }
  return {*this};
}

json::Builder &json::Builder::EndDict() {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid EndDict(), object is done");
  }
  if (!nodes_stack_.top()->IsMap()) {
    throw std::logic_error("invalid EndDict()");
  }
  nodes_stack_.pop();

  return *this;
}

json::Builder &json::Builder::EndArray() {
  if (nodes_stack_.empty() && root_ != nullptr) {
    throw std::logic_error("invalid EndArray(), object is done");
  }
  if (!nodes_stack_.top()->IsArray()) {
    throw std::logic_error("invalid EndArray()");
  }
  nodes_stack_.pop();

  return *this;
}

json::Node json::Builder::Build() {
  if (!nodes_stack_.empty() || root_ == nullptr) {
    throw std::logic_error("invalid Build()");
  }
  return root_;
}
