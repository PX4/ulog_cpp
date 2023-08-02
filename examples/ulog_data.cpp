/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <fstream>
#include <iostream>
#include <string>
#include <variant>

#include "data_container.hpp"
#include "reader.hpp"

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
  // TODO: create a simpler API for this
  const std::string message = "multirotor_motor_limits";
  printf("%s timestamps: ", message.c_str());
  for (const auto& sub : data_container->subscriptions()) {
    if (sub.second.add_logged_message.messageName() == message) {
      const auto& fields = data_container->messageFormats().at(message).fields();
      // Expect the first field to be the timestamp
      if (fields[0].name != "timestamp") {
        printf("Error: first field is not 'timestamp'\n");
        return -1;
      }
      for (const auto& data : sub.second.data) {
        auto value = ulog_cpp::Value(
            fields[0],
            std::vector<uint8_t>(data.data().begin(), data.data().begin() + sizeof(uint64_t)));
        printf("%lu, ", std::get<uint64_t>(value.data()));
      }
    }
  }
  printf("\n");

  return 0;
}
