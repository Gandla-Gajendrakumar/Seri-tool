// F-UP-04: per-device MQTT broker settings, provisioned from a file on the microSD card (D-060).
#include <unity.h>
#include "seri/provision.h"
using namespace seri;

void setUp() {}
void tearDown() {}

static const char* kPem = "-----BEGIN CERTIFICATE-----\nMIIBszCCAVmgAwIBAgIUQ0FURVNU\n-----END CERTIFICATE-----\n";

static void test_parses_a_complete_file_with_comments_and_crlf() {
  MqttConfig c; std::string err;
  TEST_ASSERT_TRUE(parseProvisioning("# broker\r\nmqtt_host=broker.example.in\r\nmqtt_port=8883\r\n\r\nmqtt_user=shed-17\r\nmqtt_pass=s3cret=x\r\nclient_id=seri-0017\r\n", c, err));
  TEST_ASSERT_EQUAL_STRING("broker.example.in", c.host.c_str());
  TEST_ASSERT_EQUAL(8883, c.port);
  TEST_ASSERT_EQUAL_STRING("shed-17", c.user.c_str());
  TEST_ASSERT_EQUAL_STRING("s3cret=x", c.pass.c_str());   // '=' allowed inside the value
  TEST_ASSERT_EQUAL_STRING("seri-0017", c.clientId.c_str());
}

static void test_port_defaults_to_8883() {
  MqttConfig c; std::string err;
  TEST_ASSERT_TRUE(parseProvisioning("mqtt_host=b.example.in\n", c, err));
  TEST_ASSERT_EQUAL(8883, c.port);
}

static void test_rejects_bad_files_with_a_reason() {
  MqttConfig c; std::string err;
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_port=8883\n", c, err));            // no host
  TEST_ASSERT_EQUAL_STRING("mqtt_host missing", err.c_str());
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_host=a b.in\n", c, err));          // space in host
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_host=b.in\nmqtt_port=0\n", c, err));
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_host=b.in\nmqtt_port=70000\n", c, err));
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_host=b.in\nmqtt_prot=8883\n", c, err));  // typo caught
  TEST_ASSERT_EQUAL_STRING("unknown key: mqtt_prot", err.c_str());
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_host b.in\n", c, err));            // no '='
}

static void test_failed_parse_leaves_config_untouched() {
  MqttConfig c; c.host = "keep.example.in"; std::string err;
  TEST_ASSERT_FALSE(parseProvisioning("mqtt_port=1\n", c, err));
  TEST_ASSERT_EQUAL_STRING("keep.example.in", c.host.c_str());
}

static void test_pem_validation() {
  TEST_ASSERT_TRUE(validPem(kPem));
  TEST_ASSERT_FALSE(validPem("hello"));
  TEST_ASSERT_FALSE(validPem("-----BEGIN CERTIFICATE-----\nabc\n"));
  TEST_ASSERT_FALSE(validPem(std::string("-----BEGIN CERTIFICATE-----\n") + std::string(5000, 'A') + "\n-----END CERTIFICATE-----\n"));
}

static void test_encode_decode_round_trip() {
  MqttConfig c; c.host = "b.example.in"; c.port = 1883; c.user = "u"; c.pass = "p\nq"; c.clientId = "id"; c.caPem = kPem;
  MqttConfig d;
  TEST_ASSERT_TRUE(decodeMqtt(encodeMqtt(c), d));
  TEST_ASSERT_EQUAL_STRING(c.host.c_str(), d.host.c_str());
  TEST_ASSERT_EQUAL(1883, d.port);
  TEST_ASSERT_EQUAL_STRING("p\nq", d.pass.c_str());
  TEST_ASSERT_EQUAL_STRING(kPem, d.caPem.c_str());
  TEST_ASSERT_FALSE(decodeMqtt("garbage", d));
}

static void test_valid_needs_host_and_ca() {
  MqttConfig c;
  TEST_ASSERT_FALSE(c.valid());
  c.host = "b.example.in";
  TEST_ASSERT_FALSE(c.valid());   // TLS without a CA certificate is refused
  c.caPem = kPem;
  TEST_ASSERT_TRUE(c.valid());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parses_a_complete_file_with_comments_and_crlf);
  RUN_TEST(test_port_defaults_to_8883);
  RUN_TEST(test_rejects_bad_files_with_a_reason);
  RUN_TEST(test_failed_parse_leaves_config_untouched);
  RUN_TEST(test_pem_validation);
  RUN_TEST(test_encode_decode_round_trip);
  RUN_TEST(test_valid_needs_host_and_ca);
  return UNITY_END();
}
