# blendixserialCSV

A single-header Arduino library for sending and receiving CSV-formatted coordinate data over serial, built for use with the [blendixserial Blender addon](https://electronicstree.com/blendixserial-addon/).

## What is this, and why does it exist?

[blendixserial](https://electronicstree.com/blendixserial-addon/) is a Blender addon that talks to microcontrollers over serial to drive 3D objects (position, rotation, scale) in real time. On the addon side, it supports **two wire formats**:

- **CSV Mode** : plain-text, comma-separated values ending in `;`. Always sends the full 9-value transform (3 Location + 3 Rotation + 3 Scale) per object, regardless of which axes are actually enabled.
- **Protocol Mode** : a compact, length-delimited binary packet format with a checksum, sparse axis encoding, and object indexing.

The official [blendixserial-arduino](https://github.com/electronicstree/blendixserial-arduino) library (the one installable via the Arduino Library Manager, see its [documentation](https://electronicstree.com/arduino-library-for-blendixserial-addon/)) has moved to **binary Protocol Mode only**, it no longer supports the CSV wire format on the Arduino side.

**`blendixserialCSV`** (this header) exists to fill that gap: it's a small, standalone, header-only companion for anyone who wants to keep using the simpler, human-readable **CSV format** with the addon, on boards where the binary protocol isn't needed or wanted (e.g. quick prototyping, debugging over a serial monitor, or simpler projects). It is not affiliated with or a replacement for the official binary library, install both if you need both formats, and pick whichever one matches the **Data Format** you select in the addon's Object Control panel.

> **Performance note:** CSV is a plain-text, always-send-all-9-values format, so it's noticeably slower and heavier on bandwidth than the binary protocol, especially as object count grows. It's best suited for **one or two, at most a small handful of objects**. If you're driving several objects, or need high update rates, use a **high baud rate** (115200+) with CSV, or better, switch to the official binary Protocol library for anything beyond a couple of objects.

| | CSV format | Binary Protocol format |
|---|---|---|
| Addon support | ✅ Data Format: CSV (Fixed) | ✅ Data Format: Protocol (Selective) |
| Official Arduino library (`blendixserial-arduino`) | ❌ dropped | ✅ supported |
| This header (`blendixserialCSV`) | ✅ supported | ❌ not supported |

Each transmitted or received "set" carries **9 values** (`v0` .. `v8`), matching the addon's fixed 9-value CSV layout, plus an optional trailing text string on transmit. No `.cpp` file, no separate compilation, just drop the header into your sketch and `#include` it.

## Features

- Header-only, nothing to compile or link, just one `.h` file
- Configurable number of transmit/receive sets (up to `BLENDIX_MAX_SETS`)
- Switchable coordinate storage type: `int` or `float`
- Simple CSV wire format for both transmit and receive
- Optional free-text field appended to transmitted output
- No dynamic dependencies beyond the Arduino core

## Installation

1. Download [`blendixserial_csv_ard.h`](./blendixserial_csv_ard.h).
2. Place it in your sketch folder (next to your `.ino` file), **or**
3. Place it in your Arduino `libraries/` folder (e.g. `Documents/Arduino/libraries/blendixserialCSV/blendixserial_csv_ard.h`) so it's available to all sketches.
4. In your sketch:

   ```cpp
   #include <blendixserial_csv_ard.h>
   ```

## Quick Start

```cpp
#include <blendixserial_csv_ard.h>

blendixserialCSV blendix;

void setup() {
  Serial.begin(9600);

  blendix.setCoordinateType(COORD_TYPE_FLOAT); // or COORD_TYPE_INT
  blendix.setTxSets(1);                        // number of sets to transmit
  blendix.setText("hello");                    // optional trailing text
}

void loop() {
  blendix.setCoordinates(1, 1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f, 9.9f);

  uint8_t buffer[128];
  blendix.getFormattedOutput(buffer, sizeof(buffer));
  Serial.println((char*)buffer);

  delay(1000);
}
```

Output:

```
1.10,2.20,3.30,4.40,5.50,6.60,7.70,8.80,9.90;hello
```

## Wire Format

**Transmit** (`getFormattedOutput`):

```
v0,v1,v2,v3,v4,v5,v6,v7,v8,...;OptionalText
```

9 comma-separated values per set, one set after another, a semicolon, then the optional text.

**Receive** (`parseReceivedData`):

```
v0,v1,v2,v3,v4,v5,v6,v7,v8,...;
```

9 comma-separated values per set, must end with `;`. The total value count must be a multiple of 9 or the data is rejected.

## API Reference

### Constructor

```cpp
blendixserialCSV blendix;
```

### Configuration

| Method | Description |
|---|---|
| `bool setCoordinateType(const char* type)` | `COORD_TYPE_INT` or `COORD_TYPE_FLOAT`. Switches internal storage and resets values. |
| `bool setTxSets(int sets)` | Number of sets to transmit (0..`BLENDIX_MAX_SETS`, combined with rx sets). |
| `bool setRxSets(int sets)` | Number of sets expected on receive (0..`BLENDIX_MAX_SETS`, combined with tx sets). |
| `void setText(const char* text)` | Sets the optional text appended after transmitted coordinates. |
| `void resetCoordinates()` | Zeroes all stored transmit coordinate values. |

### Transmit

```cpp
bool setCoordinates(int setNum, int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8);
bool setCoordinates(int setNum, float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8);
```
Sets the 9 values for set `setNum` (1-indexed). Use the `int` overload after `setCoordinateType(COORD_TYPE_INT)`, or the `float` overload after `setCoordinateType(COORD_TYPE_FLOAT)`.

```cpp
void getFormattedOutput(uint8_t* outputBuffer, size_t bufferSize);
```
Writes the CSV-formatted, null-terminated output into `outputBuffer`.

### Receive

```cpp
bool parseReceivedData(const String& inputData);
```
Parses a complete, semicolon-terminated CSV string into internal storage. Returns `false` if the data doesn't end in `;` or the value count isn't a multiple of 9.

```cpp
int getReceivedNumSets() const;
```
Returns how many sets were parsed from the last successful `parseReceivedData` call.

```cpp
bool getReceivedCoordinates(int index, float& v0, float& v1, float& v2, float& v3, float& v4, float& v5, float& v6, float& v7, float& v8) const;
```
Copies the 9 received values for set `index` (0-indexed) into the given references. Received values are always `float`, regardless of the tx coordinate type.

## Configuration Macros

Define these **before** the `#include` to override the defaults:

```cpp
#define BLENDIX_MAX_SETS 5           // max sets (tx + rx combined)
#define BLENDIX_TEXT_BUFFER_SIZE 50  // max length of the optional text field
#define BLENDIX_VALUES_PER_SET 9     // values per set
#include <blendixserial_csv_ard.h>
```

## Full Example: Receive and Print

```cpp
#include <blendixserial_csv_ard.h>
#include <SoftwareSerial.h>

SoftwareSerial debugSerial(10, 11);  // RX, TX
blendixserialCSV blendix;

void setup() {
  Serial.begin(9600);
  blendix.setRxSets(3);
  debugSerial.begin(9600);
}

void loop() {
  static String serialData = "";

  while (Serial.available()) {
    char receivedChar = Serial.read();
    serialData += receivedChar;

    if (blendix.parseReceivedData(serialData)) {
      printReceivedCoordinates();
      serialData = "";
    }
  }
}

void printReceivedCoordinates() {
  int numSets = blendix.getReceivedNumSets();
  for (int i = 0; i < numSets; i++) {
    float v0, v1, v2, v3, v4, v5, v6, v7, v8;
    if (blendix.getReceivedCoordinates(i, v0, v1, v2, v3, v4, v5, v6, v7, v8)) {
      debugSerial.print("Set "); debugSerial.print(i + 1);
      debugSerial.print(": v0="); debugSerial.print(v0);
      debugSerial.print(", v1="); debugSerial.print(v1);
      debugSerial.print(", v2="); debugSerial.print(v2);
      debugSerial.print(", v3="); debugSerial.print(v3);
      debugSerial.print(", v4="); debugSerial.print(v4);
      debugSerial.print(", v5="); debugSerial.print(v5);
      debugSerial.print(", v6="); debugSerial.print(v6);
      debugSerial.print(", v7="); debugSerial.print(v7);
      debugSerial.print(", v8="); debugSerial.println(v8);
    }
  }
}
```

More examples are in the [`examples/`](./examples) folder.

## Migrating old CSV sketches (3 values → 9 values per set)

Earlier CSV-based Arduino sketches (including older versions of this header) used 3 values per set (`x`, `y`, `z`). This version aligns with the addon's fixed 9-value CSV layout (3 Location + 3 Rotation + 3 Scale per object), so four changes are needed in any old sketch:

1. **Include path**
   ```cpp
   // old
   #include <blendixserial.h>
   // new
   #include <blendixserial_csv_ard.h>
   ```

2. **Class name**
   ```cpp
   // old
   blendixserial blendix;
   // new
   blendixserialCSV blendix;
   ```

3. **`setCoordinates(...)` calls** : 3 args become 9:
   ```cpp
   // old
   blendix.setCoordinates(1, x, y, z);
   // new
   blendix.setCoordinates(1, v0, v1, v2, v3, v4, v5, v6, v7, v8);
   ```

4. **`getReceivedCoordinates(...)` calls** : 3 refs become 9:
   ```cpp
   // old
   blendix.getReceivedCoordinates(0, x, y, z);
   // new
   blendix.getReceivedCoordinates(0, v0, v1, v2, v3, v4, v5, v6, v7, v8);
   ```

> **Note:** The wire format also changed to match the addon, anything sending data to the device must now send 9 comma-separated values per set (not 3), still ending with `;`. Old 3-value senders are not compatible without updating them as well.

## Related links

- [blendixserial Addon](https://electronicstree.com/blendixserial-addon/) : the Blender addon (CSV + binary Protocol Mode)
- [blendixserial-arduino](https://github.com/electronicstree/blendixserial-arduino) : official Arduino library (binary Protocol Mode only)
- [blendixserial-arduino documentation](https://electronicstree.com/arduino-library-for-blendixserial-addon/)

## License

The blendixserialCSV header file  is distributed under the GNU General Public License. Please refer to the license text for more details. Remember to comply with the license terms and ensure you have the necessary permissions to use the add-on.
