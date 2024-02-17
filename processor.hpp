#pragma once

#include <NimBLEDevice.h>

#include "custom_beacon.h"
#include "settings.hpp"

class AdvertisementProcessor : public NimBLEAdvertisedDeviceCallbacks
{
private:
    static const size_t json_capacity = JSON_OBJECT_SIZE(7); // max size
    const char *key_sensor = "sensor";
    const char *key_temp = "temperature";
    const char *key_humi = "humidity";
    const char *key_volt = "voltage";
    const char *key_batt = "battery";
    const char *key_time = "timestamp";
    const char *key_rssi = "rssi";

    /* Encrypted atc/custom nonce */
    /* from from pvvx/ATC_MiThermometer/src/custom_beacon.c */
    struct __attribute__((packed)) enc_beacon_nonce_t
    {
        uint8_t MAC[6];
        adv_cust_head_t head;
    };

    PubSubClient *const mqtt_client;
    unsigned long last_success;

    template <typename T>
    bool verify_header(const T *p) const;

    template <typename T_in, typename T_out>
    bool decrypt(T_out *target, const T_in *adv, const uint64_t &mac, const uint8_t *key) const;

    bool decode_adv_cust_enc_t(JsonDocument &doc, const padv_cust_enc_t payload, const uint64_t &mac, const uint8_t *key) const;

    bool decode_adv_atc_enc_t(JsonDocument &doc, const padv_atc_enc_t payload, uint64_t mac, const uint8_t *key) const;

    bool decode_adv_custom_t(JsonDocument &doc, const padv_custom_t payload) const;

    bool decode_adv_atc1441_t(JsonDocument &doc, const padv_atc1441_t payload) const;

public:
    AdvertisementProcessor(PubSubClient &mqtt_client);

    void onResult(NimBLEAdvertisedDevice *adv);

    unsigned long lastSuccess() const;
};
