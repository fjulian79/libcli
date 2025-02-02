# libcli
A simple and generic command line interface with a small footprint for bare metal embedded projects. 

Never the less it shall provide some convenience features known from Linux shells like command repetition or some vt100 control sequences. 

## Optional configuration file
Instead of changing settings in cli.h you should add a file called cli_config.h and set the constants to the needed values. Here is a example on how this file might look like:

```C
#ifndef CLI_CONFIG_H_
#define CLI_CONFIG_H_

/**
 * @brief Defines the maximum length of a command including all arguments in 
 * bytes.
 */
#define CLI_COMMANDSIZ              128

/**
 * @brief Defines the maximum number of arguments.
 */
#define CLI_ARGVSIZ                 8

/**
 * @brief Defines the command line prompt after a new line.
 */
#define CLI_PROMPT                  "#>"

/**
 * @brief Enable if stdio is buffered to call fflush(stdout) if needed.
 */
#define CLI_BUFFEREDIO              0

#endif /* CLI_CONFIG_H_ */
```
## Supported convenience features:

### Command repetition
When pressing the UP key on the keyboard the last command will be recalled. This is comparable with the bash history but there are some limitations as libcli is made for bare metal systems and shall have a small footprint:
* Only the last command can be recalled
* As soon you start typing a new command the last one can no longe be recalled.

In this way no extra buffer for the history is needed. I have already thought about extending this feature by using the command buffer as some kind of ring buffer and remember the starting point of commands. But this is not implemented yet.

### Ring the Bell
Within your code you can ring the users terminal bell by calling ```sendBell()``` libcli does this when:
* The user want's to delete characters of his command but there is nothing to delete
* The maximum command length has been reached while typing more text.

### Delete a single character
Just use backspace as usual, it will just work
See https://vt100.net/docs/vt510-rm/chapter4.html for details.

### Delete a command line
Press ```CTRL + L``` and a entire line including the prompt will be removed from your Terminal

### Clear the screen
Press ```CTRL + K``` and a entire screen of your terminal sessions wil be cleared.

## Automatic detection of user commands

All you have to do to write a command which is then supported by libcli is to use the provided Macro:

```C
#define CLI_COMMAND(_name)                                                     \
                                                                               \
    CLI_COMMAND_FUNC(_name);                                                   \
    __attribute__((used, section(".cli_cmd_section")))                          \
    const cliCmd_t cli_cmd_##_name = {#_name, cmd_ ## _name};                  \
    CLI_COMMAND_FUNC(_name)
```

This Macro declares the comamnd function and generates a varable which will be placed in dedicated section called .cli_cmd_section. There is a add on linker script that adds ```__start_cli_cmd_section``` and  ```__stop_cli_cmd_section``` markers to this section. Finaly the ```begin(Stream *pIoStr)``` uses those markers to locate the implicit build table and calculate it's size. This approach is chosen because this allows proper handling of commands disabled by the preprocessor.

If you dont use this macro you have to create your own command table and use it as argument to ```begin(cmdTab, pIoStr)``` or ```begin(cmdTab, size, pIoStr)```. In that case you should use just ```CLI_COMMAND_FUNC(_name)``` to define your coammnds. 

## Example Code
Below you can find a small example which implements some commands, see the help text for more details. There is also a platformio based demo project: https://github.com/fjulian79/clidemo.git 

```C
include <Arduino.h>
#include <cli/cli.h>
#include <generic/generic.hpp>

#include <stdio.h>
#include <stdint.h>

#define VERSIONSTRING      "rel_1_0_0"

Cli cli;

CLI_COMMAND(ver)
{
    Serial.printf("\nclidemo %s Copyright (C) 2023 Julian Friedrich\n", 
            VERSIONSTRING);
    Serial.printf("build: %s, %s\n", __DATE__, __TIME__);
    Serial.printf("\n");
    Serial.printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n");
    Serial.printf("are welcome to redistribute it under certain conditions.\n");
    Serial.printf("See GPL v3 licence at https://www.gnu.org/licenses/ for details.\n\n");
       
    return 0;
}

CLI_COMMAND(list)
{
    Serial.printf("Recognized arguments:\n");
    for(size_t i = 0; i < argc; i++)
    {
        Serial.printf("  argv[%d]: \"%s\"\n", i, argv[i]);
    }
    
    return 0;
}

CLI_COMMAND(bell)
{
    cli.sendBell();
    Serial.printf("Sent a bell cmd\n");

    return 0; 
}

CLI_COMMAND(help)
{
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver         Used to print version infos.\n");
    Serial.printf("  list        Lists the given arguments.\n");
    Serial.printf("  bell        Used to ring the bell of the host terminal.\n");
    Serial.printf("  help        Prints this text.\n");

    return 0;
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
 
    Serial.begin(115200);
    while (!Serial);   
    Serial.println();
    cmd_ver(0, 0);
    
    cli.begin();
}

void loop()
{
    static uint32_t lastTick = 0;
    uint32_t tick = millis();

    if (tick - lastTick >= 250)
    {
        lastTick = tick;
        digitalToggle(LED_BUILTIN);
    }

    cli.read();
}
```
