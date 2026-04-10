# Command Registration in libCli

This document explains how libCli implements automatic command registration and why this particular design was chosen.

## The Challenge

When building a command-line interface library for embedded systems, you need a way to collect all available commands into a central table. This table is used by the CLI to match user input against registered commands and execute the appropriate handler.

### Traditional Approaches and Their Problems

**1. Manual Command Tables**
```cpp
// User must manually maintain this table
const cliCmd_t commands[] = {
    {"help", cmd_help},
    {"status", cmd_status},
    // ... must add every command here
};
```

**Problems:**
- Error-prone: Easy to forget to add a command
- Maintenance burden: Every new command requires updates in multiple places
- Typos: Command name string and function name can drift apart

**2. Script-Based Generation**

Another option is to use scripts at build time to scan source files for commands and generate the command table.

**Problems:**
- **Preprocessor blindness**: Scripts see all code, even disabled `#if` blocks
- Commands inside `#ifdef FEATURE_X` are found even when `FEATURE_X` is not defined
- Requires complex parsing to handle C preprocessor correctly
- Adds build system dependencies
- Fragile when dealing with complex macro expansions

**3. Linker Section Tricks**

It may be possible to use compiler-specific attributes to place command definitions in special linker sections.

```cpp
__attribute__((section(".commands"))) const cliCmd_t cmd_help = {...};
```

**Problems:**
- **Not portable**: Different compilers and platforms have different linker behaviors
- STM32, ESP32, ESP8266, RP2040 all have different toolchains
- May not work reliably across all Arduino platforms
- Requires linker script modifications on some platforms wich is a issue when using build systems like PlatformIO that do not expose linker script configuration to the user in a eays way (or at all).
- Complex to debug when it fails
- May break with optimizations or certain other compiler flags

## The libCli Solution: Constructor-Based Registration

libCli uses **static initialization** via C++ constructors to automatically register commands at program startup. This approach is:

- ✅ **Preprocessor-aware**: Only compiled code gets registered
- ✅ **Cross-platform**: Works on all C++ compilers that support Arduino
- ✅ **Zero runtime overhead**: Registration happens before `main()`
- ✅ **No build tools required**: Pure C++ without external dependencies
- ✅ **Type-safe**: Compiler checks everything at compile time

## How It Works

### The CLI_COMMAND Macro

When you write:

```cpp
CLI_COMMAND(help) {
    ioStream.println("Available commands...");
    return 0;
}
```

The macro expands to:

```cpp
// 1. Forward declaration
int8_t cmd_help(Stream& ioStream, const char *argv[], uint8_t argc);

// 2. Static registrar object (this is the magic!)
static CliCommand help_registrar("help", cmd_help);

// 3. Actual function definition
int8_t cmd_help(Stream& ioStream, const char *argv[], uint8_t argc) {
    ioStream.println("Available commands...");
    return 0;
}
```

### The Magic: Static Initialization

The key is line 2: the static `CliCommand` object.

```cpp
static CliCommand help_registrar("help", cmd_help);
```

In C++, **static objects are constructed before `main()` is called**. This means:

1. When your program starts (before `setup()` runs), the C++ runtime calls constructors for all static objects
2. The `CliCommand` constructor is called with `"help"` and a pointer to `cmd_help`
3. The constructor adds this command to the global command table
4. By the time `setup()` runs, all commands are already registered

This is called the **Static Initialization Order** pattern.

### Token Pasting and Stringification

The macro uses two important preprocessor operators:

**`##` (Token Pasting Operator):**
```cpp
_name ## _registrar  →  help_registrar  (when _name = help)
cmd_ ## _name        →  cmd_help
```

**`#` (Stringification Operator):**
```cpp
#_name  →  "help"  (converts token to string literal)
```

### Why CLI_COMMAND_DEF Appears Twice

Looking at the macro definition:

```cpp
#define CLI_COMMAND(_name)                                          \
    CLI_COMMAND_DEF(_name);                                         \
    static CliCommand _name ## _registrar(#_name, cmd_ ## _name);   \
    CLI_COMMAND_DEF(_name)
```

The first `CLI_COMMAND_DEF(_name)` creates a **forward declaration** so that the static registrar on the next line can reference the function. The second `CLI_COMMAND_DEF(_name)` begins the **actual function definition** that you complete with the function body.

Expanding `CLI_COMMAND_DEF`:

```cpp
#define CLI_COMMAND_DEF(_name) \
    int8_t cmd_ ## _name (Stream& ioStream, const char *argv[], uint8_t argc)
```

So the full expansion is:
```cpp
// Forward declaration (ends with semicolon)
int8_t cmd_help(Stream& ioStream, const char *argv[], uint8_t argc);

// Static registrar
static CliCommand help_registrar("help", cmd_help);

// Function definition (you add the body)
int8_t cmd_help(Stream& ioStream, const char *argv[], uint8_t argc)
{
    // Your code here
}
```

## Preprocessor Awareness

This design is **preprocessor-aware**, which solves a major problem:

```cpp
#ifdef FEATURE_TELNET

CLI_COMMAND(telnet) {
    // Telnet-specific code
    return 0;
}

#endif
```

**With libCli:**
- If `FEATURE_TELNET` is defined → command compiles → static object created → registered
- If `FEATURE_TELNET` is NOT defined → entire block skipped → no registration

**With Python script approach:**
- The script sees `CLI_COMMAND(telnet)` even when `FEATURE_TELNET` is undefined
- Would try to register a command that doesn't exist
- Requires complex preprocessing to handle correctly

## The CliCommand Class

The registration is handled by the `CliCommand` class:

```cpp
class CliCommand {
public:
    CliCommand(const char* name, CmdFuncPtr function) {
        if (CmdCnt < CLI_COMMANDS_MAX) {
            CmdTab[CmdCnt++] = {name, function};
        } else {
            DropCnt++;
        }
    }

private:
    static cliCmd_t CmdTab[CLI_COMMANDS_MAX];
    static size_t CmdCnt;
    static size_t DropCnt;
};
```

The constructor simply adds the command to the static command table. The table is allocated at compile time, maintaining libCli's **zero dynamic allocation** principle.

### Memory Overhead Per Command

You might wonder: "Does each command create a separate `CliCommand` instance that stays in memory?"

The answer is subtle:

**Yes**, technically each command creates one static registrar object (e.g., `help_registrar`), **but** the memory overhead is minimal because:

1. The `CliCommand` class has **only static members** (`CmdTab`, `CmdCnt`, `DropCnt`)
2. Static members exist **once per class**, not once per instance
3. Each registrar object is essentially empty - it has no instance data
4. The C++ standard requires objects to have non-zero size, so each registrar typically occupies **1 byte**

**Memory breakdown:**
```
Global (shared by all commands):
  - CmdTab array:     CLI_COMMANDS_MAX * sizeof(cliCmd_t)  (~10 * 8 = 80 bytes)
  - CmdCnt counter:   sizeof(size_t)                       (~4 bytes)
  - DropCnt counter:  sizeof(size_t)                       (~4 bytes)

Per command:
  - Registrar object: ~1 byte (empty object, minimal footprint)
```

So if you have 10 commands, the overhead is approximately:
- **88 bytes total** (command table + counters)
- **~10 bytes** (10 empty registrar objects)
- **Total: ~98 bytes** for the registration system

The actual command data (name string and function pointer) lives in the command table, which would exist regardless of the registration mechanism used.

## Platform Compatibility

This approach works reliably across all Arduino platforms:

- **STM32** (various toolchains: Arduino_STM32, STM32duino)
- **ESP32** (ESP-IDF based)
- **ESP8266** (GCC for Xtensa)
- **RP2040** (ARM GCC)
- **AVR** (avr-gcc)
- **SAMD** (ARM GCC)
- Any other Arduino-compatible platform with C++ support

All these platforms support C++ static initialization, making the solution universally portable.

## Why This Design Is Elegant

1. **No manual table maintenance**: Just use `CLI_COMMAND(name)` and it's automatically available
2. **Impossible to have mismatched names**: The function name and command name are generated from the same token
3. **Works everywhere**: Pure C++, no platform-specific tricks
4. **Preprocessor-aware**: Conditionally compiled code is automatically handled
5. **Zero runtime cost**: Registration happens at program initialization, not during execution
6. **Minimal memory overhead**: Only static members exist; registrar objects are essentially empty (~1 byte each)
7. **Type-safe**: The compiler verifies function signatures at compile time
8. **No external tools**: No Python scripts, no custom linker scripts, no build system magic
9. **Easy to debug**: Everything is standard C++, visible to debuggers and code analyzers
10. **Modular**: Each command can live in its own file without central coordination

## Trade-offs

**Limitations:**
- ❌ Commands are registered in unpredictable order (static initialization order is undefined across translation units)
  - **Solution**: libCli doesn't rely on command order; lookup is by name
- ❌ Limited to compile-time known commands (can't dynamically add commands at runtime)
  - **Rationale**: This aligns with libCli's embedded/deterministic design philosophy

## Example: Complete Flow

**Your code:**
```cpp
CLI_COMMAND(led) {
    if (argc < 2) return -1;
    digitalWrite(LED_BUILTIN, atoi(argv[1]));
    return 0;
}
```

**After preprocessor expansion:**
```cpp
int8_t cmd_led(Stream& ioStream, const char *argv[], uint8_t argc);
static CliCommand led_registrar("led", cmd_led);
int8_t cmd_led(Stream& ioStream, const char *argv[], uint8_t argc) {
    if (argc < 2) return -1;
    digitalWrite(LED_BUILTIN, atoi(argv[1]));
    return 0;
}
```

**At program startup (before setup() runs):**
```cpp
// C++ runtime calls:
led_registrar.CliCommand("led", cmd_led)
  → Adds {"led", cmd_led} to global command table
```

**When user types "led 1":**
```cpp
cli.loop()
  → Parses "led 1"
  → Looks up "led" in command table
  → Finds cmd_led function pointer
  → Calls cmd_led(ioStream, {"led", "1"}, 2)
```

## Further Reading

This pattern is sometimes called:
- **Static Registration Pattern**
- **Self-Registering Classes**
- **Static Initialization for Discovery**

It's widely used in:
- Plugin systems
- Factory pattern implementations
- Test framework command registration (Google Test, Catch2)
- Embedded command dispatchers
