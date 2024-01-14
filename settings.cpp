#pragma once

#if __has_include("settings-private.hpp")
#include "settings-private.hpp"
#endif

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
#ifndef TZ
#define TZ UTC
#endif
#ifndef DEVICES
#define DEVICES \
    {           \
    }
#endif

Settings::Settings()
    : watchdog_timer(60) {}

Settings::Time::Time()
    : ntpServer("pool.ntp.org"),
      tz(TZ) {}

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

Settings::Led::Led()
    : wifi_off(0x800000u),
      wifi_on_mqtt_off(0xC08000u),
      mqtt_on(0x008000u),
      ble(0x0000ffu) {}

Settings::Ble::Ble()
    : devices(DEVICES) {}
