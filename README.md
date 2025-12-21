# Project Good Boy

An open-source, battery-powered dog treat dispenser built around an ESP32. Control it from your phone via a simple web interface—no cloud, no app install required.

**Want one built for you?** Open an issue or reach out—we'd be happy to help!

---

## Features

- **Wi-Fi Control** — Operates on your local network or as its own access point
- **Web Interface** — Phone-friendly UI served directly from the device
- **No Cloud Required** — Everything runs locally on the ESP32
- **Battery Powered** — Uses 18650 Li-ion cells with USB-C charging
- **Accessible Design** — Large buttons, simple controls, wheelchair-mountable

---

## Bill of Materials

See `BOM.xlsx` for the complete parts list. Key components:

| Component | Description | Link |
|-----------|-------------|------|
| ESP32 Dev Kit | ELEGOO ESP-WROOM-32 | [Amazon](https://www.amazon.com/ELEGOO-ESP-WROOM-32-Development-Bluetooth-Microcontroller/dp/B0D8T53CQ5/) |
| Stepper Motor + Driver | 28BYJ-48 with ULN2003 or A4988 | [Amazon](https://www.amazon.com/ELEGOO-28BYJ-48-ULN2003-Stepper-Arduino/dp/B01CP18J4A) |
| 5V Li-ion UPS | DWEII Battery Charger/Converter | [Amazon](https://www.amazon.com/DWEII-Lithium-Battery-Charger-Converter/dp/B0DCVRXTW8/) |
| 18650 Batteries | (2) Lithium Ion cells | — |
| Wire Connectors | Assorted pack | [Amazon](https://www.amazon.com/Connectors-Conductor-Combination-Assortment-Connection/dp/B09CKDWK4Q/) |
| Mounting Hardware | Bolts, nuts, washers | — |

### Tools Required

- Soldering iron
- Pliers
- Screwdriver
- USB-C cable (for uploading firmware and charging)

---

## Wiring

| ESP32 Pin | Connection | Purpose |
|-----------|------------|---------|
| GPIO 5 | A4988 DIR | Stepper direction |
| GPIO 19 | A4988 STEP | Stepper pulse |
| GPIO 27 | Relay IN | Relay control |
| GND | Common ground | — |

See `system_schematic.drawio` for the full schematic.

---

## Repository Structure

```
project-good-boy/
├── esp32_system_owen.ino      # Main firmware (Arduino)
├── data/                      # Web UI files (uploaded to LittleFS)
│   ├── index.html             # Control panel
│   └── main.css               # Styles
├── models/                    # 3D printable parts
│   ├── agitator_1.stl
│   ├── bottom.stl
│   ├── disp_4.stl
│   ├── holder_5.stl
│   ├── lid_5.stl
│   ├── middle.stl
│   ├── spacer.stl
│   ├── spinner_5.stl
│   ├── top.stl
│   └── vacuum_mount.stl
├── BOM.xlsx                   # Bill of materials
├── system_schematic.drawio    # Circuit schematic
├── LICENSE
├── CONTRIBUTING.md
└── README.md
```

---

## Building & Uploading

### Prerequisites

1. Install [Arduino IDE](https://www.arduino.cc/en/software) or [Arduino CLI](https://arduino.github.io/arduino-cli/)
2. Install ESP32 board support:
   ```
   arduino-cli core install esp32:esp32
   ```
3. Install required libraries:
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `LittleFS`

### Upload Firmware

1. Connect the ESP32 via USB-C
2. Find your port:
   ```powershell
   arduino-cli board list
   ```
3. Upload the sketch:
   ```powershell
   arduino-cli compile -b esp32:esp32:esp32 esp32_system_owen.ino
   arduino-cli upload -b esp32:esp32:esp32 -p COM3 esp32_system_owen.ino
   ```

### Upload Web Files (LittleFS)

Upload the `data/` folder to the ESP32's filesystem using the [Arduino ESP32 LittleFS Uploader](https://github.com/lorol/arduino-esp32littlefs-plugin) or the Arduino IDE plugin.

---

## Usage

### First Boot (Access Point Mode)

1. Power on the device
2. Connect to Wi-Fi network: **GoodBoy** (password: `buddythedog`)
3. Open browser: `http://192.168.4.1`
4. Enter your home Wi-Fi credentials to connect the device to your network

### Normal Operation

1. Connect to the same Wi-Fi network as the device
2. Open browser: `http://goodboy.local` (or check serial output for IP)
3. Use the web interface:
   - **Dispense** — Triggers the motor to dispense a treat
   - **Speed** — Adjust RPM (1–20)
   - **Pick Up** — Toggles the relay (for vacuum or other accessories)

### Serial Monitor

Connect via USB and open Serial Monitor at **115200 baud** to see:
- Connection status
- IP address
- Motor commands
- Debug output

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No Wi-Fi network visible | Check power; verify ESP32 LED is on |
| Can't access web interface | Confirm you're on the GoodBoy network; try `http://192.168.4.1` |
| Motor not spinning | Check wiring; verify GPIO pins match your setup |
| Treats not dispensing | Adjust speed; check auger alignment |

---

## Contributing

Issues and PRs welcome! Please open an issue before proposing major changes.

---

## License

MIT License — see `LICENSE`.

---

## Credits

Built with love for a very good boy.
