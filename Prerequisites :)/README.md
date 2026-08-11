# Library Installation Guide

This project uses several libraries, but **most of them are already included with the ESP32 Arduino Core**.

## Required Libraries

The project includes:

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32BLECombo.h>
```

### Library Status

| Library | Install Separately? | Source |
|---|:---:|---|
| `WiFi.h` | ❌ No | ESP32 Arduino Core |
| `HTTPClient.h` | ❌ No | ESP32 Arduino Core |
| `WiFiClientSecure.h` | ❌ No | ESP32 Arduino Core |
| `ArduinoJson.h` | ✅ Yes | Arduino Library Manager |
| `WebServer.h` | ❌ No | ESP32 Arduino Core |
| `ESPmDNS.h` | ❌ No | ESP32 Arduino Core |
| `ESP32BLECombo.h` | ✅ Yes | Included in `prerequisites/` |

---

## 1. ESP32 Arduino Core

The following libraries are provided automatically by the **ESP32 Arduino Core**:

```text
WiFi.h
HTTPClient.h
WiFiClientSecure.h
WebServer.h
ESPmDNS.h
```

You **do not need to install these individually**.

### Installation

1. Open **Arduino IDE**.
2. Go to:

   `Tools → Board → Boards Manager`

3. Search for:

   `esp32`

4. Install:

   **esp32 by Espressif Systems**

5. Select your ESP32 board from:

   `Tools → Board → esp32`

---

## 2. ArduinoJson

The project uses:

```cpp
#include <ArduinoJson.h>
```

**ArduinoJson** is a separate library and must be installed.

### Installation

1. Open **Arduino IDE**.
2. Go to:

   `Sketch → Include Library → Manage Libraries...`

3. Search for:

   `ArduinoJson`

4. Install **ArduinoJson**.

After installation, this include should work:

```cpp
#include <ArduinoJson.h>
```

---

## 3. ESP32BLECombo

The project uses:

```cpp
#include <ESP32BLECombo.h>
```

> [!IMPORTANT]
> **ESP32BLECombo is already included in the project's `prerequisites/` folder.**
>
> You **do not need to download it separately**.

### ZIP Installation

If the `prerequisites/` folder contains an ESP32BLECombo `.zip` file:

1. Open **Arduino IDE**.
2. Go to:

   `Sketch → Include Library → Add .ZIP Library...`

3. Select the ESP32BLECombo ZIP file from the `prerequisites/` folder.
4. Restart Arduino IDE if necessary.

### Manual Installation

If ESP32BLECombo is provided as a normal folder, copy it into your Arduino libraries directory.

On Linux:

```text
~/Arduino/libraries/
```

For example:

```text
~/Arduino/libraries/ESP32BLECombo/
```

Then restart Arduino IDE.

---

## Final Installation Checklist

You only need to install:

- [ ] **ESP32 by Espressif Systems**
- [ ] **ArduinoJson**
- [ ] **ESP32BLECombo** from the project's `prerequisites/` folder

You **do not** need to separately install:

- `WiFi.h`
- `HTTPClient.h`
- `WiFiClientSecure.h`
- `WebServer.h`
- `ESPmDNS.h`

These are already provided by the **ESP32 Arduino Core**.

---

## Quick Reference

| Include | Installation |
|---|---|
| `WiFi.h` | ESP32 Arduino Core |
| `HTTPClient.h` | ESP32 Arduino Core |
| `WiFiClientSecure.h` | ESP32 Arduino Core |
| `ArduinoJson.h` | Arduino Library Manager |
| `WebServer.h` | ESP32 Arduino Core |
| `ESPmDNS.h` | ESP32 Arduino Core |
| `ESP32BLECombo.h` | `prerequisites/` folder |

## Minimum Setup

Install the **ESP32 Arduino Core**, install **ArduinoJson** through the Arduino Library Manager, and install **ESP32BLECombo** from the project's `prerequisites/` folder.

Once these are installed, all required libraries for the project should be available.
