/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>
#include <variant>

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
  uint8_t buffer[4048];
  int bytes_read;
  const auto data_container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader{data_container};
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    reader.readChunk(buffer, bytes_read);
  }
  fclose(file);

  // Check for errors
  if (!data_container->parsingErrors().empty()) {
    printf("###### File Parsing Errors ######\n");
    for (const auto& parsing_error : data_container->parsingErrors()) {
      printf("   %s\n", parsing_error.c_str());
    }
  }
  if (data_container->hadFatalError()) {
    printf("Fatal parsing error, exiting\n");
    return -1;
  }

  // Print info
  // Dropouts
  const auto& dropouts = data_container->dropouts();
  const int total_dropouts_ms = std::accumulate(
      dropouts.begin(), dropouts.end(), 0,
      [](int sum, const ulog_cpp::Dropout& curr) { return sum + curr.durationMs(); });
  printf("Dropouts: %zu, total duration: %i ms\n", dropouts.size(), total_dropouts_ms);

  auto print_value = [](const std::string& name, const ulog_cpp::Value& value) {
    if (const auto* const str_ptr(std::get_if<std::string>(&value.data())); str_ptr) {
      printf(" %s: %s\n", name.c_str(), str_ptr->c_str());
    } else if (const auto* const int_ptr(std::get_if<int32_t>(&value.data())); int_ptr) {
      printf(" %s: %i\n", name.c_str(), *int_ptr);
    } else if (const auto* const uint_ptr(std::get_if<uint32_t>(&value.data())); uint_ptr) {
      printf(" %s: %u\n", name.c_str(), *uint_ptr);
    } else if (const auto* const float_ptr(std::get_if<float>(&value.data())); float_ptr) {
      printf(" %s: %.3f\n", name.c_str(), static_cast<double>(*float_ptr));
    } else {
      printf(" %s: <data>\n", name.c_str());
    }
  };

  // Info messages
  printf("Info Messages:\n");
  for (const auto& info_msg : data_container->messageInfo()) {
    print_value(info_msg.second.field().name, info_msg.second.value());
  }
  // Info multi messages
  printf("Info Multiple Messages:");
  for (const auto& info_msg : data_container->messageInfoMulti()) {
    printf(" [%s: %zu],", info_msg.first.c_str(), info_msg.second.size());
  }
  printf("\n");

  // Messages
  printf("\n");
  printf("Name (multi id)  - number of data points\n");

  // Sort by name & multi id
  const auto& subscriptions = data_container->subscriptions();
  std::vector<uint16_t> sorted_subscription_ids(subscriptions.size());
  std::transform(subscriptions.begin(), subscriptions.end(), sorted_subscription_ids.begin(),
                 [](const auto& pair) { return pair.first; });
  std::sort(sorted_subscription_ids.begin(), sorted_subscription_ids.end(),
            [&subscriptions](const uint16_t a, const uint16_t b) {
              const auto& add_logged_a = subscriptions.at(a).add_logged_message;
              const auto& add_logged_b = subscriptions.at(b).add_logged_message;
              if (add_logged_a.messageName() == add_logged_b.messageName()) {
                return add_logged_a.multiId() < add_logged_b.multiId();
              }
              return add_logged_a.messageName() < add_logged_b.messageName();
            });
  for (const auto& subscription_id : sorted_subscription_ids) {
    const auto& subscription = subscriptions.at(subscription_id);
    const int multi_instance = subscription.add_logged_message.multiId();
    const std::string message_name = subscription.add_logged_message.messageName();
    printf(" %s (%i)   -  %zu\n", message_name.c_str(), multi_instance, subscription.data.size());
  }

  printf("Formats:\n");
  for (const auto& msg_format : data_container->messageFormats()) {
    std::string format_fields;
    for (const auto& field : msg_format.second.fields()) {
      format_fields += field.encode() + ", ";
    }
    printf(" %s: %s\n", msg_format.second.name().c_str(), format_fields.c_str());
  }

  // logging
  printf("Logging:\n");
  for (const auto& logging : data_container->logging()) {
    std::string tag_str;
    if (logging.hasTag()) {
      tag_str = std::to_string(logging.tag()) + " ";
    }
    printf(" %s<%s> %lu %s\n", tag_str.c_str(), logging.logLevelStr().c_str(), logging.timestamp(),
           logging.message().c_str());
  }

  // Params (init, after, defaults)
  printf("Default Params:\n");
  for (const auto& default_param : data_container->defaultParameters()) {
    print_value(default_param.second.field().name, default_param.second.value());
  }
  printf("Initial Params:\n");
  for (const auto& default_param : data_container->initialParameters()) {
    print_value(default_param.second.field().name, default_param.second.value());
  }

  return 0;
}
