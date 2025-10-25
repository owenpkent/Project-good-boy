# Project Good Boy - Wheelchair-Mountable Dog Treat Dispenser

A battery-powered, Wi-Fi-controlled dog treat dispenser designed for wheelchair users. Easily train your service dog or give your pet treats with a simple web interface accessible from your phone or tablet.

## Overview

**Good Boy** is an ESP32-based device that:
- Creates its own Wi-Fi access point (`GoodBoy` network)
- Serves a simple, responsive web UI for controlling treat dispensing
- Uses a stepper motor to precisely dispense treats in forward or reverse direction
- Runs on battery power (suitable for wheelchair mounting)
- Allows speed control (1–15 RPM) for different treat sizes

## Hardware Requirements

### Core Components
- **ESP32 Dev Module** (or compatible ESP32 board)
- **28BYJ-48 Stepper Motor** (unipolar, 2048 steps per revolution)
- **ULN2003 Stepper Driver Module**
- **Power Supply**: 
  - 5V for ESP32 (USB or battery pack)
  - 5V for stepper motor (separate supply recommended)
- **Battery Pack**: 5V USB power bank (10,000+ mAh recommended for all-day use)

### Pin Connections (ESP32 → ULN2003)
| ESP32 Pin | ULN2003 Pin | Purpose |
|-----------|-------------|---------|
| GPIO 5    | IN1         | Stepper coil 1 |
| GPIO 19   | IN2         | Stepper coil 2 |
| GPIO 18   | IN3         | Stepper coil 3 |
| GPIO 21   | IN4         | Stepper coil 4 |
| GND       | GND         | Ground |

### Mechanical Assembly
- Mount the stepper motor to a treat hopper or dispenser mechanism
- Attach a screw or gear to the motor shaft to control treat flow
- Secure the ESP32 and ULN2003 driver to the wheelchair frame or mounting bracket
- Use a battery pack mounted on the wheelchair for power

## File Structure

```
firmware/
├── good_boy_stepper_ap.ino    # Main sketch (WiFi, web server, motor control)
├── index.html                  # Web UI (served from SPIFFS)
└── data/
    └── index.html              # Copy of index.html for SPIFFS upload
```

## Building and Uploading with Arduino CLI

### Prerequisites
1. Install Arduino CLI: https://arduino.cc/en/software#arduino-cli
2. Install ESP32 board support:
   ```bash
   arduino-cli core install esp32:esp32
   ```

### Step 1: Prepare the project structure
```
firmware/
├── good_boy_stepper_ap.ino
├── index.html
└── data/
    └── index.html
```

Copy `index.html` into the `data/` folder so it gets uploaded to SPIFFS.

### Step 2: Compile the sketch
```bash
cd firmware
arduino-cli compile --fqbn esp32:esp32:esp32 good_boy_stepper_ap.ino
```

### Step 3: Upload the sketch
```bash
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 good_boy_stepper_ap.ino
```
Replace `COM3` with your ESP32's serial port (use `arduino-cli board list` to find it).

### Step 4: Upload the filesystem (index.html to SPIFFS)
Arduino CLI doesn't natively support SPIFFS uploads. Use one of these alternatives:

**Option A: Use esptool.py (recommended)**
```bash
pip install esptool
python -m esptool --chip esp32 --port COM3 --baud 921600 write_flash -z 0x290000 spiffs.bin
```

**Option B: Use Arduino IDE's ESP32 Sketch Data Upload tool**
1. Open the sketch in Arduino IDE
2. Tools → ESP32 Sketch Data Upload
3. This uploads the `data/` folder to SPIFFS

**Option C: Use PlatformIO (alternative build system)**
```bash
pio run -t uploadfs -t upload
```

## How It Works

### Web Interface
1. Power on the device
2. Connect to Wi-Fi network: **GoodBoy** (password: `buddythedog`)
3. Open browser and navigate to `http://192.168.4.1`
4. You'll see the control panel with:
   - **Dispense button**: Triggers one full motor revolution
   - **Direction selector**: Choose `Forward` or `Reverse`
   - **Speed control**: Set RPM (1–15, default 15)
   - **Status display**: Shows current state and results

### Motor Control
- **Forward**: Motor rotates clockwise (default treat dispensing)
- **Reverse**: Motor rotates counter-clockwise (useful for clearing jams or reversing the mechanism)
- **Speed**: Controls RPM; lower speeds for precise control, higher speeds for faster dispensing
- **One revolution**: Each "Dispense" action runs exactly 2048 steps (one full rotation)

### Serial Output
Connect to the ESP32 via USB and open Serial Monitor (115200 baud) to see:
- Access point startup messages
- Client connection logs
- Motor command details (direction, speed, RPM)
- Status updates

Example output:
```
Access Point started!
Connect to: GoodBoy
Open browser at: http://192.168.4.1
New client connected
Running one revolution (forward, 15 RPM)
Client disconnected
```

## Code Structure

### `good_boy_stepper_ap.ino`

**Setup Phase**
- Initializes Serial communication (115200 baud)
- Mounts SPIFFS file system
- Configures ESP32 as Wi-Fi Access Point
- Starts web server on port 80

**Main Loop**
- Waits for HTTP client connections
- Parses incoming HTTP requests
- Routes to either web page serving or motor command handling

**`sendWebPage()`**
- Reads `index.html` from SPIFFS
- Sends it as HTTP response with proper headers

**`handleRunCommand()`**
- Parses query parameters: `dir` (forward/reverse) and `speed` (1–15)
- Validates and clamps speed values
- Executes motor control:
  - Sets motor speed via `Stepper.setSpeed()`
  - Runs exactly 2048 steps in the chosen direction
  - Adds 1ms delay between steps for precise timing
- Returns plain-text response to client

### `index.html`

**UI Components**
- Dark theme (background: `#111827`, accent blue: `#1d4ed8`)
- Responsive design (works on phones, tablets, desktops)
- Direction dropdown (forward/reverse)
- Speed input (number field, 5–15 RPM)
- Dispense button (large, easy to tap)
- Status footer (shows real-time feedback)

**JavaScript**
- `run()` function handles button clicks
- Sends `fetch()` request to `/run?dir=...&speed=...`
- Updates status display with server response

## Usage Tips

### For Dog Training
1. Mount the device on your wheelchair at a comfortable height
2. Fill the hopper with small training treats
3. Connect to the Wi-Fi network from your phone
4. Use the web interface to dispense treats during training sessions
5. Adjust speed for different treat sizes (slower = smaller portions)

### For Maintenance
- **Jam clearing**: Use the Reverse direction to back out stuck treats
- **Testing**: Start with Forward at low speed (5 RPM) to verify motor operation
- **Battery life**: A 10,000 mAh power bank typically lasts a full day of moderate use

### Troubleshooting
- **No Wi-Fi network visible**: Check ESP32 power and serial output
- **Can't connect to web interface**: Verify you're on the `GoodBoy` network and try `http://192.168.4.1`
- **Motor not spinning**: Check GPIO pin connections and ULN2003 power supply
- **Treats not dispensing**: Verify motor direction and hopper mechanism alignment

## Power Considerations

- **ESP32 current draw**: ~80–160 mA (Wi-Fi active)
- **Stepper motor current draw**: ~200–400 mA (during operation)
- **Recommended battery**: 5V, 10,000+ mAh USB power bank
- **Expected runtime**: 8–12 hours of moderate use

## Future Enhancements

- Treat counter (track total dispensed)
- Scheduling (dispense at set intervals)
- Mobile app (instead of web interface)
- Multiple motor support (for different treat types)
- Treat jam detection (current sensing)
- Battery level monitoring

## License

Open source. Feel free to modify and adapt for your needs.

## Support

For issues or questions, check the serial output and verify:
1. ESP32 is powered and connected
2. SPIFFS contains `index.html`
3. Motor pins are correctly connected
4. ULN2003 driver has adequate power supply
