# DucatiPanigaleCanBus

- [DucatiPanigaleCanBus](#ducatipanigalecanbus)
  - [What is DucatiPanigaleCanBus?](#what-is-ducatipanigalecanbus)
  - [What data is collected?](#what-data-is-collected)
    - [Transmitted at 20Hz or 25Hz](#transmitted-at-20hz-or-25hz)
    - [Transmitted at 1Hz](#transmitted-at-1hz)
  - [Setup of Harry Lap Timer](#setup-of-harry-lap-timer)
    - [Connect to a specific unit](#connect-to-a-specific-unit)
  - [Building the software in Platform IO and Arduino IDE](#building-the-software-in-platform-io-and-arduino-ide)
    - [Simulating CANbus values to troubleshoot BLE](#simulating-canbus-values-to-troubleshoot-ble)
    - [OTA updates with DoubleResetDetector](#ota-updates-with-doubleresetdetector)
  - [Schematics](#schematics)
  - [Hardware](#hardware)
    - [An example based on common DevKit ESP32 module](#an-example-based-on-common-devkit-esp32-module)
    - [An example of a compact unit based on Wemos D1mini 32](#an-example-of-a-compact-unit-based-on-wemos-d1mini-32)
  - [Credits](#credits)

## What is DucatiPanigaleCanBus?

If you are a motorcycle rider/racer, you might want to log data from your bike to improve your riding style.
This project connects a small device based on ESP32 to the CAN Bus on a Ducati Panigale (or other Ducati bikes with similar ECU), it monitors and extracts RPM/speed/gear/temperature, and it broadcasts the collected data via Bluetooth Low Energy to Harry Lap Timer 'Engine sensor'. Other apps can be supported (for example, [NBP in TrackAddict](https://racerender.com/TrackAddict/docs/NBP%20Specification.pdf)), not available today.

This code is specialized for messages generated by the Mitsubishi ECU used in some models like

- Panigale (899/959/1199/1199S/1199R/1299/1299S/R and Superleggera)
- Multistrada 1200 (2010 to 2014)
- Diavel, Diavel Strada, Diavel DS

Right now this is compatible only with BLE which is the preferred option if you are using an iOS device with Harry Lap Timer: extending to regular bluetooth or WiFi/TCP-IP is not complicated, yet not on the priority list right now - please log a request if you are really interested and willing to help in the extension.

## What data is collected?

The choice of data is a combination of what could be useful and what is supported by HLT channels, right now:

### Transmitted at 20Hz or 25Hz

- Wheel speed
- Engine RPM
- Gear
- Throttle position

Define the frequency with the macro `FAST_MESSAGES_FREQUENCY_HZ`

### Transmitted at 1Hz

- Engine coolant temperature
- Air temperature
- Battery voltage

Measurements are mapped to the channels built in HLT, so you can inspect data directly there and export it in other formats for analysis outside of the app (think VBO format for example, or CSV).

As of Jun 2021, Gear collected from the engine is not exported by HLT even if collected.

![Engine](docs/using/hlt_engine.png)
![Throttle](docs/using/hlt_throttle.png)
![Temperature](docs/using/hlt_temperature.png)

## Setup of Harry Lap Timer

At this time, the script running on Harry Lap Timer needs to be made available with instructions [that are described in the HLT developer forum](http://forum.gps-laptimer.de/index.php).

There is no further configuration required on HLT: once the script is available and your ESP32 flash and connected, HLT will discover the device (based on the device ID naming convention, the BLE Service, and the Characteristics IDs defined in the ESP32 code and in the LUA script)

To pair the device to your phone (so that other phones with the same setup don't randomly pick up your sensor), use the pairing code defined in `BLE_SECURITY_PASS`, e.g.

```C
// Define the PIN requested by Bluetooth Pairing for bonding
#define BLE_SECURITY_PASS 123456
```

![Device found](docs/sensor_found.png)
![Sensor info](docs/sensor_info.png)
![Sensor live data](docs/sensor_data.png)

### Connect to a specific unit

HLT will connect to any nearby device matching a name of "DuCan-...." where the 4 dots match an id unique to your device.

If you need to connect to a specific device

- Open Administration
- go to Settings
- scroll down to Expert Settings
- scroll down to Custom BTLE ODB Adapter
- in 'Peripheral Name' add the full name of your device, e.g. `DuCan-A499` or `DuCan-8409`. You can once again use a single `.` to specify *any character*

![Expert Settings > Custom peripheral name](docs/config_customperipheral_name.png)

In this way, if other riders have this same unit nearby your phone will only connect to the device you specify.

## Building the software in Platform IO and Arduino IDE

This project is best handled with Platform.io. Nonetheless, it uses the Arduino framework so you can use the Arduino IDE as well.

Steps required to compile this in the Arduino IDE

- copy content of `src` and `include` into your sketch folder
- perform a `git clone https://github.com/timurrrr/arduino-CAN` in the sketch folder, or copy a zip file from github directly
- move the content of `arduino-CAN/src` in the main folder of your sketch

### Simulating CANbus values to troubleshoot BLE

Having the device connected to a running bike to generate data while troubleshooting BLE connectivity from the console log is quite inconvenient: you can enable a "data simulator" that will linearly increase data in a loop without a live CANbus connection.

Define `CAN_DATA_SIMULATOR_MODE` or add it to the build command line (as in the example *doit_simulateCAN* environment).

With logging enable, your serial will show

```plaintext
[   364][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 0, 80: 0, 100: 0], values RPM: 0, Speed: 0, APS: 0, Gear: 0, ETemp: 0, ATemp 0, Battery: 0. SLOW [00000000], FAST [00000000000000]
[  1364][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 14, 80: 14, 100: 14], values RPM: 2140, Speed: 9, APS: 2, Gear: 1, ETemp: 70, ATemp 16, Battery: 120. SLOW [00000000], FAST [00000000000000]
[  2364][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 24, 80: 24, 100: 24], values RPM: 2240, Speed: 12, APS: 3, Gear: 1, ETemp: 70, ATemp 16, Battery: 120. SLOW [00000000], FAST [00000000000000]
[  3364][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 34, 80: 34, 100: 34], values RPM: 2340, Speed: 15, APS: 4, Gear: 1, ETemp: 71, ATemp 16, Battery: 120. SLOW [00000000], FAST [00000000000000]
[  4364][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 44, 80: 44, 100: 44], values RPM: 2440, Speed: 17, APS: 5, Gear: 1, ETemp: 71, ATemp 17, Battery: 120. SLOW [00000000], FAST [00000000000000]
```

and with a BLE client attached

```plaintext
[312367][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 3124, 80: 3124, 100: 3124], values RPM: 3180, Speed: 39, APS: 12, Gear: 1, ETemp: 74, ATemp 18, Battery: 122. SLOW [024A127A], FAST [016C0C27000C01]
[313367][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 3134, 80: 3134, 100: 3134], values RPM: 3280, Speed: 42, APS: 13, Gear: 1, ETemp: 75, ATemp 19, Battery: 122. SLOW [024A127A], FAST [01D00C2A000D01]
[314367][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 3144, 80: 3144, 100: 3144], values RPM: 3380, Speed: 45, APS: 14, Gear: 1, ETemp: 75, ATemp 19, Battery: 122. SLOW [024B137A], FAST [01340D2D000E01]
[315367][I][canbusble.cpp:314] report_msg_counters(): Counters [18: 3154, 80: 3154, 100: 3154], values RPM: 3480, Speed: 48, APS: 15, Gear: 1, ETemp: 75, ATemp 19, Battery: 122. SLOW [024B137A], FAST [01980D30000F01]
```

### OTA updates with DoubleResetDetector

When the unit is tucked away, updates via USB are inconvenient. The "Double Reset Detector" library will be triggered after a double "reset" activity within 5 seconds (configurable), with these series of actions

- start WiFi in Access Point mode
- start OTA in listening mode

The AP name is based on the `DEVICE_ID`  macro, and it includes the 4 digits identifying the unit. At that point, mDNS starts and after connecting to the ESP32 Access Point you can upload to the `ducan.local` device.

- To activate the OTA functionality, define the compiler marco `ENABLE_OTA_WITH_DRD`
- Define the Access Point password with macro `WIFI_PWD` (default `123456789`)
- Press Reset once, wait 1 second, press Reset again to enable

**Note**: the double reset needs about 1 second between each action for the bootstrap procedure to correclty identify the reset.

## Schematics

![schematics](docs/Schematic_DuCanBus_2021-02-25.png)

The most complicated and hard to source part is the 4 pin adapter to the DDA port (check [monocilindro.com for a nice article on connections](https://www.monocilindro.com/2018/08/26/ducati-monster-797-obd2-dda-diagnostic-connector-and-communication/)

- pin1: +12
- pin2: Ground
- pin3: CAN high
- pin4: CAN low

You can also buy an ODBII adapter and use th ODBII pin schema convention.

In a nutshell:

- Pin1 (+12) and Pin2 (ground) connect to the buck converter to power the ESP32
- Pin3 and Pin4 carry the CANBUS signal going to the transceiver

Since you can use any pin for TWAI, there are 2 options

- use predefined options in [include/canbusble_pinout.h](include/canbusble_pinout.h)
- define via compile time definitions of `TX_GPIO_NUM` and `RX_GPIO_NUM`

## Hardware

Parts: `U<x>` refers to the schematic reported above. Links to sources are examples, these are very generic components.

- U1: A generic ESP32
- U2: [A CANBUS transceiver, such as one based on SN65HVD230](https://www.amazon.com/gp/product/B07ZT7LLSK). Be careful to source a genuine part
- U3: [Buck converter 12V to 5V](https://www.amazon.com/gp/product/B076P4C42B) to power ESP32 from the CANBUS directly
- U4: [4 PINs adapter for Ducati DDA port (male)](https://www.aliexpress.com/item/4001007307044.html)

Misc parts from your HW bin

- U5: small switch (to cut power when not needed) inline with the Input power
- heat shrink tube to protect the cable from the 4 PINs adapter

![U2](docs/U2_transceiver.jpeg)
![U3](docs/U3_buck_adapter.jpeg)
![U4](docs/U4_4pin_adapter.jpeg)
![U5](docs/U5_switch_small.jpeg)

### An example based on common DevKit ESP32 module

The most common ESP32 is the DevKit: the box is 3d printed, [here the source files for DevKit](docs/DuCanBus_DevKit.f3d) of which you should print *box* and *lid*. You will need 4 M1.7 screws 4 to 6 mm long, and a lot of patience:

- hot glue to hold pieces together in a firm location, to avoid issues with vibrations, is recommended
- the transceiver is designed to be locked in position by the two columns: you can melt some plastic to ensure it does not move around

![open](docs/doit_open.jpg)

### An example of a compact unit based on Wemos D1mini 32

The smallest esp32 readily available is the D1 mini ESP32: fitting everything in a box that can go in the tail section of the Panigale has been tediously meticolous, but the result works: the box is 3d printed, [here the source files for D1 Mini](docs/DuCanBus.f3d) of which you should print *box* and *lid*. You will need a few M1.7 screws 4 to 5 mm long, and a lot of patience:

- hot glue to hold pieces together in a firm location, to avoid issues with vibrations, is recommended
- the transceiver is designed to be locked in position by the two columns: you can melt some plastic to ensure it does not move around

![open](docs/d1mini32_open.jpg)

Once closed

![closed](docs/d1mini32_closed.jpg)

## Credits

Other projects that inspired this work are

- The excellent work performed by MrCanBus [in this thread](https://www.ducati.ms/threads/canbus-data-on-you-android-device-via-bluetooth.337705/) and his [CanBus sniffer](https://github.com/MrCanBus/MTS1200-CANBUS)
- Collective decoding of Ducati CANBUS messages ([spreadsheet](https://docs.google.com/spreadsheets/d/1-NJ9OlGQYTGMzBzwDPYn-aI_7_ign9SCiscKZufx3Uw/edit?pli=1#gid=1950998351))
- Field testing and motivation by [@dookie454](https://github.com/dookie454), including porting to car PIDs and TFT display of data
