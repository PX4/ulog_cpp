/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include <chrono>
#include <string>
#include <thread>
#include <ulog_cpp/simple_writer.hpp>

// Example of how to create an ULog file with timeseries, printf messages and parameters

using namespace std::chrono_literals;

namespace {
uint64_t currentTimeUs()
{
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
}  // namespace

struct MyData {
  uint64_t timestamp;
  float debug_array[4];
  float cpuload;
  float temperature;
  int8_t counter;

  static std::string messageName() { return "my_data"; }

  static std::vector<ulog_cpp::Field> fields()
  {
    // clang-format off
    return {
        {"uint64_t", "timestamp"}, // Monotonic timestamp in microseconds (since boot), must always be the first field
        {"float", "debug_array", 4},
        {"float", "cpuload"},
        {"float", "temperature"},
        {"int8_t", "counter"},
    };  // clang-format on
  }
};

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("Usage: %s <file.ulg>\n", argv[0]);
    return -1;
  }

  try {
    ulog_cpp::SimpleWriter writer(argv[1], currentTimeUs());
    // See https://docs.px4.io/main/en/dev_log/ulog_file_format.html#i-information-message for
    // well-known keys
    writer.writeInfo("sys_name", "ULogExampleWriter");

    writer.writeParameter("PARAM_A", 382.23F);
    writer.writeParameter("PARAM_B", 8272);

    writer.writeMessageFormat(MyData::messageName(), MyData::fields());
    writer.headerComplete();

    const uint16_t my_data_msg_id = writer.writeAddLoggedMessage(MyData::messageName());

    writer.writeTextMessage(ulog_cpp::Logging::Level::Info, "Hello world", currentTimeUs());

    float cpuload = 25.423F;
    for (int i = 0; i < 100; ++i) {
      MyData data{};
      data.timestamp = currentTimeUs();
      data.cpuload = cpuload;
      data.counter = i;
      writer.writeData(my_data_msg_id, data);
      cpuload -= 0.424F;

      std::this_thread::sleep_for(10ms);
    }
  } catch (const ulog_cpp::ExceptionBase& e) {
    printf("ULog exception: %s\n", e.what());
  }

  return 0;
}
