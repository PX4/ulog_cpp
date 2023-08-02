/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <unordered_map>
#include <vector>

#include "data_handler_interface.hpp"

namespace ulog_cpp {

class DataContainer : public DataHandlerInterface {
 public:
  enum class StorageConfig {
    Header,   ///< keep header in memory
    FullLog,  ///< keep full log in memory
  };

  struct Subscription {
    AddLoggedMessage add_logged_message;
    std::vector<Data> data;
  };

  explicit DataContainer(StorageConfig storage_config);
  virtual ~DataContainer() = default;

  void error(const std::string& msg, bool is_recoverable) override;

  void headerComplete() override;

  void fileHeader(const FileHeader& header) override;
  void messageInfo(const MessageInfo& message_info) override;
  void messageFormat(const MessageFormat& message_format) override;
  void parameter(const Parameter& parameter) override;
  void parameterDefault(const ParameterDefault& parameter_default) override;
  void addLoggedMessage(const AddLoggedMessage& add_logged_message) override;
  void logging(const Logging& logging) override;
  void data(const Data& data) override;
  void dropout(const Dropout& dropout) override;

  // Stored data
  bool isHeaderComplete() const { return _header_complete; }
  bool hadFatalError() const { return _had_fatal_error; }
  const std::vector<std::string>& parsingErrors() const { return _parsing_errors; }
  const FileHeader& fileHeader() const { return _file_header; }
  const std::map<std::string, MessageInfo>& messageInfo() const { return _message_info; }
  const std::map<std::string, std::vector<std::vector<MessageInfo>>>& messageInfoMulti() const
  {
    return _message_info_multi;
  }
  const std::map<std::string, MessageFormat>& messageFormats() const { return _message_formats; }
  const std::map<std::string, Parameter>& initialParameters() const { return _initial_parameters; }
  const std::map<std::string, ParameterDefault>& defaultParameters() const
  {
    return _default_parameters;
  }
  const std::vector<Parameter>& changedParameters() const { return _changed_parameters; }
  const std::vector<Logging>& logging() const { return _logging; }
  const std::unordered_map<uint16_t, Subscription>& subscriptions() const { return _subscriptions; }
  const std::vector<Dropout>& dropouts() const { return _dropouts; }

 private:
  const StorageConfig _storage_config;

  bool _header_complete{false};
  bool _had_fatal_error{false};
  std::vector<std::string> _parsing_errors;

  FileHeader _file_header;
  std::map<std::string, MessageInfo> _message_info;
  std::map<std::string, std::vector<std::vector<MessageInfo>>> _message_info_multi;
  std::map<std::string, MessageFormat> _message_formats;
  std::map<std::string, Parameter> _initial_parameters;
  std::map<std::string, ParameterDefault> _default_parameters;
  std::vector<Parameter> _changed_parameters;
  std::unordered_map<uint16_t, Subscription> _subscriptions;
  std::vector<Logging> _logging;
  std::vector<Dropout> _dropouts;
};

}  // namespace ulog_cpp
