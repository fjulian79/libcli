# API Reference

This file is intended to provide infos on the public API of libCli which is intended to be used by 'users'. This file will not provide detailed information on the internal workings of the library, as this is done in the source code by comments where necessary. 

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

**How does auto-registration work?**

The `CLI_COMMAND` macro uses constructor-based registration via static C++ objects initialized before `main()`. This approach is preprocessor-aware and works across all Arduino platforms without requiring linker tricks or build scripts.

For a detailed explanation of the registration mechanism, why this design was chosen over alternatives (Python scripts, linker sections), and how it handles conditional compilation, see [Command Registration](COMMAND_REGISTRATION.md).

### CLI_COMMAND_DEF(name)

**Description:** Macro to only define the command signature without registration. Useful for forward declarations.

This macro can be used when you need to or want to declare a command fucntion which is implemented somewhere else but you want to execute it directly with minimal overhead.

However, the CliCommand::exec() method can be used to execute commands by name at runtime without the need for forward declarations, as it looks up the command in the command table. So in most cases, you don't need to use CLI_COMMAND_DEF unless you want to call the command function directly by its name (e.g., cmd_<name>) instead of using CliCommand::exec(). See [CliCommand Class](#clicommand-class) documentation for more details on how to execute commands programmatically.

**Usage:**
```cpp
// Forward declaration
CLI_COMMAND_DEF(mycommand);

// Example for manual command execution with forward declaration
cmd_mycommand(ioStream, nullptr, 0);

// Example for manual command execution without the need of forward declarations. 
// In this case you don't need forward decalartion because it command is found by name at runtime. 
CliCommand::exec(ioStream, "mycommand", nullptr, 0);

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

Process a single incoming byte. Useful for custom input handling. Usually you don't need this method as `loop()` handles reading from the stream by calling `read()` internally, but it's available for corner cases if needed.

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

**Important:** Changing the stream implicitly resets the CLI state (input buffer, history navigation, tab completion state) to avoid inconsistencies.

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

### clearScreen()

```cpp
void clearScreen(void);
```

Clear the entire screen and move the cursor to the first row and column using VT100 sequences.

**Example:**
```cpp
CLI_COMMAND(clear) {
    cli.clearScreen();
    cli.refreshPrompt();
    return 0;
}
```

### saveCursor()

```cpp
void saveCursor(void);
```

Save the current terminal cursor position using VT100 escape sequences (DECSC). The position can be restored later with `restoreCursor()`.

**Use Cases:**
- Within libCli for printing completion matches while preserving the prompt position
- Multi-line status displays

**Example:**
```cpp
cli.saveCursor();
ioStream.println("\nTemporary status info");
cli.restoreCursor();  // Back to original position
```

### restoreCursor()

```cpp
void restoreCursor(void);
```

Restore the terminal cursor to the position previously saved by `saveCursor()` using VT100 escape sequences (DECRC).

**Note:** Must be called after `saveCursor()`, otherwise behavior is terminal-dependent.

**Example:**
```cpp
cli.saveCursor();
ioStream.println("\nStatus: Processing...");
delay(1000);
cli.restoreCursor();
ioStream.println("\nStatus: Complete!    ");
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

### Return Code Conventions

Commands should return appropriate status codes:

| Code | Meaning |
|------|---------|
| `0` | Success |
| `!= 0` | Error condition |

When a command returns a non-zero value, libCli will print an error message:
```
Error, cmd fails: <error code>
``` 

to the terminal, where `<error code>` is the value returned by the command. This allows users to implement their own error code scheme. 

### Special Case: libCli Parser Errors:

in case of a parsing error (e.g., unterminated string, too many arguments), read() and loop() return `INT8_MIN` and print a specific error message to the terminal:

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

**Example:**
```cpp
CLI_COMMAND(setvalue) {
    if (argc < 2) {
        ioStream.println("Error: Missing argument");
        // Error
        return -1;
    }
    
    int value = atoi(argv[1]);
    if (value < 0 || value > 100) {
        ioStream.println("Error: Value out of range");
        // Error
        return -2;
    }
    
    // Success
    return 0;  
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

Supported escape sequences:
- `\"` - Double quote
- `\\` - Backslash

Example:
```
command "quote: \" backslash: \\" arg
```
Becomes:
- `argv[1]` = `quote: " backslash: \`
- `argv[2]` = `arg`


## Complete Example

```cpp
#include <Arduino.h>
#include <cli/cli.hpp>

Cli cli;

CLI_COMMAND(ver) {
    ioStream.println("myApp version 1.0");
    return 0;
}

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

    // Call by name at runtime without forward declaration
    CliCommand::exec(Serial, "ver", nullptr, 0);
    // Direct call without lookup depending on forward declaration
    cmd_info(Serial, nullptr, 0);
    
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
