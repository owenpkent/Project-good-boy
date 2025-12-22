# Project Good Boy

An open-source battery powered dog treat dispenser that is activated via a web interface and is wheelchair mountable.
It is currently compatible with being mounted on 40x40 aluminum extrusions. 

**Want one built for you? Or modifications/additional support?** Reach out—we'd be happy to help!

## Contact

Owen Kent - https://okstud.io/

Marshall Saltz - https://saltztech.com/

---

## Features

- **Wi-Fi Control** — Operates on your local network or as its own access point
- **Web Interface** — Phone-friendly UI served directly from the device
- **Battery Powered** — Uses a 12 V Lipo to power the system
- **Accessible Design** — Large buttons, simple controls, wheelchair-mountable
- **Modular** — Components of your choosing can be easily added to or removed from the system
---

## Bill of Materials

| Component | Amount | Link |
|-----------|-------------|------|
| Battery | 1 | [Amazon](https://www.amazon.com/dp/B0D9D8ZSV9?ref=ppx_yo2ov_dt_b_fed_asin_title) |
| Nema 17 Stepper Motor | 1 | [Amazon](https://www.amazon.com/dp/B07PNV7RBW) |
| A4988 Stepper Motor Driver | 1 | [Amazon](https://www.amazon.com/your-orders/order-details?orderID=112-0511300-4533845) |
| 12V to 5V DC Converter | 1 | [Amazon](https://www.amazon.com/dp/B08VHZJ3C8) |
| M3 Bolts | 4 | [Amazon](https://www.amazon.com/dp/B0C7ZRTH3Q) |
| Jumper Wires | —| — |
| Filament | — | — |
| T-nuts for mounting | 5 | — |
| T-nut compatible bolts | 5 | — |
| ESP32 Dev Kit | 1 | — |
| Wagu Wire Connectors | — | — |
| ArtResin or another safe resin | — | — |

### Tools Required

- Pliers
- Electronics screwdriver
- USB-C cable (for uploading firmware and charging)
- 3D printer

---

## Wiring

![Schematic](media/goodboy_schematic.png)

---

## Building & Uploading

### Assembly Instructions

Assembly video or image series needed here!

### Prerequisites

Install [Arduino IDE](https://www.arduino.cc/en/software)
Required libraries:
   - `Wifi`
   - `ESPAsyncWebServer.h`
   - `AsyncTCP`
   - `DNSServer`
   - `ESPmDNS`
   - `LittleFS` https://randomnerdtutorials.com/esp32-littlefs-arduino-ide/

### Upload Firmware

1. Connect the ESP32 via USB-C and open Arduino IDE
   
2. Upload the sketch

3. Upload the data folder to the ESP32 using LittleFS

## Usage

### First Boot (Access Point Mode)

1. Power on the device
2. Connect to Wi-Fi network: **GoodBoy** (password: `buddythedog`)
3. Open browser: `http://192.168.4.1`
4. Enter your home Wi-Fi credentials to connect the device to your network

### Normal Operation

1. Connect to the same Wifi network as the device
2. Open browser: `http://goodboy.local` (or check serial output for IP)
3. Use the web interface:
   - **Dispense** — Triggers the motor to dispense a treat
   - **Speed** — Adjusts motor speed (1–20)

Can also be used in AP mode by following steps 1-3 of First Boot

---

## TODO

- [ ] Include assembly instructions/video
- [ ] Remove vacuum code from ino file
- [ ] Maintainance instructions 
---

## Contributing

Issues and PRs welcome! Please open an issue before proposing major changes.

---

## License

MIT License — see `LICENSE`.

---

## Credits

Built with love for a very good boy.
