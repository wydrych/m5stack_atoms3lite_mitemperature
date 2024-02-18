#if __has_include("settings-private.hpp")
#include "settings-private.hpp"
#endif

#include <cstring>
#include <M5Unified.h>
#include <NimBLEAddress.h>
#include <esp_mac.h>

#include "settings.hpp"

#ifndef WIFI_SSID
#define WIFI_SSID NULL
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD NULL
#endif
#ifndef MQTT_SERVER
#define MQTT_SERVER NULL
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_USER
#define MQTT_USER NULL
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD NULL
#endif
#ifndef MQTT_CLIENT_NAME_PREFIX
#define MQTT_CLIENT_NAME_PREFIX "M5Stack AtomS3 Lite"
#endif
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "m5stack_atoms3lite_mitemperature"
#endif
#ifndef DEVICES
#define DEVICES \
    {           \
    }
#endif

Settings::Settings()
    : watchdog_timer(60) {}

Settings::Time::Time()
    : ntpServer("pool.ntp.org") {}

Settings::Wifi::Wifi()
    : ssid(WIFI_SSID),
      password(WIFI_PASSWORD) {}

Settings::Mqtt::Mqtt()
    : server(MQTT_SERVER),
      port(MQTT_PORT),
      user(MQTT_USER),
      password(MQTT_PASSWORD),
      client_name(getClientName(MQTT_CLIENT_NAME_PREFIX)),
      topic_prefix(MQTT_TOPIC_PREFIX),
      status_interval_ms(60000),
      status_topic_name(getStatusTopicName(MQTT_TOPIC_PREFIX)),
      reconnect_ms(3000) {}

char *Settings::Mqtt::getClientName(const char *prefix) const
{
    uint8_t mac[6];
    char *name;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    asprintf(&name, "%s %02X:%02X:%02X:%02X:%02X:%02X",
             prefix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return name;
}

char *Settings::Mqtt::getStatusTopicName(const char *prefix) const
{
    char *name;
    asprintf(&name, "%s/status", prefix);
    return name;
}

Settings::Led::Led()
    : wifi_off(0x800000u),
      wifi_on_mqtt_off(0xC08000u),
      mqtt_on(0x008000u),
      ble(0x0000ffu) {}

Settings::Ble::Ble()
    : devices(convert_devices(DEVICES)) {}

std::map<uint64_t, Settings::Ble::device_t> Settings::Ble::convert_devices(const std::vector<Settings::Ble::device_def_t> devices) const
{
    std::map<uint64_t, device_t> m;
    for (auto d : devices)
    {
        uint64_t mac = NimBLEAddress(d.mac);
        if (mac == 0)
        {
            M5_LOGE("invalid MAC address: %s", d.mac);
            continue;
        }
        uint8_t *device_key = nullptr;
        char *device_name,
            *device_mqtt_topic;
        if (d.key != nullptr)
        {
            uint8_t key[16];
            if (d.key != nullptr &&
                std::strlen(d.key) == 32 &&
                sscanf(d.key,
                       "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                       &key[0], &key[1], &key[2], &key[3], &key[4], &key[5], &key[6], &key[7],
                       &key[8], &key[9], &key[10], &key[11], &key[12], &key[13], &key[14], &key[15]) == 16)
                memcpy(device_key = (uint8_t *)malloc(16), key, 16);
            else
                M5_LOGI("invalid decryption key: %s", d.key);
        }
        if (d.name != nullptr)
            asprintf(&device_name, "%s", d.name);
        else
            asprintf(&device_name, "%06X", mac & 0xffffffu);
        asprintf(&device_mqtt_topic, "%s/%s", settings.mqtt.topic_prefix, device_name);

        device_t device{device_key, device_name, device_mqtt_topic};
        m[mac] = device;
    }
    return m;
}
