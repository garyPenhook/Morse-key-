# Morse Key Decoder

A Linux GUI application for decoding Morse code from a CW Morse "My Key Serial" USB adapter or compatible device.

## Features

- Real-time Morse code decoding to text
- Sidetone audio feedback while keying
- Adaptive timing algorithm (auto-adjusts to your speed)
- Adjustable WPM (5-50 words per minute)
- Configurable sidetone frequency and volume
- Settings persistence between sessions
- Copy decoded text to clipboard

## Requirements

- Linux Mint (or compatible Linux distribution)
- Qt6 development libraries
- CW Morse "My Key Serial" adapter or compatible USB-to-serial Morse key interface

## Installation

### Install Dependencies

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-serialport-dev qt6-multimedia-dev
```

### Build from Source

```bash
git clone https://github.com/garyPenhook/Morse-key-.git
cd Morse-key-
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Serial Port Access

Add your user to the `dialout` group to access serial ports:

```bash
sudo usermod -a -G dialout $USER
```

**Log out and log back in** for the group change to take effect.

## Usage

### Running the Application

```bash
./build/morse-decoder
```

### Connecting to Your Key

1. Plug in your Morse key USB adapter
2. Click **Refresh** to detect available serial ports
3. Select your device from the Port dropdown (typically `/dev/ttyUSB0` or `/dev/ttyACM0`)
4. Set the baud rate (default 9600 works for most devices)
5. Click **Connect**

### Decoding Morse

Once connected, the application will:
- Display dits (.) and dahs (-) as you key them in the "Current Input" area
- Decode complete characters and show them in the "Decoded Text" area
- Play sidetone audio while keying (if enabled)

### Settings

- **WPM**: Adjust expected words per minute (5-50). The decoder adapts automatically, but this sets the initial timing.
- **Sidetone**: Enable/disable audio feedback
- **Frequency**: Sidetone pitch in Hz (200-1500)
- **Volume**: Sidetone loudness

### Controls

- **Clear**: Erase decoded text and reset decoder
- **Copy**: Copy decoded text to clipboard

## Serial Protocol Support

The application supports multiple protocols:

1. **Timing Events**: Device sends `K1` (key down) and `K0` (key up)
2. **Character Mode**: Device sends `.` (dit) and `-` (dah) characters

## Troubleshooting

### "Permission denied" error
Ensure you're in the `dialout` group and have logged out/in.

### No ports detected
- Check that the USB adapter is plugged in
- Try a different USB port
- Run `dmesg | tail` to see if the device was recognized

### Decoded characters are wrong
- Adjust WPM to better match your sending speed
- The decoder adapts over time - keep sending

### No sidetone audio
- Check system volume
- Verify sidetone is enabled in settings
- Check that Qt6 Multimedia is installed

## License

See [LICENSE](LICENSE) file.

## Contributing

Contributions welcome! Please see [CLAUDE.md](CLAUDE.md) for technical design details.
