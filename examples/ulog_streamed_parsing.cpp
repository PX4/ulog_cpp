/****************************************************************************
 * Copyright (c) 2025 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <string>
#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

// Example for parsing and processing an ULog file in real-time, without keeping the whole file
// in memory

class TopicSubscription {
 public:
  virtual ~TopicSubscription() = default;

  virtual void handleData(const ulog_cpp::TypedDataView& data) = 0;
};

// A topic we are interested in
class VehicleStatus : public TopicSubscription {
 public:
  explicit VehicleStatus(const std::shared_ptr<ulog_cpp::Subscription>& subscription)
  {
    _timestamp_field = subscription->field("timestamp");
    _nav_state_field = subscription->field("nav_state");
    // Optional field (e.g. when a message changes)
    if (subscription->fieldMap().find("armed_state") != subscription->fieldMap().end()) {
      _armed_state_field = subscription->field("armed_state");
    }
  }

  void handleData(const ulog_cpp::TypedDataView& data) override
  {
    const auto timestamp = data[*_timestamp_field].as<std::uint64_t>();
    const auto nav_state = data[*_nav_state_field].as<std::uint32_t>();
    uint8_t armed_state = 0;
    if (_armed_state_field) {
      armed_state = data[*_nav_state_field].as<std::uint8_t>();
    }
    printf("vehicle_status: t: %" PRId64 ": nav_state: %" PRIu32 ", armed_state: %" PRId8 "\n",
           timestamp, nav_state, armed_state);
  }

 private:
  std::shared_ptr<ulog_cpp::Field> _timestamp_field;
  std::shared_ptr<ulog_cpp::Field> _nav_state_field;
  std::shared_ptr<ulog_cpp::Field> _armed_state_field;
};

class ULogDataHandler : public ulog_cpp::DataContainer {
 public:
  ULogDataHandler() : ulog_cpp::DataContainer(ulog_cpp::DataContainer::StorageConfig::Header) {}

  void error(const std::string& msg, bool is_recoverable) override
  {
    printf("Parsing error: %s\n", msg.c_str());
  }

  void headerComplete() override { ulog_cpp::DataContainer::headerComplete(); }

  void messageInfo(const ulog_cpp::MessageInfo& message_info) override
  {
    DataContainer::messageInfo(message_info);
    if (message_info.isMulti()) {
      // Multi messages might be continued, but we only know with the next message, so we keep it
      // stored and append if needed. We assume that continued multi messages are not interleaved
      // with other messages.
      if (message_info.isContinued()) {
        if (_current_multi_message.field().name() == message_info.field().name()) {
          // Append to previous
          _current_multi_message.valueRaw().insert(_current_multi_message.valueRaw().end(),
                                                   message_info.valueRaw().begin(),
                                                   message_info.valueRaw().end());
        }
      } else {
        finishCurrentMultiMessage();
        _current_multi_message = message_info;
      }
    } else {
      finishCurrentMultiMessage();
      messageInfoComplete(message_info);
    }
  }
  void parameter(const ulog_cpp::Parameter& parameter) override
  {
    finishCurrentMultiMessage();
    DataContainer::parameter(parameter);
  }
  void addLoggedMessage(const ulog_cpp::AddLoggedMessage& add_logged_message) override
  {
    finishCurrentMultiMessage();
    DataContainer::addLoggedMessage(add_logged_message);
    if (_subscriptions_by_message_id.find(add_logged_message.msgId()) !=
        _subscriptions_by_message_id.end()) {
      throw ulog_cpp::ParsingException("Duplicate AddLoggedMessage message ID");
    }

    auto format_iter = messageFormats().find(add_logged_message.messageName());
    if (format_iter == messageFormats().end()) {
      throw ulog_cpp::ParsingException("AddLoggedMessage message format not found");
    }

    auto ulog_subscription = std::make_shared<ulog_cpp::Subscription>(
        add_logged_message, std::vector<ulog_cpp::Data>{}, format_iter->second);

    if (add_logged_message.messageName() == "vehicle_status" && add_logged_message.multiId() == 0) {
      auto subscription = std::make_shared<VehicleStatus>(ulog_subscription);
      _subscriptions_by_message_id.insert(
          {add_logged_message.msgId(), SubscriptionData{ulog_subscription, subscription}});
    }
  }
  void logging(const ulog_cpp::Logging& logging) override
  {
    finishCurrentMultiMessage();
    DataContainer::logging(logging);
  }
  void data(const ulog_cpp::Data& data) override
  {
    finishCurrentMultiMessage();
    const auto iter = _subscriptions_by_message_id.find(data.msgId());
    if (iter != _subscriptions_by_message_id.end()) {
      const ulog_cpp::TypedDataView data_view(data, *iter->second.ulog_subscription->format());
      iter->second.subscription->handleData(data_view);
    }
  }

 private:
  struct SubscriptionData {
    std::shared_ptr<ulog_cpp::Subscription> ulog_subscription;
    std::shared_ptr<TopicSubscription> subscription;
  };

  void finishCurrentMultiMessage()
  {
    if (!_current_multi_message.field().name().empty()) {
      messageInfoComplete(_current_multi_message);
      _current_multi_message.field() = {};
    }
  }
  void messageInfoComplete(const ulog_cpp::MessageInfo& message_info)
  {
    if (message_info.field().definitionResolved()) {
      printf("Info message: %s\n", message_info.field().name().c_str());
    }
  }

  std::map<uint16_t, SubscriptionData> _subscriptions_by_message_id;
  ulog_cpp::MessageInfo _current_multi_message{"",
                                               ""};  ///< Keep this stored for continued messages
};

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("Usage: %s <file.ulg>\n", argv[0]);
    return -1;
  }
  FILE* file = fopen(argv[1], "rb");
  if (!file) {
    printf("opening file failed\n");
    return -1;
  }
  uint8_t buffer[4096];
  int bytes_read;
  const auto data_container = std::make_shared<ULogDataHandler>();
  ulog_cpp::Reader reader{data_container};
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    try {
      reader.readChunk(buffer, bytes_read);
    } catch (const ulog_cpp::ExceptionBase& exception) {
      printf("Failed to parse ulog file: %s\n", exception.what());
      return -1;
    }
    if (data_container->hadFatalError()) {
      printf("Failed to parse ulog file\n");
      return -1;
    }
  }
  fclose(file);

  return 0;
}
