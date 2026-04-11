# Configuration Reference

libCli can be customized through configuration defines that control buffer sizes, limits, and behavior.

## Configuration Methods

You can configure libCli in two ways:

### 1. Using a Header File (Recommended)

Create a file named `cli_config.hpp` in your project and define your custom values:

```cpp
#pragma once

#define CLI_COMMANDS_MAX    20
#define CLI_COMMANDSIZ      200
#define CLI_HISTORYSIZ      400
#define CLI_ARGVSIZ         8
#define CLI_PROMPT          "$ "
```

Then add the include path in your `platformio.ini`:

```ini
build_flags = -Icfg  # If your cli_config.hpp is in ./cfg/
```

### 2. Using Build Flags

Define the configuration directly in your `platformio.ini`:

```ini
build_flags = 
    -DCLI_COMMANDS_MAX=20
    -DCLI_COMMANDSIZ=200
    -DCLI_PROMPT='"$ "'
```

## Configuration Options

### CLI_COMMANDS_MAX
**Type:** Integer  
**Default:** `10`  
**Description:** Maximum number of commands that can be registered in the global command table.

If you define more commands than this limit, the excess commands will be dropped. Use `CliCommand::getDropCnt()` to check if commands were dropped.

**Example:**
```cpp
#define CLI_COMMANDS_MAX    20  // Support up to 20 commands
```

### CLI_COMMANDSIZ
**Type:** Integer  
**Default:** `100`  
**Description:** Maximum length of a command line including all arguments in bytes.

This defines the size of the input buffer. Commands longer than this will be truncated.

**Example:**
```cpp
#define CLI_COMMANDSIZ      200  // Allow longer commands
```

### CLI_HISTORYSIZ
**Type:** Integer  
**Default:** `CLI_COMMANDSIZ * 2` (200 bytes)  
**Description:** Size of the command history ring buffer in bytes.

The history stores multiple commands in a circular buffer. The number of commands that can be stored depends on their length. Longer commands take up more space, so fewer will fit in the buffer.

**Disabling History:**
Set to `0` to completely disable command history support. This saves memory (measured on RP2040):
- **~224 bytes RAM** (buffer + pointers)
- **~816 bytes Flash** (entire history implementation removed by linker)

Actual savings may vary depending on platform, compiler, and optimization settings.

**Size Validation:**
The history buffer must be large enough to store at least one complete command. Therefore:
- **`CLI_HISTORYSIZ >= CLI_COMMANDSIZ`**: Valid, history is enabled
- **`CLI_HISTORYSIZ == 0`**: Valid, history is intentionally disabled
- **`0 < CLI_HISTORYSIZ < CLI_COMMANDSIZ`**: Invalid - generates compile warning and is forced to 0

Values between 1 and CLI_COMMANDSIZ-1 will trigger a compile warning and be forced to 0 to prevent inconsistent behavior. Inconsistency may occur when short commands can be stored in the history (because they fit) while long commands cannot be stored. This leads to the decision that the history buffer shall offer at least enough space to store the longest possible command.

**Example:**
```cpp
#define CLI_HISTORYSIZ      500   // Store more history
#define CLI_HISTORYSIZ      0     // Disable history completely
```

### CLI_ARGVSIZ
**Type:** Integer  
**Default:** `4`  
**Description:** Maximum number of arguments that can be passed to a command.

This includes the command name itself (argv[0]). So with the default of 4, you can have the command plus 3 arguments.

**Example:**
```cpp
#define CLI_ARGVSIZ         8  // Support up to 7 arguments
```

### CLI_PROMPT
**Type:** String  
**Default:** `"#>"`  
**Description:** Command prompt string displayed to the user.

**Example:**
```cpp
#define CLI_PROMPT          "$ "
#define CLI_PROMPT          "MyDevice> "
```

### CLI_BUFFEREDIO
**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable if stdio is buffered and requires `fflush(stdout)`.

This is rarely needed in microcontroller environments but may be useful in certain platform configurations.

**Example:**
```cpp
#define CLI_BUFFEREDIO      1  // Enable fflush() calls
```

### CLI_TAB_COMPLETION
**Type:** Integer (0 or 1)  
**Default:** `1`  
**Description:** Enable or disable bash-like tab completion for commands.

When enabled (default), pressing Tab will:
- Auto-complete if there's exactly one matching command
- Complete to the longest common prefix if there are multiple matches
- Display all matching commands below the current line
- Use intelligent line wrapping based on `CLI_TERMINAL_WIDTH`

When disabled, pressing Tab will just trigger a bell sound.

**Memory Savings:**
Disabling tab completion saves approximately **~712 bytes of flash memory** (measured on RP2040) by removing the entire completion implementation. RAM savings are negligible (state tracking variables are optimized away by the compiler when disabled).

**Example:**
```cpp
#define CLI_TAB_COMPLETION  0     // Disable for resource-constrained systems
#define CLI_TAB_COMPLETION  1     // Enable (default)
```

### CLI_TERMINAL_WIDTH
**Type:** Integer  
**Default:** `80`  
**Description:** Assumed terminal width in characters for intelligent line wrapping.

This setting is used by tab completion to wrap the list of matching commands across multiple lines. Commands are arranged to fit within this width, preventing line breaks at awkward positions.

Common values:
- `80` - Standard terminal width (default)
- `120` - Wide terminal
- `40` - Narrow embedded display

**Note:** This setting has no effect if `CLI_TAB_COMPLETION` is disabled.

**Example:**
```cpp
#define CLI_TERMINAL_WIDTH  120   // Wide terminal
#define CLI_TERMINAL_WIDTH  40    // Narrow display
```

## Memory Considerations

### Calculating Memory Usage

Total static memory used by libCli:

```
Command Table:     CLI_COMMANDS_MAX * sizeof(cliCmd_t)
Command Buffer:    CLI_COMMANDSIZ
History Buffer:    CLI_HISTORYSIZ (if enabled)
Argument Array:    CLI_ARGVSIZ * sizeof(char*)
Tab Completion:    2 bytes state (if enabled, negligible in practice)
```

### Example Calculation

With default settings (approximate):
- Command table: 10 × 8 bytes = 80 bytes
- Command buffer: 100 bytes
- History buffer: 200 bytes
- History pointers/state: 4 × 4 + 1 = 17 bytes
- Argument array: 4 × 4 bytes = 16 bytes
- String flags: 4 bytes
- Tab completion state: 2 bytes (optimized away when disabled)
- Misc (pointers, counters, state): ~12 bytes
- **Total: ~431 bytes** (plus compiler padding/alignment)

With history disabled (approximate):
- Command table: 10 × 8 bytes = 80 bytes
- Command buffer: 100 bytes
- Argument array: 4 × 4 bytes = 16 bytes
- String flags: 4 bytes
- Misc (pointers, counters, state): ~12 bytes
- **Total: ~212 bytes** (plus compiler padding/alignment)

**Measured on RP2040:** Disabling history saves ~224 bytes RAM + ~816 bytes Flash.  
(Actual values depend on platform, compiler, optimization, and struct alignment.)

### Optimization Tips

**For minimal memory footprint:**
```cpp
#define CLI_COMMANDS_MAX    5
#define CLI_COMMANDSIZ      50
#define CLI_HISTORYSIZ      0    // Disable history
#define CLI_ARGVSIZ         3
#define CLI_TAB_COMPLETION  0    // Disable tab completion
```

**For feature-rich applications:**
```cpp
#define CLI_COMMANDS_MAX    30
#define CLI_COMMANDSIZ      256
#define CLI_HISTORYSIZ      1024
#define CLI_ARGVSIZ         10
#define CLI_TAB_COMPLETION  1    // Enable tab completion (default)
#define CLI_TERMINAL_WIDTH  120  // Wide terminal
```

## Default Configuration

The default configuration is defined in `cli/config.hpp`. You can view this file to see all available options and their defaults. Any values you define in your `cli_config.hpp` will override the defaults.
