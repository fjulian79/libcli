# libcli
A simple and generic command line interface with a small footprint for bare metal embedded projects. 

Never the less it shall provide some convinience features known from Linux shells like command repetition or some vt100 controll sequences. 

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
## Supportet comvienence features:

### Command repitition
When pressing the UP key on the keyboard the last command will be recalled. This is comparable with the bash history but there are some limitiations as libcli is made for bare mettal systems and shall have a small footprint:
* Only the last comamnd can be recalled
* As soon you start typing a new command the last one can no longe be recalled.

In this way no extra buffer for the history is needed. I have allready thought about extending this feature by using the command buffer as some kind of ring buffer and remember the starting point of commands. But this is not implemented yet.

### Ring the Bell
Within your code you can ring the users terminal bell by calling ```sendBell()``` libcli does this wehn:
* The user want's to delete caracters of his comamnd but there is nothing to delete
* The maximum command lenght has been reached while typing more text.

### Delete a single character
Just use backspace as usual, it will just work
See https://vt100.net/docs/vt510-rm/chapter4.html for details.

### Delete a commandline
Press ```CTRL + L``` and a entire line including the prompt will be removed from your Teminal

### Clear the screen
Press ```CTRL + K``` and a entire screen of your teminal sessions wil be cleared.

## Example Code
This is a small example which implements some comamnds, see the help text for more details.

```C
include <Arduino.h>
#include "cli/cli.h"
#include "generic/generic.hpp"

#include <stdio.h>
#include <stdint.h>

#define VERSIONSTRING      "rel_1_0_0"

Cli cli;

int8_t cmd_ver(char *argv[], uint8_t argc)
{
    Serial.printf("\nclidemo %s Copyright (C) 2020 Julian Friedrich\n", 
            VERSIONSTRING);
    Serial.printf("build: %s, %s\n", __DATE__, __TIME__);
    Serial.printf("\n");
    Serial.printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n");
    Serial.printf("are welcome to redistribute it under certain conditions.\n");
    Serial.printf("See GPL v3 licence at https://www.gnu.org/licenses/ for details.\n\n");
       
    return 0;
}

int8_t cmd_list(char *argv[], uint8_t argc)
{
    Serial.printf("Recognized arguments:\n");
    for(size_t i = 0; i < argc; i++)
    {
        Serial.printf("  argv[%d]: \"%s\"\n", i, argv[i]);
    }
    
    return 0;
}

int8_t cmd_bell(char *argv[], uint8_t argc)
{
    cli.sendBell();
    Serial.printf("Sent a bell cmd\n");

    return 0; 
}

int8_t cmd_help(char *argv[], uint8_t argc)
{
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver         Used to print version infos.\n");
    Serial.printf("  list        Lists the given arguments.\n");
    Serial.printf("  bell        Used to ring the bell of the host terminal.\n");
    Serial.printf("  help        Prints this text.\n");

    return 0;
}

/**
 * @brief The table of supported commands.
 */
cliCmd_t cmdTable[] =
{
   CLI_CMD_DEF("ver"),              // Use the provided macro ..
   CLI_CMD_DEF("list"),
   {"bell", cmd_bell},              // .. or do it on your own if your function
   {"help", cmd_help},              // names do not match with the commands
};

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
 
    Serial.begin(115200);
    while (!Serial);   
    Serial.println();
    cmd_ver(0, 0);
    cli.begin(cmdTable);
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