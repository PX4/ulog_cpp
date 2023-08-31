/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#pragma once
#include <utility>

#include "messages.hpp"

namespace ulog_cpp {

class TypedDataView {
 public:
  TypedDataView(const Data& data_ref, const MessageFormat& message_format_ref)
      : _data_ref(data_ref), _message_format_ref(message_format_ref)
  {
  }

  std::string name() const { return _message_format_ref.name(); }

  Value at(const Field& field) const
  {
    if (!field.definitionResolved()) {
      throw ParsingException("Field definition not resolved");
    }
    return Value(field, _data_ref.data());
  }

  Value at(const std::shared_ptr<Field>& field) const { return at(*field); }

  Value at(const std::string& field_name) const
  {
    auto field = _message_format_ref.field(field_name);
    return at(field);
  }

  Value operator[](const Field& field) const { return at(field); }

  Value operator[](const std::shared_ptr<Field>& field) const { return at(field); }

  Value operator[](const std::string& field_name) const { return at(field_name); }

  const MessageFormat& format() const { return _message_format_ref; }
  const std::vector<uint8_t>& rawData() const { return _data_ref.data(); }

 private:
  const Data& _data_ref;
  const MessageFormat& _message_format_ref;
};

template <typename base_iterator_type>
class SubscriptionIterator {
 public:
  SubscriptionIterator(const base_iterator_type& it,
                       const std::shared_ptr<MessageFormat>& message_format)
      : _it(it), _message_format(message_format)
  {
  }

  using iterator_category = std::random_access_iterator_tag;
  using value_type = TypedDataView;
  using difference_type = std::ptrdiff_t;
  using pointer = TypedDataView*;
  using reference = TypedDataView&;

  TypedDataView operator*() { return TypedDataView(*_it, *_message_format); }

  const TypedDataView operator*() const { return TypedDataView(*_it, *_message_format); }

  SubscriptionIterator& operator++()
  {
    ++_it;
    return *this;
  }

  SubscriptionIterator operator++(int)
  {
    SubscriptionIterator tmp(*this);
    operator++();
    return tmp;
  }

  SubscriptionIterator& operator--()
  {
    --_it;
    return *this;
  }

  SubscriptionIterator operator--(int)
  {
    SubscriptionIterator tmp(*this);
    operator--();
    return tmp;
  }

  SubscriptionIterator& operator+=(difference_type n)
  {
    _it += n;
    return *this;
  }

  SubscriptionIterator operator+(difference_type n) const
  {
    SubscriptionIterator tmp(*this);
    tmp += n;
    return tmp;
  }

  SubscriptionIterator& operator-=(difference_type n)
  {
    _it -= n;
    return *this;
  }

  SubscriptionIterator operator-(difference_type n) const
  {
    SubscriptionIterator tmp(*this);
    tmp -= n;
    return tmp;
  }

  difference_type operator-(const SubscriptionIterator& other) const { return _it - other._it; }

  TypedDataView operator[](difference_type n) { return *((*this) + n); }

  bool operator==(const SubscriptionIterator& other) const { return _it == other._it; }

  bool operator!=(const SubscriptionIterator& other) const { return _it != other._it; }

  bool operator<(const SubscriptionIterator& other) const { return _it < other._it; }

  bool operator>(const SubscriptionIterator& other) const { return _it > other._it; }

  bool operator<=(const SubscriptionIterator& other) const { return _it <= other._it; }

  bool operator>=(const SubscriptionIterator& other) const { return _it >= other._it; }

 private:
  base_iterator_type _it;
  std::shared_ptr<MessageFormat> _message_format;
};

class Subscription {
 public:
  Subscription(AddLoggedMessage add_logged_message, std::vector<Data> samples,
               std::shared_ptr<MessageFormat> message_format)
      : _add_logged_message(std::move(add_logged_message)),
        _message_format(message_format),
        _samples(std::move(samples))
  {
  }

  void emplaceSample(const Data& sample) { _samples.emplace_back(sample); }

  const AddLoggedMessage& getAddLoggedMessage() const { return _add_logged_message; }

  const std::vector<Data>& rawSamples() const { return _samples; }

  const std::shared_ptr<MessageFormat>& format() const { return _message_format; }

  const std::map<std::string, std::shared_ptr<Field>>& fieldMap() const
  {
    return _message_format->fieldMap();
  }

  std::vector<std::string> fieldNames() const { return _message_format->fieldNames(); }

  std::shared_ptr<Field> field(const std::string& name) const
  {
    return _message_format->field(name);
  }

  auto begin()
  {
    return SubscriptionIterator<std::vector<Data>::iterator>(_samples.begin(), _message_format);
  }
  auto end()
  {
    return SubscriptionIterator<std::vector<Data>::iterator>(_samples.end(), _message_format);
  }

  auto begin() const
  {
    return SubscriptionIterator<std::vector<Data>::const_iterator>(_samples.begin(),
                                                                   _message_format);
  }
  auto end() const
  {
    return SubscriptionIterator<std::vector<Data>::const_iterator>(_samples.cend(),
                                                                   _message_format);
  }

  TypedDataView at(std::size_t n) const { return begin()[n]; }

  TypedDataView operator[](std::size_t n) { return at(n); }

  std::size_t size() const { return _samples.size(); }

 private:
  AddLoggedMessage _add_logged_message;
  std::shared_ptr<MessageFormat> _message_format;
  std::vector<Data> _samples;
};

}  // namespace ulog_cpp