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

Settings settings;

CRGB led;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
AdvertisementProcessor advertisementProcessor(mqtt_client);

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
    esp_task_wdt_init(settings.watchdog_timer_s, true);
    esp_task_wdt_add(NULL);

    M5.Log.setEnableColor(m5::log_target_serial, false);
    M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_DEBUG);

    auto cfg = M5.config();
    M5.begin(cfg);

    FastLED.addLeds<NEOPIXEL, 35>(&led, 1);

    WiFi.begin(settings.wifi.ssid, settings.wifi.password);

    mqtt_client.setServer(settings.mqtt.server, settings.mqtt.port);

    configTzTime("GMT0", settings.time.ntpServer);

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
        if (last_reconnect == 0 || now - last_reconnect >= settings.mqtt.reconnect * 1000)
        {
            last_reconnect = now;
            M5_LOGI("Trying to connect MQTT");
            mqtt_client.connect(settings.mqtt.client_name,
                                settings.mqtt.user, settings.mqtt.password);
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
                               ? settings.led.mqtt_on
                               : settings.led.wifi_on_mqtt_off
                         : settings.led.wifi_off;
    unsigned long since = millis() - advertisementProcessor.lastSuccess();
    if (since <= 255)
        new_color = new_color.lerp8(settings.led.ble, 255 - since);
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

void status_loop()
{
    static int64_t next = 0;
    if (esp_timer_get_time() < next)
        return;
    next += settings.mqtt.status_interval * 1000000;

    multi_heap_info_t info;
    StaticJsonDocument<JSON_OBJECT_SIZE(8)> doc;

    doc["uptime"] = esp_timer_get_time() / 1000000;

    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    doc["heap_8bit_total_free_bytes"] = info.total_free_bytes;
    doc["heap_8bit_total_allocated_bytes"] = info.total_allocated_bytes;
    doc["heap_8bit_largest_free_block"] = info.largest_free_block;
    doc["heap_8bit_minimum_free_bytes"] = info.minimum_free_bytes;
    doc["heap_8bit_allocated_blocks"] = info.allocated_blocks;
    doc["heap_8bit_free_blocks"] = info.free_blocks;
    doc["heap_8bit_total_blocks"] = info.total_blocks;

    char json[1024];
    size_t json_length = serializeJson(doc, json);

    if (!mqtt_client.beginPublish(settings.mqtt.status_topic_name, json_length, false))
        return;
    mqtt_client.write((uint8_t *)json, json_length);
    mqtt_client.endPublish();
}

void loop()
{
    esp_task_wdt_reset();
    wifi_loop();
    mqtt_loop();
    ble_scan_loop();
    led_loop();
    status_loop();
}