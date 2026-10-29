# libCli

**A feature-rich yet lightweight and resource-conscious CLI implementation specifically designed for bare-metal embedded systems based on Arduino.**

## Features

### Core Functionality
- **Automatic Command Registration** - Define commands with `CLI_COMMAND(name)` macro, no manual table management
  - Preprocessor-aware: only compiled code gets registered
  - Cross-platform compatible: works on all Arduino platforms
  - See [Command Registration](doc/command-registration.md) for how it works
- **Stream-Based I/O** - Works with any Arduino Stream (Serial, Telnet, WebSocket, etc.)
- **Line Endings Handled** - CR, CR LF and CR NUL all work as Enter, also for commands reading input themselves, see [Line Endings](doc/line-endings.md)
- **Built-in Telnet Support** - Character mode, cursor keys, history and tab completion via telnet, just pass the client
- **Low Memory Footprint** - Configurable buffer sizes, ~400 bytes RAM with defaults
- **Simple Integration** - Just `cli.begin()` and `cli.loop()` in your sketch

### User Interface
- **Command History** - Navigate through multiple previous commands using arrow keys
  - Ring buffer stores multiple commands based on size
  - Preserves escaped characters
  - Stores invalid commands for easy correction
  - Automatic duplicate filtering (consecutive identical commands)
  - Optional feature: Can be disabled to save RAM and flash memory
- **Tab Completion** - Bash-like command completion
  - Press Tab to auto-complete commands
  - Single match: Completes and adds space for argument input
  - Line wrapping respects terminal width
  - VT100 cursor control for seamless editing
  - Optional feature: Can be disabled to save flash memory
- **VT100 Terminal Support** - Standard terminal sequences for better usability
  - Arrow Up/Down: Navigate command history
  - Tab: Auto-complete commands
  - Backspace/DEL: Edit commands
  - Ctrl+L: Clear screen
  - Ctrl+K: Clear line
  - Bell signal support
- **Configurable Prompt** - Customize the command prompt (default: `#>`)

### Argument Parsing
- **Flexible Arguments** - Up to 4 arguments by default (configurable)
- **Quoted Strings** - Support for arguments with spaces: `cmd "arg with spaces"`
- **Escape Sequences** - Within quoted strings `\"` and `\\` stand for a quote and a backslash: `cmd "say \"hi\""`
- **Leading Whitespace** - Ignored, so pasted indented commands just work

### Configuration
- **Header-Based Config** - Override defaults via `cli_config.hpp`
- **Build Flag Config** - Or configure directly in `platformio.ini`
- **Fully Documented** - See [configuration.md](doc/configuration.md) for all options

## Design Principles

Modern dual-core MCUs like the RP2040 or ESP32 running at 133MHz+ enable more sophisticated CLI features than was possible when this project started. libCli embraces these capabilities to provide better usability while maintaining its focus on efficiency and lightweight operation. There must be a clear difference between a microcontroller CLI and embedded Linux running on a Raspberry Pi - **libCli shall not become bash!**

At this point you may ask: And why does the library then provide features like command history and tab completion? The answer is simple: Because we can with reasonable effort! With careful design and optimization, these features can be implemented in a way that is both efficient and user-friendly, without compromising the core principles of the library. The goal is to provide a powerful yet lightweight CLI solution that enhances the user experience while respecting the constraints of embedded systems. What's not planned is add things like argument auto-completion, scripting capabilities, or a full-fledged shell environment. 

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

libCli uses **constructor-based command registration** instead of linker tricks or build scripts to ensure reliable operation across all Arduino platforms. This approach is preprocessor-aware, meaning commands inside `#ifdef` blocks are only registered when actually compiled.

libCli requires an Arduino core whose `Print` class provides `printf()`, which is the case for ESP32, ESP8266, RP2040 (Earle Philhower) and STM32 (STM32duino). The AVR and SAMD cores lack it.

Unlike Python script-based solutions that scan source code, libCli's registration respects the preprocessor. Disabled code is never registered, and the system works identically across different toolchains without requiring linker script modifications.

For a detailed explanation of why this design was chosen and how it works, see [Command Registration](doc/command-registration.md).

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

- **[API Reference](doc/api.md)** - Complete API documentation for all classes and methods
- **[Configuration Guide](doc/configuration.md)** - Detailed configuration options and memory considerations
- **[Command Registration](doc/command-registration.md)** - How automatic command registration works and why
- **[Line Endings](doc/line-endings.md)** - CR, LF, terminals, Linux and Telnet, and which settings work
- **[Changelog](CHANGELOG.md)** - Version history and release notes

## Quick Start

### Installation

#### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps = fjulian79/libCli
```

To stay on a major version, append it as in `fjulian79/libCli@^4.7.0`, see the
[releases](https://github.com/fjulian79/libcli/releases) for the latest one.

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
int8_t cmd_<name>(CliStream& ioStream, const char *argv[], uint8_t argc)
```

- **ioStream** - Use for all I/O operations (`printf`, `println`, etc.). It
  is a `CliStream` wrapping the stream the CLI runs on, which is available by
  `ioStream.parent()`. See [api.md](doc/api.md#clistream-class) for details.
- **argv** - Array of arguments, `argv[0]` is the first argument *after* the
  command name, not the command name itself. Unlike Unix `main(argc, argv)`,
  the name is deliberately excluded - it's already known statically via
  `CLI_COMMAND(name)`, so repeating it would waste a `CLI_ARGVSIZ` slot for
  no benefit. See [api.md](doc/api.md#argv) for details.
- **argc** - Number of arguments, excluding the command name
- **return** - 0 for success, negative for errors

See [api.md](doc/api.md) for complete details.

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
#define CLI_TAB_COMPLETION  1       // Enable tab completion (default)
#define CLI_TERMINAL_WIDTH  80      // Terminal width for wrapping
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
| `CLI_TAB_COMPLETION` | 1 | Enable tab completion (0=off) |
| `CLI_TERMINAL_WIDTH` | 80 | Terminal width for wrapping |

For complete configuration documentation, see [configuration.md](doc/configuration.md).

## Examples

| Example | Shows |
|---|---|
| [basic](examples/basic/basic.ino) | Defining commands and integrating libCli into a sketch |
| [arguments](examples/arguments/arguments.ino) | Quoted strings, parsing numbers, validating arguments and returning errors |
| [blink](examples/blink/blink.ino) | Controlling hardware while `loop()` keeps doing other work, without `delay()` |
| [exec](examples/exec/exec.ino) | Executing commands from code, e.g. at startup or for commands received via MQTT |
| [telnet](examples/telnet/telnet.ino) | Using the CLI via WiFi and telnet (ESP32, ESP8266, Pico W) |

## Advanced Usage

### Using Different Streams

```cpp
// A telnet client accepted by a WiFiServer, libCli handles the protocol
cli.setStream(&client, CliStream::telnet);

// Switch back at runtime, e.g. when the client disconnects
cli.setStream(&Serial);
```

### Disabling Echo

Suppresses the per-character echo of user input, useful when a hostapplication drives the CLI programmatically and handles its own echo. Note this only affects the echo of typed characters - prompts, the bell signal, error messages and the tab-completion match list are always written regardless of this setting. It is therefore **not** suitable for masking password input, since e.g. history navigation or Ctrl+L still reprint the current buffer unconditionally.

```cpp
cli.setEcho(false);  // No character echo
// Process commands
cli.setEcho(true);   // Re-enable
```

### Programmatic Command Execution

The command name is looked up separately, so it is not part of `argv`, just
like an interactively typed command never sees its own name in `argv[0]`:
```cpp
const char* args[] = {"verbose"};
int8_t result = CliCommand::exec(Serial, "status", args, 1);
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

For more examples and detailed API usage, see [api.md](doc/api.md).

## Tests

Unit tests can be run on the host, using AddressSanitizer and
UndefinedBehaviorSanitizer:

```
pio test
```

This runs the tests with the default configuration and once with the history
and once with the tab completion disabled, a single configuration can be
selected with e.g. `pio test -e native`. `Stream` and `Serial` are provided by
a minimal Arduino replacement in `test/support`.

## Contributing

Contributions are welcome! Please feel free to:
- Report issues on [GitHub Issues](https://github.com/fjulian79/libcli/issues)
- Submit pull requests
- Suggest new features
- Improve documentation

## License

This library is licensed under the **MIT License**, so it can be used in any
project, including closed source firmware. Versions up to 4.7.0 have been
licensed under GPL-3.0.

See [LICENSE.txt](LICENSE.txt) for details.

## Author

**Julian Friedrich**

- GitHub: [@fjulian79](https://github.com/fjulian79)
- Project: [github.com/fjulian79/libcli](https://github.com/fjulian79/libcli)
