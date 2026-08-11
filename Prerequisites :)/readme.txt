# ESP32 Prerequisites

This guide explains everything required to compile and run the project.

## Required Libraries

The project uses the following headers:

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32BLECombo.h>
```

### Installation Summary

| Library | Install Separately? | Source |
| --- | :---: | --- |
| `WiFi.h` | No | ESP32 Arduino Core |
| `HTTPClient.h` | No | ESP32 Arduino Core |
| `WiFiClientSecure.h` | No | ESP32 Arduino Core |
| `ArduinoJson.h` | **Yes** | Arduino Library Manager |
| `WebServer.h` | No | ESP32 Arduino Core |
| `ESPmDNS.h` | No | ESP32 Arduino Core |
| `ESP32BLECombo.h` | **Yes** | Included in this `prerequisites` folder |

---

## 1. Install the ESP32 Arduino Core

The following libraries are included automatically with the ESP32 Arduino Core:

- `WiFi.h`
- `HTTPClient.h`
- `WiFiClientSecure.h`
- `WebServer.h`
- `ESPmDNS.h`

You **do not** need to install these libraries individually.

### Installation Steps

1. Open **Arduino IDE**.
2. Open **Tools → Board → Boards Manager**.
3. Search for `esp32`.
4. Install **esp32 by Espressif Systems**.
5. Select your ESP32 board from **Tools → Board → esp32**.

---

## 2. Install ArduinoJson

The project uses:

```cpp
#include <ArduinoJson.h>
```

ArduinoJson is a separate library and must be installed.

### Installation Steps

1. Open **Arduino IDE**.
2. Open **Sketch → Include Library → Manage Libraries...**.
3. Search for `ArduinoJson`.
4. Install **ArduinoJson**.

After installation, the following should compile correctly:

```cpp
#include <ArduinoJson.h>
```

---

## 3. Install ESP32BLECombo

The project uses:

```cpp
#include <ESP32BLECombo.h>
```

> [!IMPORTANT]
> The **ESP32BLECombo** library is already included in this project's `prerequisites/` folder.
>
> **Do not download another copy from the internet.** Use the version provided with this project.

### If ESP32BLECombo Is a ZIP File

1. Open **Arduino IDE**.
2. Open **Sketch → Include Library → Add .ZIP Library...**.
3. Select the ESP32BLECombo ZIP file from the `prerequisites/` folder.
4. Restart Arduino IDE if required.

### If ESP32BLECombo Is a Folder

Copy the `ESP32BLECombo` folder into your Arduino libraries directory.

On Linux, the usual location is:

```text
~/Arduino/libraries/
```

The final structure should look similar to:

```text
~/Arduino/libraries/ESP32BLECombo/
```

Restart Arduino IDE after copying the library.

---

## Final Installation Checklist

Before compiling the project, make sure you have:

- [ ] **ESP32 by Espressif Systems** installed through Boards Manager
- [ ] **ArduinoJson** installed through Library Manager
- [ ] **ESP32BLECombo** installed from the project's `prerequisites/` folder

You **do not** need to separately install:

- `WiFi.h`
- `HTTPClient.h`
- `WiFiClientSecure.h`
- `WebServer.h`
- `ESPmDNS.h`

These are provided by the **ESP32 Arduino Core**.

---

## Quick Reference

| Header | Where It Comes From |
| --- | --- |
| `WiFi.h` | ESP32 Arduino Core |
| `HTTPClient.h` | ESP32 Arduino Core |
| `WiFiClientSecure.h` | ESP32 Arduino Core |
| `ArduinoJson.h` | Arduino Library Manager |
| `WebServer.h` | ESP32 Arduino Core |
| `ESPmDNS.h` | ESP32 Arduino Core |
| `ESP32BLECombo.h` | Project `prerequisites/` folder |

---

## Minimum Setup

Install these three things:

1. **ESP32 by Espressif Systems**
2. **ArduinoJson**
3. **ESP32BLECombo** from the project's `prerequisites/` folder

Once these are installed, all required headers for the project should be available.
