#pragma once
// CA certificate the MQTT broker's TLS certificate must chain to. PLACEHOLDER until the broker
// provider is chosen (question Q-5); replace with the provider's root CA before field trials.
static const char kMqttCaPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "PLACEHOLDER-UNTIL-BROKER-CHOSEN\n"
    "-----END CERTIFICATE-----\n";
