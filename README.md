# libCli

**A feature-rich yet lightweight and resource-conscious CLI implementation specifically designed for bare-metal embedded systems based on Arduino.**

## Features

### Core Functionality
- **Automatic Command Registration** - Define commands with `CLI_COMMAND(name)` macro, no manual table management
  - Preprocessor-aware: only compiled code gets registered
  - Cross-platform compatible: works on all Arduino platforms
  - See [Command Registration](doc/COMMAND_REGISTRATION.md) for how it works
- **Stream-Based I/O** - Works with any Arduino Stream (Serial, Telnet, WebSocket, etc.)
- **Low Memory Footprint** - Configurable buffer sizes, ~400 bytes RAM with defaults
- **Simple Integration** - Just `cli.begin()` and `cli.loop()` in your sketch

### User Interface
- **Command History** - Navigate through multiple previous commands using arrow keys
  - Ring buffer stores multiple commands based on size
  - Preserves escaped characters
  - Stores invalid commands for easy correction
  - Automatic duplicate filtering (consecutive identical commands)
- **VT100 Terminal Support** - Standard terminal sequences for better usability
  - Arrow Up/Down: Navigate command history
  - Backspace/DEL: Edit commands
  - Ctrl+L: Clear screen
  - Ctrl+K: Clear line
  - Bell signal support
- **Configurable Prompt** - Customize the command prompt (default: `#>`)

### Argument Parsing
- **Flexible Arguments** - Up to 4 arguments by default (configurable)
- **Quoted Strings** - Support for arguments with spaces: `cmd "arg with spaces"`
- **Escape Sequences** - Handle special characters: `\"`, `\\`, `\n`, `\t`, `\r`
- **Configurable Separator** - Space by default, but can be changed

### Configuration
- **Header-Based Config** - Override defaults via `cli_config.hpp`
- **Build Flag Config** - Or configure directly in `platformio.ini`
- **Fully Documented** - See [CONFIGURATION.md](doc/CONFIGURATION.md) for all options

## Design Principles

Modern dual-core MCUs like the RP2040 or ESP32 running at 133MHz+ enable more sophisticated CLI features than was possible when this project started. libCli embraces these capabilities to provide better usability while maintaining its focus on efficiency and lightweight operation. There must be a clear difference between a microcontroller CLI and embedded Linux running on a Raspberry Pi - **libCli shall not become bash!**

The library is built on these core principles:

### Zero Dynamic Memory Allocation

Uses **exclusively static memory allocation** - no `malloc()`, `free()`, or dynamic containers under the hood.

- **Deterministic Memory Usage** - Know exact RAM consumption at compile time
- **No Heap Fragmentation** - Essential for long-running embedded systems that may run for months or years
- **Predictable Behavior** - No risk of malloc failures or memory leaks
- **Real-Time Safe** - Constant-time operations, no unpredictable heap management overhead
- **Easy Debugging** - Static analysis tools can verify memory usage

All buffers (command buffer, history buffer, command table) are sized at compile time through configuration defines.

```cpp
// Memory usage is completely predictable:
// - Command table:  CLI_COMMANDS_MAX * sizeof(cliCmd_t)
// - Command buffer: CLI_COMMANDSIZ bytes
// - History buffer: CLI_HISTORYSIZ bytes
// - Argument array: CLI_ARGVSIZ * sizeof(char*)
// Total: ~400 bytes with defaults
```

### Efficiency First

- **Small Footprint** - ~400 bytes RAM with default configuration
- **No Bloat** - Only essential features, no unnecessary overhead
- **Optimized for Microcontrollers** - Every byte and CPU cycle counts

### Simplicity and Ease of Use

- **Minimal API** - Just `cli.begin()` and `cli.loop()` to get started
- **Automatic Registration** - `CLI_COMMAND(name)` macro eliminates manual command table management
- **Clear Conventions** - Consistent patterns and predictable behavior

### Cross-Platform Portability

libCli uses **constructor-based command registration** instead of linker tricks or build scripts to ensure reliable operation across all Arduino platforms (STM32, ESP32, ESP8266, RP2040, AVR, SAMD, etc.). This approach is preprocessor-aware, meaning commands inside `#ifdef` blocks are only registered when actually compiled.

Unlike Python script-based solutions that scan source code, libCli's registration respects the preprocessor. Disabled code is never registered, and the system works identically across different toolchains without requiring linker script modifications.

For a detailed explanation of why this design was chosen and how it works, see [Command Registration](doc/COMMAND_REGISTRATION.md).

### Stream-Based Architecture

Works with any Arduino `Stream` implementation - Serial, Telnet, WebSockets, or custom streams. Write once, use with any transport layer.

## Demo Project

For a complete, real-world example of libCli in action, check out **[clidemo](https://github.com/fjulian79/clidemo)**.

This full-featured demo project showcases:
- **Multiple command implementations** - Various command types and patterns
- **Telnet integration** - Remote CLI access over network
- **Custom transport streams** - Beyond simple Serial usage
- **Production patterns** - Best practices for real-world applications
- **Unit tests** - Testing strategies for CLI commands

The demo runs on multiple platforms (see `platformio.ini` in the demo repository for the complete list) and serves as the primary development testbed for libCli itself. This means it's always up-to-date with the latest features and API changes, making it an excellent reference for real-world usage patterns.

## Documentation

- **[API Reference](doc/API.md)** - Complete API documentation for all classes and methods
- **[Configuration Guide](doc/CONFIGURATION.md)** - Detailed configuration options and memory considerations
- **[Command Registration](doc/COMMAND_REGISTRATION.md)** - How automatic command registration works and why
- **[Changelog](CHANGELOG.md)** - Version history and release notes

## Quick Start

### Installation

#### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps = fjulian79/libCli@^4.4.0
```

#### Arduino IDE
Clone the repository into your Arduino libraries folder:
```sh
cd ~/Arduino/libraries
git clone https://github.com/fjulian79/libcli.git
```

### Basic Example

```cpp
#include <Arduino.h>
#include <cli/cli.hpp>

Cli cli;

// Define a command - automatically registered!
CLI_COMMAND(ver) {
    ioStream.printf("MyProject v1.0.0\n");
    ioStream.printf("Build: %s %s\n", __DATE__, __TIME__);
    return 0;  // Return 0 for success
}

void setup() {
    Serial.begin(115200);
    while (!Serial);  // Wait for Serial on boards that need it
    
    cli.begin();  // Initialize CLI (uses Serial by default)
    
    Serial.println("Ready! Type 'help' for commands.");
}

void loop() {
    cli.loop();  // Process CLI input
}
```

### Command Signature

All commands follow this signature:
```cpp
int8_t cmd_<name>(Stream& ioStream, const char *argv[], uint8_t argc)
```

- **ioStream** - Use for all I/O operations (`printf`, `println`, etc.)
- **argv** - Array of arguments (argv[0] is command name)
- **argc** - Number of arguments including command name
- **return** - 0 for success, negative for errors

See [API.md](doc/API.md) for complete details.

## Configuration

libCli can be customized to fit your needs. All configuration options are optional.

### Quick Config Example

Create `cli_config.hpp` in your project:

```cpp
#pragma once

#define CLI_COMMANDS_MAX    20      // Support more commands
#define CLI_COMMANDSIZ      200     // Longer command lines
#define CLI_HISTORYSIZ      500     // More history
#define CLI_ARGVSIZ         8       // More arguments
#define CLI_PROMPT          "$ "    // Custom prompt
```

Add to `platformio.ini`:
```ini
build_flags = -Icfg  # Path to your cli_config.hpp
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `CLI_COMMANDS_MAX` | 10 | Maximum number of commands |
| `CLI_COMMANDSIZ` | 100 | Max command length (bytes) |
| `CLI_HISTORYSIZ` | 200 | History buffer size (bytes) |
| `CLI_ARGVSIZ` | 4 | Max number of arguments |
| `CLI_PROMPT` | `"#>"` | Command prompt string |

For complete configuration documentation, see [CONFIGURATION.md](doc/CONFIGURATION.md).

## Example

A basic example demonstrating command definition and usage is included in the repository:

```
examples/basic/basic.ino
```

This example shows the fundamentals of defining commands, handling arguments, and integrating libCli into your sketch.

## Advanced Usage

### Using Different Streams

```cpp
// Telnet stream
TelnetStream telnet;
cli.begin(&telnet);

// Switch streams at runtime
cli.setStream(&Serial);
```

### Disabling Echo

Useful for machine-to-machine communication:
```cpp
cli.setEcho(false);  // No terminal echo
// Process commands
cli.setEcho(true);   // Re-enable
```

### Programmatic Command Execution

```cpp
const char* args[] = {"status", "verbose"};
int8_t result = CliCommand::exec(Serial, "status", args, 2);
```

### Checking Registered Commands

```cpp
void setup() {
    cli.begin();
    
    size_t dropped = CliCommand::getDropCnt();
    if (dropped > 0) {
        Serial.printf("WARNING: %d commands not registered!\n", dropped);
        Serial.printf("Increase CLI_COMMANDS_MAX\n");
    }
}
```

For more examples and detailed API usage, see [API.md](doc/API.md).

## Contributing

Contributions are welcome! Please feel free to:
- Report issues on [GitHub Issues](https://github.com/fjulian79/libcli/issues)
- Submit pull requests
- Suggest new features
- Improve documentation

## License

This library is licensed under **GPL-3.0**.

See [LICENSE.txt](LICENSE.txt) for details.

## Author

**Julian Friedrich**

- GitHub: [@fjulian79](https://github.com/fjulian79)
- Project: [github.com/fjulian79/libcli](https://github.com/fjulian79/libcli)
