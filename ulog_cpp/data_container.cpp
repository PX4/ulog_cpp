/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "data_container.hpp"

namespace ulog_cpp {

DataContainer::DataContainer(DataContainer::StorageConfig storage_config)
    : _storage_config(storage_config)
{
}
void DataContainer::error(const std::string& msg, bool is_recoverable)
{
  if (!is_recoverable) {
    _had_fatal_error = true;
  }
  _parsing_errors.push_back(msg);
}

void DataContainer::headerComplete()
{
  _header_complete = true;
}
void DataContainer::fileHeader(const FileHeader& header)
{
  _file_header = header;
}
void DataContainer::messageInfo(const MessageInfo& message_info)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (message_info.isMulti()) {
    if (message_info.isContinued()) {
      auto& messages = _message_info_multi[message_info.field().name];
      if (messages.empty()) {
        throw ParsingException("info_multi msg is continued, but no previous");
      }
      messages[messages.size() - 1].push_back(message_info);
    } else {
      _message_info_multi[message_info.field().name].push_back({message_info});
    }
  } else {
    _message_info.insert({message_info.field().name, message_info});
  }
}
void DataContainer::messageFormat(const MessageFormat& message_format)
{
  if (_message_formats.find(message_format.name()) != _message_formats.end()) {
    throw ParsingException("Duplicate message format");
  }
  _message_formats.insert({message_format.name(), message_format});
}
void DataContainer::parameter(const Parameter& parameter)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (_header_complete) {
    _changed_parameters.push_back(parameter);
  } else {
    _initial_parameters.insert({parameter.field().name, parameter});
  }
}
void DataContainer::parameterDefault(const ParameterDefault& parameter_default)
{
  _default_parameters.insert({parameter_default.field().name, parameter_default});
}
void DataContainer::addLoggedMessage(const AddLoggedMessage& add_logged_message)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (_subscriptions.find(add_logged_message.msgId()) != _subscriptions.end()) {
    throw ParsingException("Duplicate AddLoggedMessage message ID");
  }
  _subscriptions.insert({add_logged_message.msgId(), {add_logged_message, {}}});
}
void DataContainer::logging(const Logging& logging)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  _logging.emplace_back(std::move(logging));
}
void DataContainer::data(const Data& data)
{
  if (_storage_config == StorageConfig::Header) {
    return;
  }
  const auto& iter = _subscriptions.find(data.msgId());
  if (iter == _subscriptions.end()) {
    throw ParsingException("Invalid subscription");
  }
  iter->second.data.emplace_back(std::move(data));
}
void DataContainer::dropout(const Dropout& dropout)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  _dropouts.emplace_back(std::move(dropout));
}
}  // namespace ulog_cpp
