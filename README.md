# LYWSD03MMC to MQTT Gateway

Gateway to convert between LYWSD03MMC (Xiaomi Mi Temperature and Humidity
Monitor 2) running custom [atc1441] or [pvvx] software Bluetooth Low Energy
measurements announcements to the MQTT messages in JSON.

The software has been written for the [M5Stack Atom S3 Lite][AtomS3Lite] board,
but should work on other similar boards with little or no adaptation needed.

Supports messages in the [atc1441] and [pvvx] formats, both non-encrypted and
encrypted (feature not available in [ESPHome's `xiaomi_ble`][ESPHomeXiaomiBle]
sensor platform).

## Configuration

Create `settings-private.hpp` file that defines:
* `WIFI_SSID` and `WIFI_PASSWORD`
* `MQTT_SERVER` plus optionally any of: `MQTT_PORT` (default: *1883*), `MQTT_USER`
   and `MQTT_PASSWORD` (both default to *null*), `MQTT_CLIENT_NAME_PREFIX`
   (default: *M5Stack AtomS3 Lite*), `MQTT_TOPIC_PREFIX` (default:
   *m5stack_atoms3lite_mitemperature*)
* `DEVICES` in the format explained below

Example `settings-private.hpp` configuration file:
```C++
#pragma once

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

#define MQTT_SERVER "192.168.0.1"
#define MQTT_TOPIC_PREFIX "m5stack_atoms3lite_mitemperature"

#define DEVICES { \
            {"A4:C1:38:97:8C:7B"}, \
            {"A4:C1:38:FF:C1:3F", "d553e7cea293deca503a2e036b630503"}, \
            {"A4:C1:38:7A:C0:DB", "68c3b1a07e224df1aa1ddd5e5421ac96", "bathroom"}}
```

### Device configuration

Devices are configured as vector of tuples with one, two, or three elements.
* The first (and mandatory) field is the MAC address of the device.
* The second field is the encryption key in hex. It is used to decode
  encrypted messages. Ignored for non-encrypted announcements. Can be `nullptr`
  if not needed but the third field is used.
* The third field is the custom name to be used in MQTT topic and in the `sensor`
  field. If not set, it is the last three octets of the MAC address without
  colons, i.e., `978C7B` and `FFC13F` in the above example.

## MQTT messages

MQTT messages are sent to the per-sensor topics and include all decoded fields
(vary by the format). Additionally, the gateway reports its uptime and heap
stats to the `status` topic:
```
m5stack_atoms3lite_mitemperature/978C7B {"temperature":27.12,"humidity":43.83,"voltage":2976,"battery":100,"sensor":"978C7B","timestamp":1708205031,"rssi":-38}
m5stack_atoms3lite_mitemperature/FFC13F {"temperature":22.94,"humidity":66.74,"battery":64,"sensor":"FFC13F","timestamp":1708205032,"rssi":-71}
m5stack_atoms3lite_mitemperature/bathroom {"temperature":21.48,"humidity":71.16,"battery":65,"sensor":"bathroom","timestamp":1708205041,"rssi":-56}
m5stack_atoms3lite_mitemperature/status {"uptime":180,"heap_8bit_total_free_bytes":202756,"heap_8bit_total_allocated_bytes":128368,"heap_8bit_largest_free_block":192500,"heap_8bit_minimum_free_bytes":196340,"heap_8bit_allocated_blocks":366,"heap_8bit_free_blocks":5,"heap_8bit_total_blocks":371}
```

[ESPHomeXiaomiBle]: https://esphome.io/components/sensor/xiaomi_ble.html#lywsd03mmc
[AtomS3Lite]: https://docs.m5stack.com/en/core/AtomS3%20Lite
[atc1441]: https://github.com/atc1441/ATC_MiThermometer
[pvvx]: https://github.com/pvvx/ATC_MiThermometer
