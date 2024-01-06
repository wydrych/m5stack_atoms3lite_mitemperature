#pragma once

#include <M5Unified.h>
#include <NimBLEDevice.h>

#include "custom_beacon.h"

class AdvertisementProcessor : public NimBLEAdvertisedDeviceCallbacks
{
private:
    struct key_t
    {
        uint8_t v[16];
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
            if (sscanf(d.second,
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

    static bool verify_adv_cust_head_t(const padv_cust_head_t head)
    {
        return head->uid == BLE_HS_ADV_TYPE_SVC_DATA_UUID16 &&
               head->UUID == ADV_CUSTOM_UUID16;
    }

    static bool decode_adv_cust_enc_t(const padv_cust_enc_t payload)
    {
        if (!verify_adv_cust_head_t(&payload->head))
            return false;

        M5_LOGD("decoding custom pvvx encrypted format");
        return true;
    }

    static bool decode_adv_atc_enc_t(const padv_atc_enc_t payload)
    {
        if (!verify_adv_cust_head_t(&payload->head))
            return false;

        M5_LOGD("decoding atc1441 encrypted format");
        return true;
    }

    static bool decode_adv_custom_t(const padv_custom_t payload)
    {
        M5_LOGD("decoding custom pvvx format");
        return true;
    }

    static bool decode_adv_atc1441_t(const padv_atc1441_t payload)
    {
        M5_LOGD("decoding atc1441 format");
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
        switch (payloadLength)
        {
        case sizeof(adv_cust_enc_t):
            success = decode_adv_cust_enc_t((padv_cust_enc_t)payload);
            break;
        case sizeof(adv_atc_enc_t):
            success = decode_adv_atc_enc_t((padv_atc_enc_t)payload);
            break;
        case sizeof(adv_custom_t):
            success = decode_adv_custom_t((padv_custom_t)payload);
            break;
        case sizeof(adv_atc1441_t):
            success = decode_adv_atc1441_t((padv_atc1441_t)payload);
            break;
        }
    }
};
