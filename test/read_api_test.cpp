/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#include <doctest/doctest.h>

#include <filesystem>
#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>
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

TEST_SUITE_BEGIN("[ULog Access]");

TEST_CASE("Write complicated, nested data format, then read it")
{
  std::vector<uint8_t> written_data;
  TestWriter writer([&](const uint8_t* data, int length) {
    const int prev_size = written_data.size();
    written_data.resize(written_data.size() + length);
    memcpy(written_data.data() + prev_size, data, length);
  });

  // Input data
  const ulog_cpp::FileHeader file_header{};
  const ulog_cpp::MessageFormat root_type{"root_type",
                                          {{"uint64_t", "timestamp"},
                                           {"int32_t", "integer"},
                                           {"char", "string", 17},
                                           {"double", "double"},
                                           {"child_1_type", "child_1"}}};

  const ulog_cpp::MessageFormat child_1_type{"child_1_type",
                                             {{"uint32_t", "unsigned_int"},
                                              {"child_1_1_type", "child_1_1"},
                                              {"child_1_2_type", "child_1_2", 3},
                                              {"uint64_t", "unsigned_long", 4}}};

  const ulog_cpp::MessageFormat child_1_1_type{
      "child_1_1_type",
      {{"char", "byte"}, {"char", "string", 19}, {"child_1_1_1_type", "child_1_1_1"}}};

  const ulog_cpp::MessageFormat child_1_1_1_type{"child_1_1_1_type",
                                                 {
                                                     {"int32_t", "integer"},
                                                 }};

  const ulog_cpp::MessageFormat child_1_2_type{"child_1_2_type",
                                               {{"uint8_t", "byte_a"}, {"uint8_t", "byte_b"}}};

  /*
   * t00 [0-8]   timestamp
   * t01 [8-12]  integer
   * t02 [12-29] string
   * t03 [29-37] double
   * t04 [37-41] child_1 / unsigned_int
   * t05 [41-42] child_1 / child_1_1 / byte
   * t06 [42-61] child_1 / child_1_1 / string
   * t07 [61-65] child_1 / child_1_1 / child_1_1_1 / integer
   * t08 [65-66] child_1 / child_1_2 / [0] / byte_a
   * t09 [66-67] child_1 / child_1_2 / [0] / byte_b
   * t10 [67-68] child_1 / child_1_2 / [1] / byte_a
   * t11 [68-69] child_1 / child_1_2 / [1] / byte_b
   * t12 [69-70] child_1 / child_1_2 / [2] / byte_a
   * t13 [70-71] child_1 / child_1_2 / [2] / byte_b
   * t14 [71-103] child_1 / unsigned_long[4]
   */

  std::vector<uint8_t> data_vector;
  data_vector.resize(103);

  const uint64_t t00 = 0xdeadbeefdeadbeef;
  const int32_t t01 = -123456;
  const char t02[] = "Hello World!";
  const double t03 = 3.14159265358979323846;
  const uint32_t t04 = 0xdeadbeef;
  const char t05 = 'a';
  const char t06[] = "Hello World! 2";
  const int32_t t07 = 123456;
  const uint8_t t08 = 0x12;
  const uint8_t t09 = 0x34;
  const uint8_t t10 = 0x56;
  const uint8_t t11 = 0x78;
  const uint8_t t12 = 0x9a;
  const uint8_t t13 = 0xbc;
  const std::vector<uint64_t> t14 = {0xfeedc0defeedc0d0, 0xfeedc0defeedc0d1, 0xfeedc0defeedc0d2,
                                     0xfeedc0defeedc0d3};

  memcpy(data_vector.data() + 0, &t00, 8);
  memcpy(data_vector.data() + 8, &t01, 4);
  memcpy(data_vector.data() + 12, &t02, 17);
  memcpy(data_vector.data() + 29, &t03, 8);
  memcpy(data_vector.data() + 37, &t04, 4);
  memcpy(data_vector.data() + 41, &t05, 1);
  memcpy(data_vector.data() + 42, &t06, 19);
  memcpy(data_vector.data() + 61, &t07, 4);
  memcpy(data_vector.data() + 65, &t08, 1);
  memcpy(data_vector.data() + 66, &t09, 1);
  memcpy(data_vector.data() + 67, &t10, 1);
  memcpy(data_vector.data() + 68, &t11, 1);
  memcpy(data_vector.data() + 69, &t12, 1);
  memcpy(data_vector.data() + 70, &t13, 1);
  memcpy(data_vector.data() + 71, t14.data(), 8 * 4);

  const ulog_cpp::MessageInfo info{{"root_type", "info"}, data_vector};

  const ulog_cpp::AddLoggedMessage add_logged_message_1{0, 1, "root_type"};
  const ulog_cpp::AddLoggedMessage add_logged_message_2{1, 2, "root_type"};
  const ulog_cpp::Data data_1{1, data_vector};
  const ulog_cpp::Data data_2{2, data_vector};

  // Write data
  writer.fileHeader(file_header);
  writer.messageInfo(info);
  writer.messageFormat(child_1_1_1_type);
  writer.messageFormat(root_type);
  writer.messageFormat(child_1_type);
  writer.messageFormat(child_1_1_type);
  writer.messageFormat(child_1_2_type);

  writer.headerComplete();
  writer.messageInfo(info);
  writer.addLoggedMessage(add_logged_message_1);
  writer.addLoggedMessage(add_logged_message_2);
  writer.data(data_1);
  writer.data(data_1);
  writer.data(data_2);
  writer.data(data_2);
  writer.data(data_2);

  REQUIRE_GT(written_data.size(), 0);
  REQUIRE_EQ(writer.num_errors, 0);

  // Read it
  const auto data_container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader{data_container};
  reader.readChunk(written_data.data(), written_data.size());

  // Check for errors
  REQUIRE(data_container->parsingErrors().empty());
  REQUIRE_FALSE(data_container->hadFatalError());

  auto subscription_names = data_container->subscriptionNames();
  CHECK_EQ(subscription_names.size(), 1);
  CHECK_EQ(*(subscription_names.begin()), "root_type");

  auto subscription_1 = data_container->subscription("root_type");
  auto subscription_2 = data_container->subscription("root_type", 1);

  CHECK_EQ(subscription_1->size(), 2);
  CHECK_EQ(subscription_2->size(), 3);

  // check access with string selectors
  for (const auto& sample : *subscription_1) {
    CHECK_EQ(sample["timestamp"].as<uint64_t>(), t00);
    CHECK_EQ(sample["integer"].as<int32_t>(), t01);
    CHECK_EQ(sample["string"].as<std::string>(), t02);
    CHECK_EQ(sample["double"].as<double>(), t03);
    CHECK_EQ(sample["child_1"]["unsigned_int"].as<uint32_t>(), t04);
    CHECK_EQ(sample["child_1"]["child_1_1"]["byte"].as<uint8_t>(), t05);
    CHECK_EQ(sample["child_1"]["child_1_1"]["string"].as<std::string>(), t06);
    CHECK_EQ(sample["child_1"]["child_1_1"]["child_1_1_1"]["integer"].as<int32_t>(), t07);
    CHECK_EQ(sample["child_1"]["child_1_2"][0]["byte_a"].as<uint8_t>(), t08);
    CHECK_EQ(sample["child_1"]["child_1_2"][0]["byte_b"].as<uint8_t>(), t09);
    CHECK_EQ(sample["child_1"]["child_1_2"][1]["byte_a"].as<uint8_t>(), t10);
    CHECK_EQ(sample["child_1"]["child_1_2"][1]["byte_b"].as<uint8_t>(), t11);
    CHECK_EQ(sample["child_1"]["child_1_2"][2]["byte_a"].as<uint8_t>(), t12);
    CHECK_EQ(sample["child_1"]["child_1_2"][2]["byte_b"].as<uint8_t>(), t13);
    CHECK_EQ(sample["child_1"]["unsigned_long"].as<std::vector<uint64_t>>(), t14);
  }

  // check access with field selectors
  auto f_timestamp = subscription_1->field("timestamp");
  auto f_integer = subscription_1->field("integer");
  auto f_string = subscription_1->field("string");
  auto f_double = subscription_1->field("double");
  auto f_child_1 = subscription_1->field("child_1");
  auto f_c1_unsinged_int = f_child_1->nestedField("unsigned_int");
  auto f_c1_c1_1 = f_child_1->nestedField("child_1_1");
  auto f_c1_c1_1_byte = f_c1_c1_1->nestedField("byte");
  auto f_c1_c1_1_string = f_c1_c1_1->nestedField("string");
  auto f_c1_c1_1_c1_1_1 = f_c1_c1_1->nestedField("child_1_1_1");
  auto f_c1_c1_1_c1_1_1_integer = f_c1_c1_1_c1_1_1->nestedField("integer");
  auto f_c1_c1_2 = f_child_1->nestedField("child_1_2");
  auto f_c1_c1_2_byte_a = f_c1_c1_2->nestedField("byte_a");
  auto f_c1_c1_2_byte_b = f_c1_c1_2->nestedField("byte_b");
  auto f_c1_unsigned_long = f_child_1->nestedField("unsigned_long");

  for (const auto& sample : *subscription_2) {
    CHECK_EQ(sample[f_timestamp].as<uint64_t>(), t00);
    CHECK_EQ(sample[f_integer].as<int32_t>(), t01);
    CHECK_EQ(sample[f_string].as<std::string>(), t02);
    CHECK_EQ(sample[f_double].as<double>(), t03);
    CHECK_EQ(sample[f_child_1][f_c1_unsinged_int].as<uint32_t>(), t04);
    CHECK_EQ(sample[f_child_1][f_c1_c1_1][f_c1_c1_1_byte].as<uint8_t>(), t05);
    CHECK_EQ(sample[f_child_1][f_c1_c1_1][f_c1_c1_1_string].as<std::string>(), t06);
    CHECK_EQ(sample[f_child_1][f_c1_c1_1][f_c1_c1_1_c1_1_1][f_c1_c1_1_c1_1_1_integer].as<int32_t>(),
             t07);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][0][f_c1_c1_2_byte_a].as<uint8_t>(), t08);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][0][f_c1_c1_2_byte_b].as<uint8_t>(), t09);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][1][f_c1_c1_2_byte_a].as<uint8_t>(), t10);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][1][f_c1_c1_2_byte_b].as<uint8_t>(), t11);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][2][f_c1_c1_2_byte_a].as<uint8_t>(), t12);
    CHECK_EQ(sample[f_child_1][f_c1_c1_2][2][f_c1_c1_2_byte_b].as<uint8_t>(), t13);
    CHECK_EQ(sample[f_child_1][f_c1_unsigned_long].as<std::vector<uint64_t>>(), t14);
  }

  // check type conversions
  auto sample = *subscription_2->begin();
  CHECK_EQ(sample[f_timestamp].as<int32_t>(), static_cast<int32_t>(t00));
  CHECK_EQ(sample[f_timestamp].as<int16_t>(), static_cast<int16_t>(t00));
  CHECK_EQ(sample[f_timestamp].as<double>(), static_cast<double>(t00));
  CHECK_EQ(sample[f_timestamp].as<std::vector<uint64_t>>(), std::vector<uint64_t>{t00});
  CHECK_EQ(sample[f_timestamp].as<std::vector<int>>(), std::vector<int>{static_cast<int>(t00)});

  CHECK_EQ(sample[f_child_1][f_c1_unsigned_long].as<uint64_t>(), t14[0]);
  CHECK_EQ(sample[f_child_1][f_c1_unsigned_long][1].as<int64_t>(), static_cast<int64_t>(t14[1]));
  CHECK_EQ(sample[f_child_1][f_c1_unsigned_long][1].as<std::vector<int>>(),
           std::vector<int>{static_cast<int>(t14[1])});

  CHECK(std::holds_alternative<uint64_t>(sample[f_timestamp].asNativeTypeVariant()));
  CHECK(std::holds_alternative<int32_t>(sample[f_integer].asNativeTypeVariant()));
  CHECK(std::holds_alternative<std::string>(sample[f_string].asNativeTypeVariant()));
  CHECK(std::holds_alternative<double>(sample[f_double].asNativeTypeVariant()));
  CHECK(
      std::holds_alternative<uint32_t>(sample[f_child_1][f_c1_unsinged_int].asNativeTypeVariant()));
  CHECK(std::holds_alternative<char>(
      sample[f_child_1][f_c1_c1_1][f_c1_c1_1_byte].asNativeTypeVariant()));
  CHECK(std::holds_alternative<std::string>(
      sample[f_child_1][f_c1_c1_1][f_c1_c1_1_string].asNativeTypeVariant()));
  CHECK(std::holds_alternative<int32_t>(
      sample[f_child_1][f_c1_c1_1][f_c1_c1_1_c1_1_1][f_c1_c1_1_c1_1_1_integer]
          .asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][0][f_c1_c1_2_byte_a].asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][0][f_c1_c1_2_byte_b].asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][1][f_c1_c1_2_byte_a].asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][1][f_c1_c1_2_byte_b].asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][2][f_c1_c1_2_byte_a].asNativeTypeVariant()));
  CHECK(std::holds_alternative<uint8_t>(
      sample[f_child_1][f_c1_c1_2][2][f_c1_c1_2_byte_b].asNativeTypeVariant()));
  CHECK(std::holds_alternative<std::vector<uint64_t>>(
      sample[f_child_1][f_c1_unsigned_long].asNativeTypeVariant()));
}

TEST_SUITE_END();
