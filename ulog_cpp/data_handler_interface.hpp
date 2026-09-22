/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <string>

#include "messages.hpp"

namespace ulog_cpp {

class DataHandlerInterface {
 public:
  virtual void headerComplete() {}

  virtual void error(const std::string& msg, bool is_recoverable) {}

  // Data methods
  virtual void fileHeader(const FileHeader& header) {}
  virtual void messageInfo(const MessageInfo& message_info) {}
  virtual void messageFormat(const MessageFormat& message_format) {}
  virtual void parameter(const Parameter& parameter) {}
  virtual void parameterDefault(const ParameterDefault& parameter_default) {}
  virtual void addLoggedMessage(const AddLoggedMessage& add_logged_message) {}
  virtual void logging(const Logging& logging) {}
  virtual void data(const Data& data) {}
  virtual void dropout(const Dropout& dropout) {}
  virtual void sync(const Sync& sync) {}

  /**
   * Used by Reader's corruption-recovery search to check a candidate byte offset that
   * looks like it could be the start of a DATA message before accepting it as a resync
   * point. Reader has no knowledge of subscriptions/formats itself, so it delegates the
   * check here. Default is permissive (accepts everything).
   * @param msg_id the candidate message's embedded msg_id
   * @param payload_size the candidate message's payload size (msg_size minus the 2-byte
   * msg_id)
   * @return true if msg_id is a known subscription and payload_size is plausible for it
   */
  virtual bool isValidDataMessage(uint16_t msg_id, uint16_t payload_size) const { return true; }

 private:
};

}  // namespace ulog_cpp
