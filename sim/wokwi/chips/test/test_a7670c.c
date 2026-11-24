#include <stdio.h>
#include <string.h>
#include "host_api_ctl.h"

static void at(const char* s) { host_clear(); host_uart_send(s, (int)strlen(s)); host_uart_send("\r", 1); }
static int has(const char* needle) {
  host_uart_out[host_uart_out_len] = 0;
  return strstr((const char*)host_uart_out, needle) != NULL;
}
static int logged(const char* needle) { host_out[host_out_len] = 0; return strstr(host_out, needle) != NULL; }

int main(void) {
  host_set_attr_str("inbox", "+919876543210|11 5;+917000000001|20");
  host_set_attr("start_epoch", 1795237200);
  host_set_attr("speed", 1);
  chip_init();
  at("AT");
  CHECK(has("AT\r") && has("OK"), "echo on by default, OK");
  at("ATE0");
  at("AT");
  CHECK(!has("AT\r") && has("OK"), "echo off after ATE0");
  at("AT+CPIN?");
  CHECK(has("+CPIN: READY"), "SIM ready");
  at("AT+CEREG?");
  CHECK(has("+CEREG: 0,1"), "registered");
  at("AT+CSQ");
  CHECK(has("+CSQ: 18,99"), "signal");
  // GSM text SMS
  at("AT+CMGF=1"); at("AT+CSCS=\"GSM\"");
  at("AT+CMGS=\"9876543210\"");
  CHECK(has("> "), "SMS prompt");
  host_clear();
  const char body[] = "Shed-1: Temp HIGH 31.2C\x1A";
  host_uart_send(body, (int)strlen(body));
  CHECK(has("+CMGS: 1") && has("OK"), "SMS accepted");
  CHECK(logged("SMS to 9876543210: Shed-1: Temp HIGH 31.2C"), "SMS logged for scenarios");
  // UCS2 SMS: "తేమ" = 0C24 0C47 0C2E
  at("AT+CSCS=\"UCS2\"");
  at("AT+CMGS=\"0039003800370036003500340033003200310030\"");
  const char ub[] = "0C240C470C2E\x1A";
  host_uart_send(ub, (int)strlen(ub));
  CHECK(logged("SMS to 9876543210: తేమ"), "UCS2 decoded to UTF-8");
  // no network
  host_set_attr("network", 0);
  at("AT+CEREG?");
  CHECK(has("+CEREG: 0,2"), "searching when no network");
  at("AT+CSCS=\"GSM\""); at("AT+CMGS=\"9876543210\"");
  CHECK(has("+CMS ERROR: 331"), "SMS refused without network");
  host_set_attr("network", 1);
  // incoming SMS
  host_set_attr("deliver", 1);
  host_clear();
  host_advance(150e6);
  CHECK(has("+CMTI: \"SM\",0"), "new SMS indication");
  at("AT+CMGL=\"REC UNREAD\"");
  CHECK(has("+919876543210") && has("11 5"), "message listed");
  at("AT+CMGL=\"REC UNREAD\"");
  CHECK(!has("11 5"), "listed only once");
  at("AT+CMGD=0,1");
  CHECK(has("OK"), "delete read");
  // clock: 05:00 UTC -> 10:30 IST
  at("AT+CCLK?");
  CHECK(has("+CCLK: \"26/11/21,10:30:00+22\""), "network time in IST with +22 quarter-hours");
  // TLS certificate for MQTT: absent at first, then downloaded
  at("AT+CCERTLIST");
  CHECK(has("OK") && !has("ca.pem"), "no certificate stored yet");
  at("AT+CCERTDOWN=\"ca.pem\",10");
  CHECK(has(">"), "certificate prompt");
  host_clear();
  host_uart_send("0123456789", 10);
  CHECK(has("OK"), "certificate stored");
  at("AT+CCERTLIST");
  CHECK(has("+CCERTLIST: \"ca.pem\""), "certificate listed");
  // MQTT
  at("AT+CMQTTSTART");
  CHECK(has("+CMQTTSTART: 0"), "mqtt start");
  at("AT+CMQTTACCQ=0,\"shed1\",1");
  at("AT+CMQTTCONNECT=0,\"tcp://broker.example:8883\",60,1");
  CHECK(has("+CMQTTCONNECT: 0,0"), "mqtt connected");
  at("AT+CMQTTTOPIC=0,16");
  CHECK(has(">"), "topic prompt");
  host_uart_send("seri/Shed-1/data", 16);
  at("AT+CMQTTPAYLOAD=0,5");
  host_uart_send("hello", 5);
  at("AT+CMQTTPUB=0,1,60");
  CHECK(has("+CMQTTPUB: 0,0"), "published");
  CHECK(logged("MQTT PUBLISH seri/Shed-1/data 5B #1 hello"), "publish logged");
  host_set_attr("data", 0);
  host_clear();
  host_advance(150e6);
  CHECK(has("+CMQTTCONNLOST"), "connection lost reported");
  at("AT+CMQTTPUB=0,1,60");
  CHECK(has("+CMQTTPUB: 0,11"), "publish fails without data");
  at("AT+BOGUS");
  CHECK(has("ERROR"), "unknown command -> ERROR");
  host_out_len = 0;
  at("AT+CCERTDELE=\"ca.pem\"");
  CHECK(has("OK") && !logged("unknown command"), "CCERTDELE accepted");
  fprintf(stderr, "a7670c: %d passed, %d failed\n", passes, fails);
  return fails != 0;
}
