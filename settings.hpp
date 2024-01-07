#pragma once

#if __has_include("settings-private.hpp")
#include "settings-private.hpp"
#endif

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
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "m5stack_atoms3lite_mitemperature"
#endif
#ifndef TZ
#define TZ UTC
#endif
#ifndef DEVICES
#define DEVICES \
    {           \
    }
#endif

namespace settings
{
    const uint32_t watchdog_timer = 60;
    namespace time
    {
        const char *const ntpServer = "pool.ntp.org";
        const char *const tz = TZ;
    }
    namespace wifi
    {
        const char *const ssid = WIFI_SSID;
        const char *const password = WIFI_PASSWORD;
    }
    namespace mqtt
    {
        const char *const server = MQTT_SERVER;
        const uint16_t port = MQTT_PORT;
        const char *const topic_prefix = MQTT_TOPIC_PREFIX;
        const float reconnect = 3.0;
    }
    namespace led
    {
        const uint32_t wifi_off = 0x800000u;
        const uint32_t wifi_on_mqtt_off = 0xC08000u;
        const uint32_t mqtt_on = 0x008000u;
    }
    namespace ble
    {
        struct device_t
        {
            const char *mac;
            const char *key;
            const char *name;
        };
        const std::vector<device_t> devices = DEVICES;
    }
}
