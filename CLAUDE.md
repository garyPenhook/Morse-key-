# Morse Key Decoder - Design Document

## Project Overview

A Linux Mint GUI application that interfaces with the CW Morse "My Key Serial" USB adapter to decode Morse code input from a paddle/straight key and display the decoded text in real-time.

## Device Information

**Target Device**: CW Morse "My Key Serial" Morse Code Key to Serial USB Adapter
- Presents as USB serial device (typically `/dev/ttyUSB0` or `/dev/ttyACM0`)
- Connects to iambic paddles or straight keys
- Compatible with various Morse keying software

## Technical Architecture

### Technology Stack

| Component | Technology | Rationale |
|-----------|------------|-----------|
| Language | C++17 | Performance, native Qt integration |
| GUI Framework | Qt6 | Modern, feature-rich, excellent Linux support |
| Serial Communication | QSerialPort | Native Qt module, cross-platform |
| Build System | CMake | Industry standard for C++ projects |
| Audio Feedback | QSoundEffect | Optional sidetone generation |

### System Components

```
┌─────────────────────────────────────────────────────┐
│                    Main Window                       │
├─────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────────────┐   │
│  │ Serial Config   │  │   Decoded Text Display  │   │
│  │ - Port select   │  │   (scrolling output)    │   │
│  │ - Baud rate     │  │                         │   │
│  │ - Connect btn   │  │                         │   │
│  └─────────────────┘  └─────────────────────────┘   │
│  ┌─────────────────┐  ┌─────────────────────────┐   │
│  │ Morse Settings  │  │   Current Character     │   │
│  │ - WPM           │  │   (dit/dah visualization)│   │
│  │ - Dit/Dah ratio │  │                         │   │
│  └─────────────────┘  └─────────────────────────┘   │
│  ┌─────────────────────────────────────────────┐    │
│  │            Status Bar                        │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

### Core Classes

#### 1. `SerialHandler` - Serial Communication
- Auto-detect available serial ports
- Connect/disconnect management
- Read serial data using Qt signals/slots
- Support multiple protocols:
  - **Character mode**: Device sends ASCII characters
  - **Timing mode**: Device sends key-down/key-up events with timestamps
  - **DTR/RTS mode**: Uses control lines for key state

#### 2. `MorseDecoder` - Morse Code Decoding
- Real-time timing analysis
- Dit/dah discrimination based on element duration
- Character/word boundary detection
- Support for:
  - International Morse Code
  - Prosigns (AR, SK, BT, etc.)
  - Adjustable WPM (5-50 WPM)

#### 3. `MainWindow` - GUI Application
- Serial port configuration panel
- Real-time decoded text display
- Visual dit/dah indicator
- Settings persistence
- Clear/copy/save functionality

#### 4. `MorseTable` - Morse Code Lookup
- Complete International Morse Code table
- Bidirectional lookup (code↔character)
- Prosign support

## Morse Code Timing

Standard timing relationships (PARIS standard):
- **Dit duration** = 1 unit
- **Dah duration** = 3 units
- **Inter-element gap** = 1 unit
- **Inter-character gap** = 3 units
- **Inter-word gap** = 7 units

At 20 WPM: 1 unit ≈ 60ms

### Adaptive Timing Algorithm

```cpp
// Distinguish dit from dah
if (elementDuration < (ditAvg + dahAvg) / 2) {
    element = Element::Dit;
} else {
    element = Element::Dah;
}

// Update running averages
if (element == Element::Dit) {
    ditAvg = 0.8 * ditAvg + 0.2 * elementDuration;
} else {
    dahAvg = 0.8 * dahAvg + 0.2 * elementDuration;
}
```

## Serial Protocol Support

### Protocol 1: Timing Events (Primary)
Device sends key state with timing:
```
K1\n  - Key down
K0\n  - Key up
```
Application measures duration between events.

### Protocol 2: Character Mode
Device sends characters representing elements:
```
.  - Dit
-  - Dah
   - Element gap
/  - Character gap
```

### Protocol 3: Raw Serial (Fallback)
Monitor DTR/CTS lines for key state changes.

## File Structure

```
Morse-key-/
├── CLAUDE.md              # This design document
├── README.md              # User documentation
├── LICENSE                # Project license
├── CMakeLists.txt         # Main CMake configuration
├── src/
│   ├── CMakeLists.txt     # Source CMake configuration
│   ├── main.cpp           # Application entry point
│   ├── MainWindow.h       # Main window header
│   ├── MainWindow.cpp     # Main window implementation
│   ├── SerialHandler.h    # Serial port management header
│   ├── SerialHandler.cpp  # Serial port management implementation
│   ├── MorseDecoder.h     # Morse decoding header
│   ├── MorseDecoder.cpp   # Morse decoding implementation
│   ├── MorseTable.h       # Morse code definitions header
│   └── MorseTable.cpp     # Morse code definitions implementation
├── resources/
│   ├── resources.qrc      # Qt resource file
│   └── icons/             # Application icons
└── tests/
    ├── CMakeLists.txt
    └── test_decoder.cpp
```

## Implementation Phases

### Phase 1: Core Foundation
- [ ] Project structure setup with CMake
- [ ] Morse code table implementation
- [ ] Basic decoder logic with timing analysis

### Phase 2: Serial Communication
- [ ] Serial port detection and listing
- [ ] Connection management
- [ ] Signal-based serial reading

### Phase 3: GUI Development
- [ ] Main window layout
- [ ] Serial configuration panel
- [ ] Text display area
- [ ] Visual feedback (dit/dah indicator)

### Phase 4: Integration & Polish
- [ ] Connect all components
- [ ] Settings persistence (QSettings)
- [ ] Error handling and user feedback
- [ ] Application packaging

## Dependencies

### System Packages (Linux Mint)
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-serialport-dev qt6-multimedia-dev
```

## Installation (Linux Mint)

```bash
# Install system dependencies
sudo apt install build-essential cmake qt6-base-dev qt6-serialport-dev qt6-multimedia-dev

# Clone and build
git clone <repository>
cd Morse-key-
mkdir build && cd build
cmake ..
make -j$(nproc)

# Add user to dialout group for serial access
sudo usermod -a -G dialout $USER
# (Logout/login required)

# Run application
./morse-decoder
```

## Configuration Options

| Setting | Default | Description |
|---------|---------|-------------|
| `serial_port` | Auto | Serial port path |
| `baud_rate` | 9600 | Serial baud rate |
| `wpm` | 20 | Words per minute |
| `dit_dah_ratio` | 3.0 | Dah/dit duration ratio |
| `sidetone_enabled` | false | Audio feedback |
| `sidetone_freq` | 600 | Sidetone frequency (Hz) |

## Error Handling

- Serial port access denied → Prompt user about dialout group
- Device disconnected → Auto-reconnect with notification
- Decoding ambiguity → Display best guess with indicator
- Invalid timing → Adaptive algorithm self-corrects

## Future Enhancements

- [ ] Transmit mode (text-to-Morse)
- [ ] Practice mode with random characters
- [ ] QSO logging
- [ ] Contest mode
- [ ] Custom Morse code tables
- [ ] Multiple device support
- [ ] Network streaming (for remote operation)

## References

- [International Morse Code](https://en.wikipedia.org/wiki/Morse_code)
- [ARRL Morse Code Timing](http://www.arrl.org/learning-morse-code)
- [Qt6 Serial Port](https://doc.qt.io/qt-6/qtserialport-index.html)
- [Qt6 Documentation](https://doc.qt.io/qt-6/)

## Notes

The CW Morse "My Key Serial" adapter documentation should be consulted for exact serial protocol details. This design supports multiple common protocols and can be adapted based on actual device behavior.

---

*Document Version: 1.1 - C++ Implementation*
*Last Updated: 2025-11-19*
