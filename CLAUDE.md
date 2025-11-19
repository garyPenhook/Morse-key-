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
| Language | Python 3.x | Cross-platform, excellent serial/GUI libraries |
| GUI Framework | PyQt6 | Modern, feature-rich, good Linux Mint integration |
| Serial Communication | pyserial | Industry standard for serial port access |
| Audio Feedback | pygame or simpleaudio | Optional sidetone generation |

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

### Core Modules

#### 1. `serial_handler.py` - Serial Communication
- Auto-detect available serial ports
- Connect/disconnect management
- Read serial data in separate thread
- Support multiple protocols:
  - **Character mode**: Device sends ASCII characters
  - **Timing mode**: Device sends key-down/key-up events with timestamps
  - **DTR/RTS mode**: Uses control lines for key state

#### 2. `morse_decoder.py` - Morse Code Decoding
- Real-time timing analysis
- Dit/dah discrimination based on element duration
- Character/word boundary detection
- Support for:
  - International Morse Code
  - Prosigns (AR, SK, BT, etc.)
  - Adjustable WPM (5-50 WPM)

#### 3. `main_window.py` - GUI Application
- Serial port configuration panel
- Real-time decoded text display
- Visual dit/dah indicator
- Settings persistence
- Clear/copy/save functionality

#### 4. `morse_table.py` - Morse Code Lookup
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

```python
# Distinguish dit from dah
if element_duration < (dit_avg + dah_avg) / 2:
    element = 'dit'
else:
    element = 'dah'

# Update running averages
if element == 'dit':
    dit_avg = 0.8 * dit_avg + 0.2 * element_duration
else:
    dah_avg = 0.8 * dah_avg + 0.2 * element_duration
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
├── requirements.txt       # Python dependencies
├── setup.py              # Installation script
├── src/
│   ├── __init__.py
│   ├── main.py           # Application entry point
│   ├── main_window.py    # Main GUI window
│   ├── serial_handler.py # Serial port management
│   ├── morse_decoder.py  # Morse decoding logic
│   ├── morse_table.py    # Morse code definitions
│   ├── settings.py       # Configuration management
│   └── resources/
│       ├── icon.png      # Application icon
│       └── sounds/       # Sidetone audio files
├── tests/
│   ├── test_decoder.py
│   └── test_morse_table.py
└── docs/
    └── screenshots/
```

## Implementation Phases

### Phase 1: Core Foundation
- [ ] Project structure setup
- [ ] Morse code table implementation
- [ ] Basic decoder logic with timing analysis
- [ ] Unit tests for decoder

### Phase 2: Serial Communication
- [ ] Serial port detection and listing
- [ ] Connection management
- [ ] Threaded serial reading
- [ ] Protocol auto-detection

### Phase 3: GUI Development
- [ ] Main window layout
- [ ] Serial configuration panel
- [ ] Text display area
- [ ] Visual feedback (dit/dah indicator)
- [ ] Settings dialog

### Phase 4: Integration & Polish
- [ ] Connect all components
- [ ] Settings persistence (QSettings)
- [ ] Error handling and user feedback
- [ ] Sidetone audio (optional)
- [ ] Application packaging

### Phase 5: Testing & Documentation
- [ ] Real device testing
- [ ] Edge case handling
- [ ] User documentation
- [ ] Installation instructions

## Dependencies

```txt
PyQt6>=6.4.0
pyserial>=3.5
```

## Installation (Linux Mint)

```bash
# Install system dependencies
sudo apt install python3-pip python3-venv

# Clone and setup
git clone <repository>
cd Morse-key-
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Add user to dialout group for serial access
sudo usermod -a -G dialout $USER
# (Logout/login required)

# Run application
python src/main.py
```

## Configuration Options

| Setting | Default | Description |
|---------|---------|-------------|
| `serial_port` | Auto | Serial port path |
| `baud_rate` | 9600 | Serial baud rate |
| `wpm` | 20 | Words per minute |
| `dit_dah_ratio` | 3.0 | Dah/dit duration ratio |
| `sidetone_enabled` | False | Audio feedback |
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
- [PySerial Documentation](https://pyserial.readthedocs.io/)
- [PyQt6 Documentation](https://www.riverbankcomputing.com/static/Docs/PyQt6/)

## Notes

The CW Morse "My Key Serial" adapter documentation should be consulted for exact serial protocol details. This design supports multiple common protocols and can be adapted based on actual device behavior.

---

*Document Version: 1.0*
*Last Updated: 2025-11-19*
