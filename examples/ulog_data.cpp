/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <fstream>
#include <iostream>
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

  // Read out some data

  // List all subscription names
  for (const auto& sub : data_container->subscriptionNames()) {
    std::cout << sub << std::endl;
  }

  // Get a particular subscription
  const auto& subscription = data_container->subscription("vehicle_status");

  // Get message format of subscription
  auto message_format = subscription->format();
  std::cout << "Message format: " << message_format->name() << std::endl;

  // List all field names
  for (const std::string& field : subscription->fieldNames()) {
    std::cout << field << std::endl;
  }

  // Get particular field
  auto nav_state_field = subscription->field("nav_state");

  // Iterate over all samples
  for (const auto& sample : *subscription) {
    // always correctly extracts the type as defined in the message definition,
    // gets cast to the value you put in int.
    // This also works for arrays and strings.
    auto nav_state = sample[nav_state_field].as<int>();
    std::cout << nav_state << std::endl;
  }

  // get a specific sample
  auto sample_12 = subscription->at(12);

  // access values by name
  auto timestamp = sample_12["timestamp"].as<uint64_t>();

  std::cout << timestamp << std::endl;

  // get from nested data type
  auto esc_format =
      data_container->messageFormats().at("esc_status")->field("esc")->type().nested_message;
  for (const auto& field_name : esc_format->fieldNames()) {
    std::cout << field_name << std::endl;
  }

  auto esc_status = data_container->subscription("esc_status");
  for (const auto& sample : *esc_status) {
    std::cout << "timestamp: " << sample["esc"][7]["esc_power"].as<int>() << std::endl;
  }
  return 0;
}
