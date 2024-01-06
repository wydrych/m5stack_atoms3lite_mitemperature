#include <esp_task_wdt.h>
#define FASTLED_INTERNAL // disable annoying pragma message
#include <FastLED.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <time.h>

#include "processor.hpp"
#include "settings.hpp"

CRGB led;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
AdvertisementProcessor advertisementProcessor(settings::ble::devices);

bool wifi_status;
bool mqtt_status;

template <typename T>
inline bool changed(T *storage, T val)
{
    if (*storage == val)
        return false;
    *storage = val;
    return true;
}

void setup()
{
    esp_task_wdt_init(settings::watchdog_timer, true);
    esp_task_wdt_add(NULL);

    M5.Log.setEnableColor(m5::log_target_serial, false);
    M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_DEBUG);

    auto cfg = M5.config();
    M5.begin(cfg);

    FastLED.addLeds<NEOPIXEL, 35>(&led, 1);

    WiFi.begin(settings::wifi::ssid, settings::wifi::password);

    mqtt_client.setServer(settings::mqtt::server, settings::mqtt::port);

    configTzTime(settings::time::tz, settings::time::ntpServer);

    NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
    NimBLEDevice::init("");
    NimBLEScan *pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(&advertisementProcessor, false);
    pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.
}

void on_wifi_connected()
{
    M5_LOGI("WiFi connected");
}

void on_wifi_disconnected()
{
    M5_LOGI("WiFi disconnected");
}

void wifi_loop()
{
    if (!changed(&wifi_status, WiFi.isConnected()))
        return;

    if (wifi_status)
        on_wifi_connected();
    else
        on_wifi_disconnected();
}

void on_mqtt_connected()
{
    M5_LOGI("MQTT connected");
}

void on_mqtt_disconnected()
{
    M5_LOGI("MQTT disconnected");
}

void mqtt_loop()
{
    if (wifi_status && !mqtt_status)
    {
        static unsigned long last_reconnect;
        unsigned long now = millis();
        if (last_reconnect == 0 || now - last_reconnect >= settings::mqtt::reconnect * 1000)
        {
            last_reconnect = now;
            M5_LOGI("Trying to connect MQTT");
            mqtt_client.connect((String("M5Stack AtomS3 Lite ") + WiFi.macAddress()).c_str());
        }
    }
    if (!wifi_status && mqtt_status)
        mqtt_client.disconnect();

    if (changed(&mqtt_status, mqtt_client.connected()))
    {
        if (mqtt_status)
            on_mqtt_connected();
        else
            on_mqtt_disconnected();
    }
    mqtt_client.loop();
}

void led_loop()
{
    CRGB new_color = wifi_status
                         ? mqtt_status
                               ? settings::led::mqtt_on
                               : settings::led::wifi_on_mqtt_off
                         : settings::led::wifi_off;
    if (!changed(&led, new_color))
        return;
    led = new_color;
    FastLED.show();
}

void ble_scan_loop()
{
    if (!NimBLEDevice::getScan()->isScanning())
    {
        M5_LOGI("Starting Bluetooth scan");
        if (!NimBLEDevice::getScan()->start(0, nullptr, false))
            M5_LOGW("Bluetooth scan failed to start");
    }
}

void loop()
{
    esp_task_wdt_reset();
    wifi_loop();
    mqtt_loop();
    ble_scan_loop();
    led_loop();
}