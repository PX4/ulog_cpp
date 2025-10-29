/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <fstream>
#include <iostream>
#include <string>
#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

// Example of how to use the typed data API for accessing topic data

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

  // Read out some data

  // List all subscription names
  auto subscription_names = data_container->subscriptionNames();
  for (const auto& sub : subscription_names) {
    std::cout << sub << "\n";
  }
  std::cout << "\n";

  // Get a particular subscription
  if (subscription_names.find("vehicle_status") != subscription_names.end()) {
    const auto& subscription = data_container->subscription("vehicle_status");

    // Get message format of subscription
    auto message_format = subscription->format();
    std::cout << "Message format: " << message_format->name() << "\n";

    // List all field names
    std::cout << "Field names: "
              << "\n";
    for (const std::string& field : subscription->fieldNames()) {
      std::cout << "  " << field << "\n";
    }

    // Get particular field
    try {
      auto nav_state_field = subscription->field("nav_state");

      // Iterate over all samples
      std::cout << "nav_state values: \n  ";
      for (const auto& sample : *subscription) {
        // always correctly extracts the type as defined in the message definition,
        // gets cast to the value you put in int.
        // This also works for arrays and strings.
        auto nav_state = sample[nav_state_field].as<int>();
        std::cout << nav_state << ", ";
      }
      std::cout << "\n";

      // get a specific sample
      auto sample_12 = subscription->at(12);

      // access values by name
      auto timestamp = sample_12["timestamp"].as<uint64_t>();

      std::cout << "timestamp at sample 12: " << timestamp << "\n";
    } catch (const ulog_cpp::AccessException& exception) {
      std::cout << "AccessException: " << exception.what() << "\n";
    }
  } else {
    std::cout << "No vehicle_status subscription found\n";
  }

  if (data_container->messageFormats().find("esc_status") !=
      data_container->messageFormats().end()) {
    const auto& message_format = data_container->messageFormats().at("esc_status");
    std::cout << "Message format: " << message_format->name() << "\n";
    for (const auto& field_name : message_format->fieldNames()) {
      std::cout << "  " << field_name << "\n";
    }
  } else {
    std::cout << "No esc_status message format found\n";
  }

  if (subscription_names.find("esc_status") != subscription_names.end()) {
    try {
      auto esc_status = data_container->subscription("esc_status");
      for (const auto& sample : *esc_status) {
        std::cout << "esc_power: " << sample["esc"][7]["esc_power"].as<int>() << "\n";
      }
    } catch (const ulog_cpp::AccessException& exception) {
      std::cout << "AccessException: " << exception.what() << "\n";
    }
  } else {
    std::cout << "No esc_status subscription found\n";
  }
  return 0;
}
