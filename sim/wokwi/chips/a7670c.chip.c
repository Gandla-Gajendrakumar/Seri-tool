// Virtual SIMCom A7670C LTE Cat-1 modem: an AT-command responder over UART (115200 8N1).
// Implements the subset of the A76XX AT command set the firmware driver uses:
//   AT, ATE0/1, AT+CPIN?, AT+CSQ, AT+CREG?/CEREG?/CGREG?, AT+CMGF, AT+CSCS, AT+CSMP, AT+CNMI,
//   AT+CMGS (text mode, GSM or UCS2 hex), AT+CMGL, AT+CMGD, AT+CCLK?, AT+CTZU,
//   AT+CMQTTSTART/ACCQ/SSLCFG/CONNECT/TOPIC/PAYLOAD/PUB/DISC/REL/STOP, AT+CSSLCFG, AT+CGDCONT.
// Every SMS sent is printed as   "SMS to <number>: <text in UTF-8>"
// Every MQTT publish is printed  "MQTT PUBLISH <topic> <bytes>B #<count>"
// so scenarios can assert on them with expect-chip-output.
// Controls: network (0 = no service), data (0 = no packet data), signal (0-31),
//           deliver (raise to N to deliver the first N messages of attr "inbox": "num|text;num|text").
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX 1200
#define OUT_MAX 8192
#define INBOX_MAX 8

typedef enum { M_CMD, M_SMS_BODY, M_RAW } mode_t2;

typedef struct { char from[24]; char text[200]; bool unread, used; } sms_t;

typedef struct {
  uart_dev_t uart;
  uint32_t a_network, a_data, a_signal, a_deliver;
  char line[LINE_MAX];
  int len;
  mode_t2 mode;
  int raw_need;          // bytes still expected in M_RAW
  int raw_target;        // 0 = topic, 1 = payload, 2 = certificate
  char topic[256], payload[LINE_MAX];
  int raw_len;
  bool echo, ucs2, mqtt_started, mqtt_connected, have_cert;
  char cert_name[32];
  char sms_to[64];
  uint8_t out[OUT_MAX];
  int out_head, out_tail;
  bool writing;
  sms_t inbox[INBOX_MAX];
  char script[2048];
  int delivered;
  int sms_count, pub_count, mr;
  double base_epoch, base_nanos, speed;
  timer_t timer;
} chip_t;

static chip_t* g;

// ---------- output queue (uart_write is asynchronous) ----------
static void kick(chip_t* c) {
  if (c->writing || c->out_head == c->out_tail) return;
  static uint8_t buf[256];
  int n = 0;
  while (c->out_tail != c->out_head && n < (int)sizeof buf) {
    buf[n++] = c->out[c->out_tail];
    c->out_tail = (c->out_tail + 1) % OUT_MAX;
  }
  c->writing = uart_write(c->uart, buf, (uint32_t)n);
}
static void on_write_done(void* ud) { chip_t* c = ud; c->writing = false; kick(c); }
static void out_str(chip_t* c, const char* s) {
  for (; *s; s++) {
    int next = (c->out_head + 1) % OUT_MAX;
    if (next == c->out_tail) break;  // full: drop
    c->out[c->out_head] = (uint8_t)*s;
    c->out_head = next;
  }
  kick(c);
}
static void reply(chip_t* c, const char* s) { out_str(c, "\r\n"); out_str(c, s); out_str(c, "\r\n"); }

// ---------- helpers ----------
static bool network(chip_t* c) { return attr_read(c->a_network) != 0; }
static bool data(chip_t* c) { return network(c) && attr_read(c->a_data) != 0; }
static bool starts(const char* s, const char* p) { return strncmp(s, p, strlen(p)) == 0; }

static int hexval(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return -1;
}
// UCS2 hex ("0C150C3F") -> UTF-8
static void ucs2hex_to_utf8(const char* hex, char* out, int cap) {
  int o = 0;
  for (int i = 0; hex[i] && hex[i + 1] && hex[i + 2] && hex[i + 3]; i += 4) {
    int a = hexval(hex[i]), b = hexval(hex[i + 1]), d = hexval(hex[i + 2]), e = hexval(hex[i + 3]);
    if (a < 0 || b < 0 || d < 0 || e < 0) break;
    unsigned cp = (unsigned)(a << 12 | b << 8 | d << 4 | e);
    if (cp < 0x80 && o + 1 < cap) out[o++] = (char)cp;
    else if (cp < 0x800 && o + 2 < cap) { out[o++] = (char)(0xC0 | cp >> 6); out[o++] = (char)(0x80 | (cp & 0x3F)); }
    else if (o + 3 < cap) { out[o++] = (char)(0xE0 | cp >> 12); out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[o++] = (char)(0x80 | (cp & 0x3F)); }
  }
  out[o] = 0;
}

static void civil_from_days(long long z, int* y, int* m, int* d) {
  z += 719468;
  long long era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  *d = (int)(doy - (153 * mp + 2) / 5 + 1);
  *m = (int)(mp < 10 ? mp + 3 : mp - 9);
  *y = (int)(yoe + era * 400 + (*m <= 2));
}

// ---------- inbox scripting ----------
static void deliver_next(chip_t* c) {
  // find the delivered-th entry in "num|text;num|text"
  int idx = 0;
  const char* p = c->script;
  while (*p && idx < c->delivered) { if (*p == ';') idx++; p++; }
  if (!*p) return;
  const char* bar = strchr(p, '|');
  const char* end = strchr(p, ';');
  if (!end) end = p + strlen(p);
  if (!bar || bar > end) return;
  for (int i = 0; i < INBOX_MAX; i++) {
    if (c->inbox[i].used) continue;
    int nl = (int)(bar - p), tl = (int)(end - bar - 1);
    if (nl > 23) nl = 23;
    if (tl > 199) tl = 199;
    memcpy(c->inbox[i].from, p, (size_t)nl); c->inbox[i].from[nl] = 0;
    memcpy(c->inbox[i].text, bar + 1, (size_t)tl); c->inbox[i].text[tl] = 0;
    c->inbox[i].used = c->inbox[i].unread = true;
    char u[40];
    snprintf(u, sizeof u, "+CMTI: \"SM\",%d", i);
    reply(c, u);
    printf("modem: SMS received from %s: %s\n", c->inbox[i].from, c->inbox[i].text);
    break;
  }
  c->delivered++;
}

static void on_timer(void* ud) {
  chip_t* c = ud;
  int want = (int)attr_read(c->a_deliver);
  if (want > c->delivered && network(c)) deliver_next(c);
  if (c->mqtt_connected && !data(c)) {
    c->mqtt_connected = false;
    reply(c, "+CMQTTCONNLOST: 0,1");
  }
}

// ---------- command handling ----------
static void handle_line(chip_t* c, char* s) {
  char buf[300];
  if (!*s) return;
  if (strcmp(s, "AT") == 0) { reply(c, "OK"); return; }
  if (strcmp(s, "ATE0") == 0) { c->echo = false; reply(c, "OK"); return; }
  if (strcmp(s, "ATE1") == 0) { c->echo = true; reply(c, "OK"); return; }
  if (strcmp(s, "AT+CPIN?") == 0) { reply(c, "+CPIN: READY"); reply(c, "OK"); return; }
  if (strcmp(s, "AT+CSQ") == 0) {
    snprintf(buf, sizeof buf, "+CSQ: %d,99", network(c) ? (int)attr_read(c->a_signal) : 99);
    reply(c, buf); reply(c, "OK"); return;
  }
  if (starts(s, "AT+CREG?") || starts(s, "AT+CEREG?") || starts(s, "AT+CGREG?")) {
    snprintf(buf, sizeof buf, "+%.5s: 0,%d", s + 3, network(c) ? 1 : 2);
    char* q = strchr(buf, '?'); if (q) *q = ':';
    reply(c, buf); reply(c, "OK"); return;
  }
  if (starts(s, "AT+CSCS=")) { c->ucs2 = strstr(s, "UCS2") != NULL; reply(c, "OK"); return; }
  if (starts(s, "AT+CMGF=") || starts(s, "AT+CSMP=") || starts(s, "AT+CNMI=") || starts(s, "AT+CTZU=") ||
      starts(s, "AT+CGDCONT=") || starts(s, "AT+CSSLCFG=") || starts(s, "AT+CMQTTSSLCFG=") || starts(s, "AT+CPMS=")) {
    reply(c, "OK"); return;
  }
  if (starts(s, "AT+CMGS=")) {
    if (!network(c)) { reply(c, "+CMS ERROR: 331"); return; }  // no network service
    char* q1 = strchr(s, '"'); char* q2 = q1 ? strchr(q1 + 1, '"') : NULL;
    if (!q1 || !q2) { reply(c, "ERROR"); return; }
    *q2 = 0;
    if (c->ucs2) ucs2hex_to_utf8(q1 + 1, c->sms_to, sizeof c->sms_to);
    else snprintf(c->sms_to, sizeof c->sms_to, "%s", q1 + 1);
    c->mode = M_SMS_BODY; c->len = 0;
    out_str(c, "\r\n> ");
    return;
  }
  if (starts(s, "AT+CMGL")) {
    for (int i = 0; i < INBOX_MAX; i++) {
      sms_t* m = &c->inbox[i];
      if (!m->used || !m->unread) continue;
      snprintf(buf, sizeof buf, "+CMGL: %d,\"REC UNREAD\",\"%s\",\"\",\"26/11/21,10:30:00+22\"", i, m->from);
      reply(c, buf);
      out_str(c, m->text); out_str(c, "\r\n");
      m->unread = false;
    }
    reply(c, "OK"); return;
  }
  if (starts(s, "AT+CMGD=")) {
    int idx = atoi(s + 8);
    const char* comma = strchr(s, ',');
    int flag = comma ? atoi(comma + 1) : 0;
    for (int i = 0; i < INBOX_MAX; i++)
      if ((flag >= 1 && c->inbox[i].used && !c->inbox[i].unread) || (flag == 0 && i == idx)) c->inbox[i].used = false;
    reply(c, "OK"); return;
  }
  if (strcmp(s, "AT+CCLK?") == 0) {
    long long e = (long long)(c->base_epoch + (get_sim_nanos_d() - c->base_nanos) / 1e9 * c->speed) + 19800;  // IST
    int y, mo, d;
    civil_from_days(e / 86400, &y, &mo, &d);
    long long sec = e % 86400;
    snprintf(buf, sizeof buf, "+CCLK: \"%02d/%02d/%02d,%02lld:%02lld:%02lld+22\"", y % 100, mo, d, sec / 3600, sec / 60 % 60, sec % 60);
    reply(c, buf); reply(c, "OK"); return;
  }
  if (strcmp(s, "AT+CCERTLIST") == 0) {
    if (c->have_cert) { snprintf(buf, sizeof buf, "+CCERTLIST: \"%s\"", c->cert_name); reply(c, buf); }
    reply(c, "OK"); return;
  }
  if (starts(s, "AT+CCERTDOWN=")) {
    char* q1 = strchr(s, '"'); char* q2 = q1 ? strchr(q1 + 1, '"') : NULL;
    const char* comma = q2 ? strchr(q2, ',') : NULL;
    if (!comma) { reply(c, "ERROR"); return; }
    *q2 = 0;
    snprintf(c->cert_name, sizeof c->cert_name, "%s", q1 + 1);
    c->raw_need = atoi(comma + 1);
    c->raw_target = 2; c->raw_len = 0;
    if (c->raw_need <= 0) { reply(c, "ERROR"); return; }
    c->mode = M_RAW;
    out_str(c, "\r\n>");
    return;
  }
  if (strcmp(s, "AT+CMQTTSTART") == 0) { c->mqtt_started = true; reply(c, "OK"); reply(c, "+CMQTTSTART: 0"); return; }
  if (starts(s, "AT+CMQTTACCQ=")) { reply(c, "OK"); return; }
  if (starts(s, "AT+CMQTTCONNECT=")) {
    reply(c, "OK");
    c->mqtt_connected = c->mqtt_started && data(c);
    reply(c, c->mqtt_connected ? "+CMQTTCONNECT: 0,0" : "+CMQTTCONNECT: 0,26");
    return;
  }
  if (starts(s, "AT+CMQTTTOPIC=") || starts(s, "AT+CMQTTPAYLOAD=")) {
    const char* comma = strchr(s, ',');
    c->raw_need = comma ? atoi(comma + 1) : 0;
    c->raw_target = starts(s, "AT+CMQTTTOPIC=") ? 0 : 1;
    c->raw_len = 0;
    if (c->raw_need <= 0 || c->raw_need >= (c->raw_target ? LINE_MAX : 256)) { reply(c, "ERROR"); return; }
    c->mode = M_RAW;
    out_str(c, "\r\n>");
    return;
  }
  if (starts(s, "AT+CMQTTPUB=")) {
    reply(c, "OK");
    if (c->mqtt_connected && data(c)) {
      c->pub_count++;
      reply(c, "+CMQTTPUB: 0,0");
      printf("MQTT PUBLISH %s %dB #%d %.80s\n", c->topic, (int)strlen(c->payload), c->pub_count, c->payload);
    } else {
      reply(c, "+CMQTTPUB: 0,11");
    }
    return;
  }
  if (starts(s, "AT+CMQTTDISC") || starts(s, "AT+CMQTTREL") || starts(s, "AT+CMQTTSTOP")) {
    c->mqtt_connected = false;
    if (starts(s, "AT+CMQTTSTOP")) c->mqtt_started = false;
    reply(c, "OK"); return;
  }
  printf("modem: unknown command: %s\n", s);
  reply(c, "ERROR");
}

static void finish_sms(chip_t* c) {
  c->line[c->len] = 0;
  char text[600];
  if (c->ucs2) ucs2hex_to_utf8(c->line, text, sizeof text);
  else snprintf(text, sizeof text, "%s", c->line);
  if (!network(c)) { reply(c, "+CMS ERROR: 331"); }
  else {
    c->sms_count++;
    char buf[40];
    snprintf(buf, sizeof buf, "+CMGS: %d", ++c->mr % 256);
    reply(c, buf); reply(c, "OK");
    printf("SMS to %s: %s\n", c->sms_to, text);
  }
  c->mode = M_CMD; c->len = 0;
}

static void on_rx(void* ud, uint8_t b) {
  chip_t* c = ud;
  if (c->mode == M_RAW) {
    if (c->raw_target == 2) {  // certificate bytes are only counted, not kept
      if (--c->raw_need == 0) { c->have_cert = true; c->mode = M_CMD; c->len = 0; reply(c, "OK"); }
      return;
    }
    char* dst = c->raw_target ? c->payload : c->topic;
    dst[c->raw_len++] = (char)b;
    if (--c->raw_need == 0) { dst[c->raw_len] = 0; c->mode = M_CMD; c->len = 0; reply(c, "OK"); }
    return;
  }
  if (c->mode == M_SMS_BODY) {
    if (b == 0x1A) { finish_sms(c); return; }
    if (b == 0x1B) { c->mode = M_CMD; c->len = 0; reply(c, "OK"); return; }  // ESC cancels
    if (c->len < LINE_MAX - 1) c->line[c->len++] = (char)b;
    return;
  }
  if (c->echo) { char s[2] = {(char)b, 0}; out_str(c, s); }
  if (b == '\r' || b == '\n') {
    c->line[c->len] = 0;
    if (c->len) handle_line(c, c->line);
    c->len = 0;
    return;
  }
  if (c->len < LINE_MAX - 1) c->line[c->len++] = (char)b;
}

void chip_init(void) {
  chip_t* c = calloc(1, sizeof(chip_t));
  g = c;
  c->echo = true;  // like the real module after power-up
  c->a_network = attr_init("network", 1);
  c->a_data = attr_init("data", 1);
  c->a_signal = attr_init("signal", 18);
  c->a_deliver = attr_init("deliver", 0);
  string_t inbox = attr_string_init("inbox");
  if (inbox != STRING_NULL) string_read(inbox, c->script, sizeof c->script);
  c->base_epoch = (double)attr_read(attr_init("start_epoch", 1795237200));
  c->speed = (double)attr_read(attr_init("speed", 1));
  c->base_nanos = get_sim_nanos_d();
  const uart_config_t cfg = {
      .user_data = c, .rx = pin_init("RX", INPUT), .tx = pin_init("TX", INPUT_PULLUP),
      .baud_rate = 115200, .rx_data = on_rx, .write_done = on_write_done,
  };
  c->uart = uart_init(&cfg);
  const timer_config_t tcfg = {.user_data = c, .callback = on_timer};
  c->timer = timer_init(&tcfg);
  timer_start(c->timer, 100000, true);  // 100 ms
  printf("a7670c: ready\n");
}
