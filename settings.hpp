#pragma once

#include <map>
#include <vector>

class Settings
{
public:
    const uint32_t watchdog_timer;

    class Time
    {
    public:
        const char *const ntpServer;
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
        char *getStatusTopicName(const char *prefix) const;
        char *getClientName(const char *prefix) const;

    public:
        const char *const server;
        const uint16_t port;
        const char *const user;
        const char *const password;
        const char *const client_name;
        const char *const topic_prefix;
        const unsigned int status_interval_ms;
        const char *const status_topic_name;
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
        struct device_def_t
        {
            const char *mac;
            const char *key;
            const char *name;
        };

        struct device_t
        {
            const uint8_t *key;     // 16B or nullptr if not set
            const char *name;       // last 3 bytes of MAC if not defined in settings
            const char *mqtt_topic; // topic prefix + "/" + name
        };

    private:
        std::map<uint64_t, device_t> convert_devices(const std::vector<device_def_t> devices) const;

    public:
        const std::map<uint64_t, device_t> devices;
        Ble();
    };
    const Ble ble;

    Settings();
};

extern Settings settings;
