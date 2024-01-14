#pragma once

#include <NimBLEDevice.h>

#include "custom_beacon.h"
#include "settings.hpp"

class AdvertisementProcessor : public NimBLEAdvertisedDeviceCallbacks
{
private:
    static const size_t json_capacity = JSON_OBJECT_SIZE(7); // max size
    static constexpr char *key_sensor = "sensor";
    static constexpr char *key_temp = "temperature";
    static constexpr char *key_humi = "humidity";
    static constexpr char *key_volt = "voltage";
    static constexpr char *key_batt = "battery";
    static constexpr char *key_time = "timestamp";
    static constexpr char *key_rssi = "rssi";

    struct device_t
    {
        uint8_t key[16];
        char *name;
        char *mqtt_topic;
    };

    /* Encrypted atc/custom nonce */
    /* from from pvvx/ATC_MiThermometer/src/custom_beacon.c */
    struct __attribute__((packed)) enc_beacon_nonce_t
    {
        uint8_t MAC[6];
        adv_cust_head_t head;
    };

    const std::map<uint64_t, device_t> devices;
    PubSubClient *const mqtt_client;
    unsigned long last_success;

    std::map<uint64_t, device_t> convert_devices(const std::vector<Settings::Ble::device_t> devices) const;

    template <typename T>
    bool verify_header(const T *p) const;

    template <typename T_in, typename T_out>
    bool decrypt(T_out *target, const T_in *adv, const uint64_t &mac, const uint8_t *key) const;

    bool decode_adv_cust_enc_t(JsonDocument &doc, const padv_cust_enc_t payload, const uint64_t &mac, const uint8_t *key) const;

    bool decode_adv_atc_enc_t(JsonDocument &doc, const padv_atc_enc_t payload, uint64_t mac, const uint8_t *key) const;

    bool decode_adv_custom_t(JsonDocument &doc, const padv_custom_t payload) const;

    bool decode_adv_atc1441_t(JsonDocument &doc, const padv_atc1441_t payload) const;

public:
    AdvertisementProcessor(const std::vector<Settings::Ble::device_t> devices, PubSubClient *mqtt_client);

    void onResult(NimBLEAdvertisedDevice *adv);

    unsigned long lastSuccess() const;
};
