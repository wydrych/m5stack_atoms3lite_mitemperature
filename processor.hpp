#pragma once

#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <mbedtls/ccm.h>

#include "custom_beacon.h"

#define KEY_SENSOR "sensor"
#define KEY_TEMP "temperature"
#define KEY_HUMI "humidity"
#define KEY_VOLT "voltage"
#define KEY_BATT "battery"
#define KEY_TIME "timestamp"
#define KEY_RSSI "rssi"

class AdvertisementProcessor : public NimBLEAdvertisedDeviceCallbacks
{
private:
    struct key_t
    {
        uint8_t v[16];
    };

    /* Encrypted atc/custom nonce */
    /* from from pvvx/ATC_MiThermometer/src/custom_beacon.c */
    struct __attribute__((packed)) enc_beacon_nonce_t
    {
        uint8_t MAC[6];
        adv_cust_head_t head;
    };

    const std::map<uint64_t, key_t> devices;

    static std::map<uint64_t, key_t> convert_devices(std::map<const char *, const char *> devices)
    {
        std::map<uint64_t, key_t> m;
        for (auto d : devices)
        {
            uint64_t mac = NimBLEAddress(d.first);
            if (mac == 0)
            {
                M5_LOGE("invalid MAC address: %s", d.first);
                continue;
            }
            key_t k;
            if (strlen(d.second) != 32 ||
                sscanf(d.second,
                       "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                       &k.v[0], &k.v[1], &k.v[2], &k.v[3], &k.v[4], &k.v[5], &k.v[6], &k.v[7],
                       &k.v[8], &k.v[9], &k.v[10], &k.v[11], &k.v[12], &k.v[13], &k.v[14], &k.v[15]) != 16)

            {
                M5_LOGI("invalid decryption key, setting to zeros: %s", d.second);
                k = key_t();
            }
            m[mac] = k;
        }
        return m;
    }

    template <typename T>
    static bool verify_header(const T *p)
    {
        return p->uid == BLE_HS_ADV_TYPE_SVC_DATA_UUID16 &&
               p->UUID == ADV_CUSTOM_UUID16;
    }

    // static void dump(const char *n, const uint8_t *d, size_t l)
    // {
    //     char hex[l * 2 + 1];
    //     for (size_t i = 0; i < l; i++)
    //     {
    //         sprintf(hex + 2 * i, "%02x", d[i]);
    //     }
    //     hex[l * 2] = 0;
    //     M5_LOGD("%s=(%d)%s", n, l, hex);
    // }

    template <typename T_in, typename T_out>
    static bool decrypt(T_out *target, const T_in *adv, /*const padv_cust_head_t head, */ const uint64_t &mac, const uint8_t *key /*, const uint8_t *mic*/)
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

        // dump("key", key, 16);
        int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 16 * 8);
        if (ret)
        {
            M5_LOGE("mbedtls_ccm_setkey() failed: ret = %d", ret);
            mbedtls_ccm_free(&ctx);
            return false;
        }

        // dump("cbn", (uint8_t *)&cbn, sizeof(cbn));
        // dump("aad", &aad, sizeof(aad));
        // dump("data", (uint8_t *)&adv->data, sizeof(*target));
        // dump("mic", adv->mic, 4);

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

    static bool decode_adv_cust_enc_t(JsonDocument &doc, const padv_cust_enc_t payload, const uint64_t &mac, const uint8_t *key)
    {
        if (!verify_header(&payload->head))
            return false;

        M5_LOGV("decoding custom pvvx encrypted format");

        adv_cust_data_t d;
        if (!decrypt(&d, payload, mac, key))
        {
            M5_LOGE("could not decrypt the payload");
            return false;
        }
        doc[KEY_TEMP] = ((JsonFloat)d.temp) / 100;
        doc[KEY_HUMI] = ((JsonFloat)d.humi) / 100;
        doc[KEY_BATT] = d.bat;

        return true;
    }

    static bool decode_adv_atc_enc_t(JsonDocument &doc, const padv_atc_enc_t payload, uint64_t mac, const uint8_t *key)
    {
        if (!verify_header(&payload->head))
            return false;

        M5_LOGV("decoding atc1441 encrypted format");

        adv_atc_data_t d;
        if (!decrypt(&d, payload, mac, key))
        {
            M5_LOGE("could not decrypt the payload");
            return false;
        }
        doc[KEY_TEMP] = ((JsonFloat)d.temp) / 2 - 40;
        doc[KEY_HUMI] = ((JsonFloat)d.humi) / 2;
        doc[KEY_BATT] = d.bat & 0x7f;

        return true;
    }

    static bool decode_adv_custom_t(JsonDocument &doc, const padv_custom_t payload)
    {
        if (!verify_header(payload))
            return false;

        M5_LOGV("decoding custom pvvx format");

        doc[KEY_TEMP] = ((JsonFloat)payload->temperature) / 100;
        doc[KEY_HUMI] = ((JsonFloat)payload->humidity) / 100;
        doc[KEY_VOLT] = payload->battery_mv;
        doc[KEY_BATT] = payload->battery_level;

        return true;
    }

    static bool decode_adv_atc1441_t(JsonDocument &doc, const padv_atc1441_t payload)
    {
        if (!verify_header(payload))
            return false;

        M5_LOGV("decoding atc1441 format");

        int16_t temp;
        uint16_t volt;
        std::reverse_copy(payload->temperature, payload->temperature + 2, (uint8_t *)&temp);
        std::reverse_copy(payload->battery_mv, payload->battery_mv + 2, (uint8_t *)&volt);
        doc[KEY_TEMP] = ((JsonFloat)temp) / 10;
        doc[KEY_HUMI] = payload->humidity;
        doc[KEY_VOLT] = volt;
        doc[KEY_BATT] = payload->battery_level;

        return true;
    }

public:
    AdvertisementProcessor(std::map<const char *, const char *> devices) : devices(convert_devices(devices)) {}

    void onResult(NimBLEAdvertisedDevice *adv)
    {
        uint64_t mac = adv->getAddress();
        auto entry = devices.find(mac);
        if (entry == devices.end())
            return;

        size_t payloadLength = adv->getPayloadLength();
        const uint8_t *payload = adv->getPayload();
        char hex[payloadLength * 2 + 1];
        for (size_t i = 0; i < payloadLength; i++)
        {
            sprintf(hex + 2 * i, "%02x", payload[i]);
        }
        hex[payloadLength * 2] = 0;
        M5_LOGI("%s (%d)%s", adv->getAddress().toString().c_str(), payloadLength, hex);

        // expecting exactly one advertisement data element
        if (payloadLength == 0 || payload[0] != payloadLength - 1)
            return;

        bool success = false;
        StaticJsonDocument<JSON_OBJECT_SIZE(7)> doc;

        switch (payloadLength)
        {
        case sizeof(adv_cust_enc_t):
            success = decode_adv_cust_enc_t(doc, (padv_cust_enc_t)payload, mac, entry->second.v);
            break;
        case sizeof(adv_atc_enc_t):
            success = decode_adv_atc_enc_t(doc, (padv_atc_enc_t)payload, mac, entry->second.v);
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
        char sensorname[7];
        sprintf(sensorname, "%06X", mac & 0xffffffu);
        doc[KEY_SENSOR] = sensorname;
        doc[KEY_TIME] = now;
        doc[KEY_RSSI] = adv->getRSSI();

        size_t jsonLength = measureJson(doc);
        char json[jsonLength + 1];
        serializeJson(doc, json, jsonLength + 1);
        M5_LOGI("%s", json);
    }
};
