#pragma once

#include <vector>

class Settings
{
public:
    const uint32_t watchdog_timer;

    class Time
    {
    public:
        const char *const ntpServer;
        const char *const tz;
        Time();
    };
    const Time time;

    class Wifi
    {
    public:
        const char *const ssid;
        const char *const password;
        Wifi();
    };
    const Wifi wifi;

    class Mqtt
    {
    private:
        char *getClientName(const char *prefix) const;

    public:
        const char *const server;
        const uint16_t port;
        const char *const user;
        const char *const password;
        const char *const client_name;
        const char *const topic_prefix;
        const unsigned int reconnect_ms;
        Mqtt();
    };
    const Mqtt mqtt;

    class Led
    {
    public:
        const uint32_t wifi_off;
        const uint32_t wifi_on_mqtt_off;
        const uint32_t mqtt_on;
        const uint32_t ble;
        Led();
    };
    const Led led;

    class Ble
    {
    public:
        struct device_t
        {
            const char *mac;
            const char *key;
            const char *name;
        };
        const std::vector<device_t> devices;
        Ble();
    };
    const Ble ble;

    Settings();
};

extern Settings settings;
