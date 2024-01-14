#pragma once

#include <M5Unified.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <mbedtls/ccm.h>

#include "processor.hpp"

template <typename T>
bool AdvertisementProcessor::verify_header(const T *p) const
{
    return p->uid == BLE_HS_ADV_TYPE_SVC_DATA_UUID16 &&
           p->UUID == ADV_CUSTOM_UUID16;
}

template <typename T_in, typename T_out>
bool AdvertisementProcessor::decrypt(T_out *target, const T_in *adv, const uint64_t &mac, const uint8_t *key) const
{
    enc_beacon_nonce_t cbn;

    // reverse MAC address
    cbn.MAC[0] = (uint8_t)(mac >> 0);
    cbn.MAC[1] = (uint8_t)(mac >> 8);
    cbn.MAC[2] = (uint8_t)(mac >> 16);
    cbn.MAC[3] = (uint8_t)(mac >> 24);
    cbn.MAC[4] = (uint8_t)(mac >> 32);
    cbn.MAC[5] = (uint8_t)(mac >> 40);

    memcpy(&cbn.head, &adv->head, sizeof(cbn.head));

    uint8_t aad = 0x11;

    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);

    int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 16 * 8);
    if (ret)
    {
        M5_LOGE("mbedtls_ccm_setkey() failed: ret = %d", ret);
        mbedtls_ccm_free(&ctx);
        return false;
    }

    ret = mbedtls_ccm_auth_decrypt(&ctx, sizeof(adv->data),
                                   (uint8_t *)&cbn, sizeof(cbn),
                                   &aad, sizeof(aad),
                                   (uint8_t *)&adv->data, (uint8_t *)target,
                                   adv->mic, 4);
    if (ret)
    {
        M5_LOGE("mbedtls_ccm_auth_decrypt() failed: ret = %d", ret);
        mbedtls_ccm_free(&ctx);
        return false;
    }

    mbedtls_ccm_free(&ctx);
    return true;
}

bool AdvertisementProcessor::decode_adv_cust_enc_t(JsonDocument &doc, const padv_cust_enc_t payload, const uint64_t &mac, const uint8_t *key) const
{
    if (!key)
    {
        M5_LOGE("no key configured to decrypt the payload for MAC %012llX", mac);
        return false;
    }

    if (!verify_header(&payload->head))
        return false;

    M5_LOGV("decoding custom pvvx encrypted format");

    adv_cust_data_t d;
    if (!decrypt(&d, payload, mac, key))
    {
        M5_LOGE("could not decrypt the payload for MAC %012llX", mac);
        return false;
    }
    doc[key_temp] = ((JsonFloat)d.temp) / 100;
    doc[key_humi] = ((JsonFloat)d.humi) / 100;
    doc[key_batt] = d.bat;

    return true;
}

bool AdvertisementProcessor::decode_adv_atc_enc_t(JsonDocument &doc, const padv_atc_enc_t payload, uint64_t mac, const uint8_t *key) const
{
    if (!key)
    {
        M5_LOGE("no key configured to decrypt the payload for MAC %012llX", mac);
        return false;
    }

    if (!verify_header(&payload->head))
        return false;

    M5_LOGV("decoding atc1441 encrypted format");

    adv_atc_data_t d;
    if (!decrypt(&d, payload, mac, key))
    {
        M5_LOGE("could not decrypt the payload for MAC %012llX", mac);
        return false;
    }
    doc[key_temp] = ((JsonFloat)d.temp) / 2 - 40;
    doc[key_humi] = ((JsonFloat)d.humi) / 2;
    doc[key_batt] = d.bat & 0x7f;

    return true;
}

bool AdvertisementProcessor::decode_adv_custom_t(JsonDocument &doc, const padv_custom_t payload) const
{
    if (!verify_header(payload))
        return false;

    M5_LOGV("decoding custom pvvx format");

    doc[key_temp] = ((JsonFloat)payload->temperature) / 100;
    doc[key_humi] = ((JsonFloat)payload->humidity) / 100;
    doc[key_volt] = payload->battery_mv;
    doc[key_batt] = payload->battery_level;

    return true;
}

bool AdvertisementProcessor::decode_adv_atc1441_t(JsonDocument &doc, const padv_atc1441_t payload) const
{
    if (!verify_header(payload))
        return false;

    M5_LOGV("decoding atc1441 format");

    int16_t temp;
    uint16_t volt;
    std::reverse_copy(payload->temperature, payload->temperature + 2, (uint8_t *)&temp);
    std::reverse_copy(payload->battery_mv, payload->battery_mv + 2, (uint8_t *)&volt);
    doc[key_temp] = ((JsonFloat)temp) / 10;
    doc[key_humi] = payload->humidity;
    doc[key_volt] = volt;
    doc[key_batt] = payload->battery_level;

    return true;
}

AdvertisementProcessor::AdvertisementProcessor(PubSubClient &mqtt_client)
    : mqtt_client(&mqtt_client), last_success(0) {}

void AdvertisementProcessor::onResult(NimBLEAdvertisedDevice *adv)
{
    uint64_t mac = adv->getAddress();
    auto entry = settings.ble.devices.find(mac);
    if (entry == settings.ble.devices.end())
        return;

    size_t payloadLength = adv->getPayloadLength();
    const uint8_t *payload = adv->getPayload();

    // expecting exactly one advertisement data element
    if (payloadLength == 0 || payload[0] != payloadLength - 1)
        return;

    bool success = false;
    StaticJsonDocument<json_capacity> doc;

    switch (payloadLength)
    {
    case sizeof(adv_cust_enc_t):
        success = decode_adv_cust_enc_t(doc, (padv_cust_enc_t)payload, mac, entry->second.key);
        break;
    case sizeof(adv_atc_enc_t):
        success = decode_adv_atc_enc_t(doc, (padv_atc_enc_t)payload, mac, entry->second.key);
        break;
    case sizeof(adv_custom_t):
        success = decode_adv_custom_t(doc, (padv_custom_t)payload);
        break;
    case sizeof(adv_atc1441_t):
        success = decode_adv_atc1441_t(doc, (padv_atc1441_t)payload);
        break;
    }

    if (!success)
        return;

    time_t now;
    time(&now);
    doc[key_sensor] = entry->second.name;
    doc[key_time] = now;
    doc[key_rssi] = adv->getRSSI();

    size_t jsonLength = measureJson(doc);
    char json[jsonLength + 1];
    serializeJson(doc, json, jsonLength + 1);

    if (mqtt_client->publish(entry->second.mqtt_topic, json))
    {
        M5_LOGI("successfully published %s to MQTT topic %s", json, entry->second.mqtt_topic);
        last_success = millis();
    }
    else
        M5_LOGE("could not publish %s to MQTT topic %s", json, entry->second.mqtt_topic);
}

unsigned long AdvertisementProcessor::lastSuccess() const
{
    return last_success;
}
