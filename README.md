# TAPIoT Devices
TAPIoT is a project that can bring wisdom to your taps.

[Tapiot-arduino](https://github.com/bdsword/tapiot-arduino) is a sub-project under TAPIoT, which is the hardware part of TAPIoT. Therefore, tapiot-arduino should be uploaded to your Arduino.


## Requirements
1. 12V DC power supply * 3
2. Wifi enabled environment

## Installation

1. Clone this repository.
    ```shell
    $ git clone https://github.com/bdsword/tapiot-arduino.git
    ```

2. Setup arduino devices client configurations.
    ```shell
    $ cp config.sample.h config.h
    # edit config.h to customize your requirements
    ```

3. Upload main.ino to Arduino UNO R3, and enjoy it!

## Customize Devices Wiring

According to [RC522 Library](https://github.com/miguelbalboa/rfid), pin **11**, **12**, and **13** are **NOT** configurable. Besides, **SENSOR_PIN** should correspond to **SENSOR_INTERRUPT**, and not all the pins are able to be set as interrupt pin, please check [arduino reference](https://www.arduino.cc/en/Reference/AttachInterrupt) for more information.

# TAPIoT Service Side
Please check [tapiot](https://github.com/bdsword/tapiot) repository.
