# libCli

**A lightweight and versatile command-line interface for Arduino-based projects.**

## Changelog

### Version 4.3.0 (2026-06-15)
- Redesigned the history management to be more robust, efficient and capable.
  - No multiple commands can be stored in a fixed size ring buffer.
  - Number of comands to be stored depends on their size and the buffer size.
  - The oldest commands get overwritten if needed to store a new command.
  - Can navigate back and forth through the history.
  - Stores the commands as they are to respect potential escaped characters.
  - Stores also invalid commands to be able to recall nd fix them.
  - Does not recognise repeated commands, it just stores what you invoke. So if you recall a command and invoke it again, it will also be stored once again.

### Version 4.2.0 (2026-03-28)
- Fixed potential out of bounds write in the command buffer.
- Fixed unahandled missing string terminator in command arguments.
- Refactored comments and improved documentation.

### Version 4.1.0 (2026-01-29)
- Added support for escaped characters in strings passed as command arguments.

### Changes Version 4.0.0 (2025-02-07)
- New implementation of automatic command registration.
- API Clean up.
- platformio registration.

### Older versions
- See commit logs.

## Features
- Designed for microcontrollers, prioritizing efficiency and essential functionality
- Works with any stream-based transport (Serial, Telnet, etc.)
- Includes some selected VT100 sequences for improved terminal usability
- Automatic command registration via `CLI_COMMAND(name)` macro
- Minimalistic history support to repeat the last command
- Configurable command separator (default: space)
- Configurable prompt (default: #>)
- Simple integration with `cli.begin()` and `cli.loop()`

## Installation

### Using PlatformIO
Add the library to your `platformio.ini`:
```ini
lib_deps = https://github.com/fjulian79/libcli.git
```

### Manual Installation
Clone the repository and place it in your `libraries` folder:
```sh
git clone https://github.com/fjulian79/libcli.git
```

## Usage

### Basic Example
```cpp
#include <Arduino.h>
#include <cli/cli.hpp>

Cli cli;

CLI_COMMAND(ver) {
    ioStream.printf("libcli_example v1.0.0\n");
    return 0;
}

void setup() {
    Serial.begin(115200);
    cli.begin();
}

void loop() {
    cli.loop();
}
```

## Supported vt100 Sequences
 - Backspace, a standard behavior in terminal environments. See https://vt100.net/docs/vt510-rm/chapter4.html for details.
 - `CTRL + L` to delete the sceen
 - `CTRL + K` to delte the line
 - Ring the bell of the users terminal (see example)

## Configuration
Custom settings can be set within yor project by addind a header file called `cli_config.hpp` or in `platformio.ini`
For a list of configurable options along with their documentation see `cli/config.hpp`
If you use the header file you have to add the include path in `platformio.ini`.
E.g. when you have `./cfg/cli_config.hpp`
```ini
build_flags = -Icfg
```

## Examples
For a basic example see examples 
For full example projects, visit [clidemo](https://github.com/fjulian79/clidemo).

## License
This library is licensed under GPL-3.0.

## Author
Developed by **Julian Friedrich**. For contributions or issues, visit the [GitHub repository](https://github.com/fjulian79/libcli).
