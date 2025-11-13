# aw510m-config

aw510m-config provides a simple way to configure a HID keyboard-like device by sending binary packets directly through a Linux **hidraw** interface. It reads a configuration file, interprets defined key mappings or color settings, and transmits the corresponding HID packets to the device.

## Overview

This tool is specifically designed for configuring the **Alienware AW510M Gaming Mouse** through its HIDRAW interface on Linux systems. The mouse uses a proprietary HID packet protocol for key remapping and RGB lighting control, which this tool implements.
The program processes a text-based configuration file containing entries such as:

```
key12 = A
key13 = Enter
color = 255 128 0
```

Each configuration line triggers generation of one or more HID packets that define:

* **Key assignments** (mapping a physical key index to a HID usage code)
* **Lighting or color values** (typically RGB)

The tool writes these packets directly into a `/dev/hidrawX` device node.

## Features

* Parses `keyX = VALUE` entries and maps human-readable key names to HID usage codes.
* Parses `color = R G B` entries and converts them into RGB payloads.
* Generates unified, consistent 64-byte HID packets for each operation.
* Sends packets to a HIDRAW device using low-level write operations.
* Performs light validation and normalization of input values.
* Avoids complex dependencies; uses only standard C++17 functionality and POSIX I/O.

## How It Works

1. **Read configuration file line by line**

   * Lines beginning with `key` are treated as key mappings.
   * Lines beginning with `color` are treated as RGB LED settings.
   * Empty lines, comments (`#` or `/`) are ignored.

2. **Convert each entry into the proper packet sequence**

   * The code standardizes packet construction through a generic packet builder.
   * Each configuration entry typically produces a set of 3 sequential HID packets.

3. **Write packets to HIDRAW**

   * The packets are written directly to the device using `write()`.
   * Short delays (`usleep`) are inserted between packets to ensure device stability.

## Packet Structure

All packets sent to the device:

* Start with a predefined header (identifying command type)
* Optionally include payload bytes (key code, RGB values, etc.)
* Are padded to **exactly 64 bytes** as required by many HID devices

## Example Configuration

```
# Left Button
# key01=A

# Right Button
# key02=B

# Middle Button
# key03=C

# Back Button
# key04=A

# Forward Button
# key05=B

# Up Button
key06=PageUp

# Down Button\ nkey07=PageDown

# Bottom Button
key08=Home

# Hat Switch Up
key09=Up

# Hat Switch Down
key10=Down

#Backlight color (static)
color = 0 255 100

```
## Usage
```
Compile the tool (example):
g++ -o aw510m-config aw510m-config.cpp
```

Then run:
```
sudo ./aw510m-config config.txt /dev/hidraw3
````

## Requirements

* Linux system with HIDRAW support
* Permission to write to the `hidraw` device (may require root)

## Purpose

This tool is intended for:

* Configuring the **Alienware AW510M Gaming Mouse** using custom HID packets
* Reverse-engineering and automation of AW510M button remapping
* Adjusting AW510M RGB lighting zones via raw HID commands
* Firmware developers or power users who need low-level control of this specific device
  This tool is intended for:
* Developers reverse-engineering USB HID keyboard devices
* Custom keyboard firmware testing
* Automated remapping workflows
* RGB/LED debugging for HID-based hardware

It is **not** a general user tool; it assumes that:

* The user understands the device's proprietary packet protocol
* Wrong packets may lead to unexpected device behavior

## Disclaimer

Since the tool writes raw binary data to a USB HID device, misuse may cause:

* Temporary malfunction
* Unexpected LED behavior
* Potential need to replug/reset the device

Use with caution, preferably on hardware you control and understand.
