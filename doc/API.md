# API Reference

Complete API documentation for libCli.

## Table of Contents

- [Command Definition](#command-definition)
- [Cli Class](#cli-class)
- [CliCommand Class](#clicommand-class)
- [Command Function Signature](#command-function-signature)
- [Return Codes](#return-codes)

## Command Definition

### CLI_COMMAND(name)

**Description:** Macro to define and automatically register a command.

**Usage:**
```cpp
CLI_COMMAND(commandname) {
    // Command implementation
    ioStream.printf("Hello from %s\n", argv[0]);
    return 0;
}
```

**Details:**
- Creates a function named `cmd_<name>` with the proper signature
- Automatically registers the command in the global command table
- No manual table management needed
- Registration happens at program startup via static constructors

**Example:**
```cpp
CLI_COMMAND(status) {
    ioStream.printf("System status: OK\n");
    return 0;
}

CLI_COMMAND(echo) {
    for (uint8_t i = 1; i < argc; i++) {
        ioStream.printf("%s ", argv[i]);
    }
    ioStream.println();
    return 0;
}
```

### CLI_COMMAND_DEF(name)

**Description:** Macro to only define the command signature without registration. Useful for forward declarations.

**Usage:**
```cpp
// Forward declaration
CLI_COMMAND_DEF(mycommand);

// Later implementation
CLI_COMMAND(mycommand) {
    // Implementation
    return 0;
}
```

## Cli Class

Main class for managing the command-line interface.

### Constructor

```cpp
Cli();
```

Creates a new Cli instance.

### begin()

```cpp
void begin(Stream *pIoStr = &Serial);
```

Initializes the CLI with the specified I/O stream.

**Parameters:**
- `pIoStr` - Pointer to the Stream object to use (default: &Serial)

**Example:**
```cpp
Cli cli;

void setup() {
    Serial.begin(115200);
    cli.begin();  // Uses Serial by default
}
```

### loop()

```cpp
int8_t loop(void);
```

Main processing function to call in your main loop. Checks for incoming data and processes commands.

**Returns:**
- `0` - No command was recognized
- `INT8_MIN` - Parsing error occurred
- Command return code - The value returned by the executed command

**Example:**
```cpp
void loop() {
    int8_t result = cli.loop();
    if (result < 0) {
        // Handle error
    }
}
```

### read()

```cpp
int8_t read(char byte);
```

Process a single incoming byte. Useful for custom input handling.

**Parameters:**
- `byte` - Character to process

**Returns:** Same as `loop()`

**Example:**
```cpp
if (customStream.available()) {
    char c = customStream.read();
    cli.read(c);
}
```

### setStream()

```cpp
void setStream(Stream *pIoStr);
```

Change the I/O stream during runtime.

**Parameters:**
- `pIoStr` - Pointer to the new Stream object

**Example:**
```cpp
// Switch from Serial to telnet stream
cli.setStream(&telnetStream);
```

### setEcho()

```cpp
void setEcho(bool state);
```

Enable or disable echo mode. When disabled, all terminal echo is suppressed.

**Parameters:**
- `state` - `true` to enable echo, `false` to disable

**Use Cases:**
- Disable for machine-to-machine communication
- Disable during password input
- Disable for applications that handle echo themselves

**Example:**
```cpp
cli.setEcho(false);  // Disable echo for password input
// Get password
cli.setEcho(true);   // Re-enable echo
```

### sendBell()

```cpp
void sendBell(void);
```

Send a VT100 bell signal to the terminal (audible beep or visual flash).

**Example:**
```cpp
CLI_COMMAND(alert) {
    cli.sendBell();
    ioStream.println("Alert!");
    return 0;
}
```

### refreshPrompt()

```cpp
void refreshPrompt(void);
```

Redraw the prompt and current buffer content in the terminal. Useful after clearing the screen or for manual screen updates.

**Example:**
```cpp
CLI_COMMAND(clear) {
    ioStream.print("\033[2J\033[H");  // Clear screen
    cli.refreshPrompt();
    return 0;
}
```

### clearLine()

```cpp
void clearLine(void);
```

Clear the current line and move cursor to the first column using VT100 sequences.

**Example:**
```cpp
cli.clearLine();
cli.refreshPrompt();  // Show fresh prompt
```

### reset()

```cpp
void reset(void);
```

Reset the internal CLI state and print a clean prompt. Call this after errors or to start fresh.

**Example:**
```cpp
if (error_occurred) {
    ioStream.println("Error occurred, resetting CLI");
    cli.reset();
}
```

## CliCommand Class

Static class for accessing the command table and executing commands programmatically. 

Usually libCLI users do not need to interact with this class directly because its main purpose is to manage and create the internal command table and provide access to it. However, it can be useful for advanced use cases such as dynamic command execution or custom command management.

### getTable()

```cpp
static cliCmd_t* getTable(void);
```

Get a pointer to the global command table.

**Returns:** Pointer to the command table array

**Example:**
```cpp
cliCmd_t* cmds = CliCommand::getTable();
size_t count = CliCommand::getCmdCnt();

for (size_t i = 0; i < count; i++) {
    ioStream.printf("Command: %s\n", cmds[i].name);
}
```

### getCmdCnt()

```cpp
static size_t getCmdCnt(void);
```

Get the number of successfully registered commands.

**Returns:** Number of registered commands

### getDropCnt()

```cpp
static size_t getDropCnt(void);
```

Get the number of commands that couldn't be registered (exceeded CLI_COMMANDS_MAX).

**Returns:** Number of dropped commands

**Example:**
```cpp
size_t dropped = CliCommand::getDropCnt();
if (dropped > 0) {
    ioStream.printf("WARNING: %d commands were dropped!\n", dropped);
    ioStream.printf("Increase CLI_COMMANDS_MAX in configuration\n");
}
```

### getCmd()

```cpp
static CmdFuncPtr getCmd(const char* name);
```

Find a command by name and return its function pointer.

**Parameters:**
- `name` - Command name to search for

**Returns:** Function pointer or `nullptr` if not found

**Example:**
```cpp
CmdFuncPtr cmd = CliCommand::getCmd("status");
if (cmd != nullptr) {
    // Command exists
}
```

### exec()

```cpp
static int8_t exec(Stream& ioStream, const char* name, 
                   const char* argv[], uint8_t argc);
```

Execute a command programmatically by name.

**Parameters:**
- `ioStream` - Stream object for I/O
- `name` - Command name to execute
- `argv` - Argument array
- `argc` - Number of arguments

**Returns:** Command return code or error

**Example:**
```cpp
const char* args[] = {"status", "verbose"};
int8_t result = CliCommand::exec(Serial, "status", args, 2);
```

## Command Function Signature

All commands must follow this signature:

```cpp
int8_t cmd_<name>(Stream& ioStream, const char *argv[], uint8_t argc)
```

**Parameters:**

### ioStream
Reference to the I/O stream for input/output operations.

**Usage:**
```cpp
ioStream.printf("Output: %d\n", value);
ioStream.println("Hello");
ioStream.write(buffer, length);
```

### argv
Array of argument strings. `argv[0]` is always the command name.

**Example:**
For input `"status system verbose"`:
- `argv[0]` = `"status"`
- `argv[1]` = `"system"`
- `argv[2]` = `"verbose"`

### argc
Number of arguments including the command name.

**Example:**
For input `"status system verbose"`: `argc = 3`

## Return Codes

Commands should return appropriate status codes:

| Code | Meaning |
|------|---------|
| `0` | Success |
| `>0` | Command-specific warning or status |
| `<0` | Error condition |
| `INT8_MIN` | Reserved for parser errors |

**Example:**
```cpp
CLI_COMMAND(setvalue) {
    if (argc < 2) {
        ioStream.println("Error: Missing argument");
        return -1;  // Error
    }
    
    int value = atoi(argv[1]);
    if (value < 0 || value > 100) {
        ioStream.println("Error: Value out of range");
        return -2;  // Specific error code
    }
    
    // Set the value
    return 0;  // Success
}
```

## Argument Parsing

libCli supports advanced argument parsing:

### Quoted Arguments

Use quotes to include spaces in arguments:

```
command "argument with spaces" normal_arg
```

Becomes:
- `argv[1]` = `"argument with spaces"`
- `argv[2]` = `"normal_arg"`

### Escaped Characters

Use backslash to escape special characters:

```
command "quote: \" backslash: \\" arg
```

Supported escape sequences:
- `\"` - Double quote
- `\\` - Backslash
- `\n` - Newline
- `\t` - Tab
- `\r` - Carriage return

## VT100 Terminal Support

libCli includes VT100 terminal support for enhanced interaction:

### Supported Sequences

| Input | Action |
|-------|--------|
| Backspace / DEL | Delete previous character |
| Arrow Up | Previous command in history |
| Arrow Down | Next command in history |
| Ctrl+K | Clear current line |
| Ctrl+L | Clear screen |

### History Navigation

The history buffer stores multiple commands based on available space:
- Navigate with arrow keys
- Stores commands as entered (preserves escaped characters)
- Stores invalid commands (allows correction and retry)
- Automatically overwrites oldest commands when full
- Duplicate consecutive commands are filtered (v4.4.0+)

## Error Handling

### Common Error Scenarios

**Command not found:**
```
#> unknown_cmd
Error: Command not found
```

**Too many arguments:**
```
#> cmd arg1 arg2 arg3 arg4 arg5
Error: Too many arguments
```

**Unterminated string:**
```
#> cmd "unterminated string
Error: Unterminated string
```

**Command too long:**

The buffer silently truncates at CLI_COMMANDSIZ-1 and rings the bell.

## Complete Example

```cpp
#include <Arduino.h>
#include <cli/cli.hpp>

Cli cli;

CLI_COMMAND(help) {
    ioStream.println("Available commands:");
    
    cliCmd_t* cmds = CliCommand::getTable();
    size_t count = CliCommand::getCmdCnt();
    
    for (size_t i = 0; i < count; i++) {
        ioStream.printf("  %s\n", cmds[i].name);
    }
    return 0;
}

CLI_COMMAND(echo) {
    for (uint8_t i = 1; i < argc; i++) {
        if (i > 1) ioStream.print(" ");
        ioStream.print(argv[i]);
    }
    ioStream.println();
    return 0;
}

CLI_COMMAND(info) {
    ioStream.printf("Registered: %d/%d commands\n", 
        CliCommand::getCmdCnt(), CLI_COMMANDS_MAX);
    
    size_t dropped = CliCommand::getDropCnt();
    if (dropped > 0) {
        ioStream.printf("WARNING: %d commands dropped!\n", dropped);
    }
    return 0;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    cli.begin();
    
    // Check for dropped commands
    if (CliCommand::getDropCnt() > 0) {
        Serial.println("ERROR: Some commands were not registered!");
    }
}

void loop() {
    cli.loop();
}
```
