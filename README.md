# Mazda Tape Deck Emulator

This project provides a Tape Deck Emulator designed for the Mazda 6 (up to the 2006 model year) that features a "MD/TAPE" button. By connecting this device to the tape connector, the car's audio system will switch to "Tape mode" when the TAPE/MD button is pressed, allowing you to use the tape audio signals for playing external audio sources. It is an excellent way to add AUX functionality to your car, replacing the original tape feature. Additionally, this emulator provides playback control capabilities if you use a 4-pin audio jack.

---

## Software
The emulator is implemented in the Arduino environment and provided as an "Arduino sketch." For more details about the protocol, refer to the [Mazda Entertainment System Bus Protocol](http://nikosapi.org/w/index.php/Mazda_Entertainment_System_-_Bus_Protocol), with special thanks to Nikosapi for the documentation.

---

## Hardware
The Tape Deck Emulator uses the [Arduino Pro Micro 5V](https://www.aliexpress.com/item/1005006645661865.html?spm=a2g0o.cart.0.0.340a18fcK8edQM&mp=1) as the hardware platform. The Arduino Pro Micro is popular, easy to program, and readily available in electronic stores.

### Connections
- **Data Bus Pin**: The emulator connects to the audio system's data bus via one pin, which is configured as both input and output, depending on whether a command is being sent or received.
- **Playback Control Pin**: A second pin is used for controlling playback on your phone, similar to a single-button headset. This functionality allows start/stop and track-switching features. If you are using a 3.5mm audio jack without a microphone pin, you do not need to connect this pin.
  
### Power Supply
The device requires a 5V power supply. Unfortunately, the Mazda audio system does not provide 5V power, so you will need to use an external regulator. Here are some options:
- **Voltage Regulator Board**: Use a pre-built voltage regulator board, such as one with an LM7805 (or similar) voltage regulator chip (e.g., L7805, 78L05, REG1117-5, AMS1117-5). https://www.aliexpress.com/item/1005006486270630.html?spm=a2g0o.cart.0.0.340a18fcK8edQM&mp=1

### Audio Jack Support
The device supports a 4-pin audio jack, providing the additional feature of playback control. If you connect a microphone to the MIC pin, you can also set up a hands-free feature for your car.

### Connection Diagram
![Mazda tape deck emulator schematics](https://github.com/Krasutski/mazda_tape_deck_emulator/blob/master/doc/mazda_tape_emulator_jack_3_5_connecting.png)

---

## Compatibility
This solution is compatible with all Mazda models equipped with a "TAPE/MD" button. 

**Tested on:**
- not tested yet

---
