/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#include <doctest/doctest.h>

#include <filesystem>
#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>
#include <ulog_cpp/simple_writer.hpp>
#include <ulog_cpp/writer.hpp>
#include <vector>

class TestWriter : public ulog_cpp::Writer {
 public:
  explicit TestWriter(const ulog_cpp::DataWriteCB& cb) : ulog_cpp::Writer(cb) {}

  ~TestWriter() override = default;

  void error(const std::string& msg, bool is_recoverable) override { ++num_errors; }

  int num_errors{0};

 private:
};

TEST_SUITE_BEGIN("[ULog Parsing]");

TEST_CASE("ULog parsing - basic test: write then read")
{
  std::vector<uint8_t> written_data;
  TestWriter writer([&](const uint8_t* data, int length) {
    const int prev_size = written_data.size();
    written_data.resize(written_data.size() + length);
    memcpy(written_data.data() + prev_size, data, length);
  });

  // Input data
  const ulog_cpp::FileHeader file_header{};
  const ulog_cpp::MessageFormat format1{"message_name",
                                        {{"uint64_t", "timestamp"}, {"float", "float_value"}}};
  const ulog_cpp::MessageFormat format2{
      "other_message", {{"uint64_t", "timestamp"}, {"uint32_t", "array", 3}, {"uint16_t", "x"}}};
  std::vector<uint8_t> data_vector;
  data_vector.resize(22);
  data_vector[0] = 32;
  data_vector[20] = 49;
  const ulog_cpp::MessageInfo info{"info", "test_value"};
  const ulog_cpp::Logging logging{ulog_cpp::Logging::Level::Warning, "logging message", 3'834'732};
  const uint16_t msg_id = 1;
  const ulog_cpp::AddLoggedMessage add_logged_message{0, msg_id, "other_message"};
  const ulog_cpp::Data data{msg_id, data_vector};

  // Write data
  writer.fileHeader(file_header);
  writer.messageInfo(info);
  writer.messageFormat(format1);
  writer.messageFormat(format2);
  writer.headerComplete();
  writer.logging(logging);
  writer.addLoggedMessage(add_logged_message);
  writer.data(data);
  writer.data(data);

  REQUIRE_GT(written_data.size(), 0);
  REQUIRE_EQ(writer.num_errors, 0);

  // // Test: write to disk
  // FILE* file = fopen("test.ulg", "wb");
  // fwrite(written_data.data(), 1, written_data.size(), file);
  // fclose(file);

  // Read it
  const auto data_container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader{data_container};
  reader.readChunk(written_data.data(), written_data.size());

  // Check for errors
  REQUIRE(data_container->parsingErrors().empty());
  REQUIRE_FALSE(data_container->hadFatalError());

  // Check RAW API
  CHECK_EQ(file_header, data_container->fileHeader());
  CHECK_EQ(format1, *(data_container->messageFormats().at("message_name")));
  CHECK_EQ(format2, *(data_container->messageFormats().at("other_message")));
  CHECK_EQ(info, data_container->messageInfo().at("info"));
  REQUIRE_EQ(data_container->logging().size(), 1);
  CHECK_EQ(logging, data_container->logging()[0]);
  CHECK_EQ(data_container->subscriptionsByMessageId().at(msg_id)->rawSamples().size(), 2);
  CHECK_EQ(data, data_container->subscriptionsByMessageId().at(msg_id)->rawSamples()[0]);
  CHECK_EQ(data, data_container->subscriptionsByMessageId().at(msg_id)->rawSamples()[1]);

  // Check convenience API
  CHECK_EQ(format2, *(data_container->subscription("other_message")->format()));
  CHECK_EQ(data_container->subscription("other_message")->size(), 2);

  auto timestamp_field = data_container->subscription("other_message")->field("timestamp");
  auto x_field = data_container->subscription("other_message")->field("x");

  for (const auto& sample : *(data_container->subscription("other_message"))) {
    // field access
    CHECK_EQ(sample[timestamp_field].as<int>(), 32);
    CHECK_EQ(sample[x_field].as<int>(), 49);
    // direct string access
    CHECK_EQ(sample["timestamp"].as<int>(), 32);
    CHECK_EQ(sample["x"].as<int>(), 49);
  }
}

namespace {
void readFileWriteTest(const std::filesystem::path& path,
                       const std::function<unsigned()>& next_chunk_size)
{
  FILE* file = fopen(path.c_str(), "rb");
  CHECK(file);

  std::vector<uint8_t> written_data;
  auto writer = std::make_shared<TestWriter>([&](const uint8_t* data, int length) {
    const int prev_size = written_data.size();
    written_data.resize(written_data.size() + length);
    memcpy(written_data.data() + prev_size, data, length);
  });

  std::vector<uint8_t> input_data;
  uint8_t buffer[4048];
  int bytes_read;
  ulog_cpp::Reader reader{writer};
  while ((bytes_read =
              fread(buffer, 1, std::min<unsigned>(next_chunk_size(), sizeof(buffer)), file)) > 0) {
    input_data.resize(input_data.size() + bytes_read);
    memcpy(input_data.data() + input_data.size() - bytes_read, buffer, bytes_read);
    reader.readChunk(buffer, bytes_read);
  }
  fclose(file);

  //   // Test: write to disk
  //   FILE* file_test = fopen(path.filename().c_str(), "wb");
  //   fwrite(written_data.data(), 1, written_data.size(), file_test);
  //   fclose(file_test);

  REQUIRE_GT(written_data.size(), 0);
  REQUIRE_GT(input_data.size(), 0);
  REQUIRE_EQ(writer->num_errors, 0);
  CHECK_EQ(input_data.size(), written_data.size());
  CHECK_EQ(input_data, written_data);
}
}  // namespace

TEST_CASE("ULog parsing - read sample files then write")
{
  // Iterate over test log files
  const std::string src_file_path = __FILE__;
  const std::string test_file_dir =
      src_file_path.substr(0, src_file_path.rfind('/')) + "/log_files";
  printf("Log files dir: %s\n", test_file_dir.c_str());

  bool found_logs = false;
  bool test_min_chunk_size = true;
  for (const auto& dir_entry : std::filesystem::directory_iterator(test_file_dir)) {
    if (dir_entry.is_directory() || dir_entry.path().extension() != ".ulg") {
      continue;
    }
    found_logs = true;
    printf("Testing file %s\n", dir_entry.path().filename().string().c_str());

    // Iterate over chunk sizes
    const int first_chunk_size = 100;  // always include magic+flags messages in the first chunk
    const int chunk_sizes[] = {1, 5, 1024, 4048};
    for (const auto& chunk_size : chunk_sizes) {
      if (!test_min_chunk_size && chunk_size <= 3) {
        continue;
      }
      printf("Read chunk size: %i\n", chunk_size);
      bool first = true;
      readFileWriteTest(dir_entry, [&]() {
        if (first) {
          first = false;
          return first_chunk_size;
        }
        return chunk_size;
      });
    }
    test_min_chunk_size = false;  // only test small chunk size for first log, as it's slow
  }

  REQUIRE(found_logs);
}

TEST_CASE("ULog parsing - test corruption")
{
  // Corrupt the stream by inserting zero bytes
  int insert_zero_bytes = 0;
  std::vector<uint8_t> written_data;
  TestWriter writer([&](const uint8_t* data, int length) {
    if (insert_zero_bytes > 0) {
      printf("inserting zero bytes, num written: %zu\n", written_data.size());
      written_data.resize(written_data.size() + insert_zero_bytes);
      insert_zero_bytes = 0;
    }
    const int prev_size = written_data.size();
    written_data.resize(written_data.size() + length);
    memcpy(written_data.data() + prev_size, data, length);
  });

  // Input data
  const ulog_cpp::FileHeader file_header{};
  const ulog_cpp::MessageFormat format2{
      "other_message", {{"uint64_t", "timestamp"}, {"uint32_t", "array", 3}, {"uint16_t", "x"}}};
  std::vector<uint8_t> data_vector;
  data_vector.resize(22);
  data_vector[0] = 32;
  data_vector[20] = 49;
  const ulog_cpp::Logging logging{ulog_cpp::Logging::Level::Warning, "logging message", 3'834'732};
  const uint16_t msg_id = 1;
  const ulog_cpp::AddLoggedMessage add_logged_message{0, msg_id, "other_message"};
  const ulog_cpp::Data data{msg_id, data_vector};

  // Write data
  writer.fileHeader(file_header);
  writer.messageFormat(format2);
  writer.headerComplete();
  insert_zero_bytes = 423;
  writer.logging(logging);
  writer.addLoggedMessage(add_logged_message);
  writer.data(data);
  writer.data(data);

  REQUIRE_GT(written_data.size(), 0);
  REQUIRE_EQ(writer.num_errors, 0);

  // Read it
  const auto data_container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader{data_container};
  // Read in multiple chunks, as the parser needs more readChunk calls to recover.
  // We could change the parser to handle it, but in practice this should not be an issue.
  const int last_chunk_size = 30;
  reader.readChunk(written_data.data(), written_data.size() - last_chunk_size);
  reader.readChunk(written_data.data() + written_data.size() - last_chunk_size, last_chunk_size);

  // Expected to have errors
  CHECK_GT(data_container->parsingErrors().size(), 0);
  REQUIRE_FALSE(data_container->hadFatalError());

  // Compare
  CHECK_EQ(file_header, data_container->fileHeader());
  CHECK_EQ(format2, *(data_container->messageFormats().at("other_message")));
  // Here we inserted zero bytes, but the next messages should not be dropped
  REQUIRE_EQ(data_container->logging().size(), 1);
  CHECK_EQ(logging, data_container->logging()[0]);
  CHECK_EQ(data_container->subscriptionsByMessageId().at(msg_id)->rawSamples().size(), 2);
  CHECK_EQ(data, data_container->subscriptionsByMessageId().at(msg_id)->rawSamples()[0]);
  CHECK_EQ(data, data_container->subscriptionsByMessageId().at(msg_id)->rawSamples()[1]);

  auto timestamp_field = data_container->subscription("other_message")->field("timestamp");
  auto x_field = data_container->subscription("other_message")->field("x");

  for (const auto& sample : *(data_container->subscription("other_message"))) {
    // field access
    CHECK_EQ(sample[timestamp_field].as<int>(), 32);
    CHECK_EQ(sample[x_field].as<int>(), 49);
    // direct string access
    CHECK_EQ(sample["timestamp"].as<int>(), 32);
    CHECK_EQ(sample["x"].as<int>(), 49);
  }
}

struct MyData {
  uint64_t timestamp;
  float debug_array[4];
  float cpuload;
  float temperature;
  int8_t counter;

  bool operator==(const MyData& other) const
  {
    for (unsigned i = 0; i < sizeof(debug_array) / sizeof(debug_array[0]); ++i) {
      if (debug_array[i] != other.debug_array[i]) {
        return false;
      }
    }
    return timestamp == other.timestamp && cpuload == other.cpuload &&
           temperature == other.temperature && counter == other.counter;
  }

  static std::string messageName() { return "my_data"; }

  static std::vector<ulog_cpp::Field> fields()
  {
    // clang-format off
    return {
        {"uint64_t", "timestamp"},
        {"float", "debug_array", 4},
        {"float", "cpuload"},
        {"float", "temperature"},
        {"int8_t", "counter"},
    };  // clang-format on
  }
};

TEST_CASE("ULog parsing - simple writer")
{
  std::vector<uint8_t> written_data;
  ulog_cpp::SimpleWriter writer(
      [&](const uint8_t* data, int length) {
        const int prev_size = written_data.size();
        written_data.resize(written_data.size() + length);
        memcpy(written_data.data() + prev_size, data, length);
      },
      0);

  const std::string sys_name = "ULogExampleWriter";
  writer.writeInfo("sys_name", sys_name);

  const float param_a = 382.23F;
  const int32_t param_b = 8272;
  writer.writeParameter("PARAM_A", param_a);
  writer.writeParameter("PARAM_B", param_b);

  CHECK_THROWS(writer.writeMessageFormat("invalid_require_padding", {
                                                                        {"uint64_t", "timestamp"},
                                                                        {"int8_t", "a"},
                                                                        {"float", "b"},
                                                                    }));

  CHECK_THROWS(
      writer.writeMessageFormat("invalid_type", {{"uint64_t", "timestamp"}, {"my_type", "a"}}));

  CHECK_THROWS(writer.writeMessageFormat("invalid_no_timestamp", {{"int8_t", "a"}}));

  CHECK_THROWS(writer.writeMessageFormat("invalid_field_name",
                                         {{"uint64_t", "timestamp"}, {"int8_t", "a/b"}}));

  writer.writeMessageFormat(MyData::messageName(), MyData::fields());
  writer.headerComplete();

  const uint16_t my_data_msg_id = writer.writeAddLoggedMessage(MyData::messageName());

  const std::string text_message = "Hello world";
  writer.writeTextMessage(ulog_cpp::Logging::Level::Info, text_message, 0);

  // Write some messages
  float cpuload = 25.423F;
  std::vector<MyData> written_data_messages;
  for (int i = 0; i < 100; ++i) {
    MyData data{};
    data.timestamp = static_cast<uint64_t>(i) * 1000;
    data.cpuload = cpuload;
    data.counter = i;
    writer.writeData(my_data_msg_id, data);
    written_data_messages.push_back(data);
    cpuload -= 0.424F;
  }

  // Parse data
  const auto data_container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader{data_container};
  reader.readChunk(written_data.data(), written_data.size());

  // Check for errors
  REQUIRE(data_container->parsingErrors().empty());
  REQUIRE_FALSE(data_container->hadFatalError());

  // Compare
  CHECK_EQ(sys_name, data_container->messageInfo().at("sys_name").value().as<std::string>());
  REQUIRE_EQ(data_container->logging().size(), 1);
  CHECK_EQ(text_message, data_container->logging()[0].message());
  CHECK_EQ(param_a, data_container->initialParameters().at("PARAM_A").value().as<float>());
  CHECK_EQ(param_b, (data_container->initialParameters().at("PARAM_B").value().as<int32_t>()));

  CHECK_EQ(MyData::messageName(),
           data_container->messageFormats().at(MyData::messageName())->name());
  REQUIRE_EQ(data_container->subscriptionNames().size(), 1);

  const auto subscription = data_container->subscription(MyData::messageName());
  REQUIRE_EQ(subscription->size(), written_data_messages.size());
  for (size_t i = 0; i < subscription->size(); ++i) {
    const auto& sample = (*subscription)[i];
    const auto& gt = written_data_messages[i];
    MyData memcopied_sample{};
    REQUIRE_GE(sizeof(MyData), sample.rawData().size());
    // check raw byte equality
    memcpy(&memcopied_sample, sample.rawData().data(), sample.rawData().size());
    CHECK_EQ(gt, memcopied_sample);
    // check API access field equality
    CHECK_EQ(gt.timestamp, sample["timestamp"].as<uint64_t>());
    CHECK_EQ(gt.cpuload, sample["cpuload"].as<float>());
    CHECK_EQ(gt.counter, sample["counter"].as<int8_t>());
  }
}

TEST_SUITE_END();
