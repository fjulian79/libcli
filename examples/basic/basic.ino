/**
 * This is a basic example on how to use the libcli library.
 * It shows how to define commands and how to use them.
 *
 * Note that CLI_COMMAND is a macro that defines a function and registers it
 * with the CliCommand class. The macro takes the name of the command as an
 * argument. The function is created with the following signature:
 *
 *      int8_t cmd_<name>(Stream &ioStream, const char *argv[], uint8_t argc)
 *
 * Using CLI_COMMAND(name), the command is automatically added to the command
 * table—no need to manually register it!
 *
 * The function must return an int8_t, zero in case of success, and gets the
 * following arguments:
 *  - ioStream: A reference to the stream object that is used for input/output.
 *  - argv: An array of strings containing the arguments, the command name
 *    itself is not part of it.
 *  - argc: The number of arguments passed to the command.
 *
 * The function can use the ioStream object to print output to the console or
 * whatever is used. In this example the used stream is defined by the
 * cli.begin() function, as it takes a pointer to the stream object as
 * argument and Serial is used as default.
 *
 * Try the tab key to complete commands and the arrow keys to navigate through
 * the command history. See the other examples for arguments, controlling
 * hardware, executing commands from code and using the CLI via telnet.
 *
 * If you want to learn more, check the cli/cli.hpp file, it's well documented.
 * Options you can configure if needed are documented in cli/config.hpp.
 * The auto command registration is documented in cli/command.hpp.
 */

#include <Arduino.h>
#include <cli/cli.hpp>

Cli cli;

CLI_COMMAND(ver)
{
    ioStream.printf("\nlibcli_example v1.0.0, Copyright (C) 2025 Julian Friedrich\n");
    ioStream.printf("libcli:   %s\n", CLI_VERSION);
    ioStream.printf("Build:    %s, %s\n", __DATE__, __TIME__);
    ioStream.printf("\n");

    return 0;
}

CLI_COMMAND(info)
{
    cliCmd_t *pCmdTab = CliCommand::getTable();
    size_t cmdCnt = CliCommand::getCmdCnt();
    size_t dropCnt = CliCommand::getDropCnt();

    ioStream.printf("  Supported commands:    %u\n", (unsigned int) CLI_COMMANDS_MAX);
    ioStream.printf("  Registered commands:   %u\n", (unsigned int) cmdCnt);
    ioStream.printf("  Dropped commands:      %u\n", (unsigned int) dropCnt);
    ioStream.printf("  Command name's:\n");

    for(size_t i = 0; i < cmdCnt; i++)
    {
        ioStream.printf("    %s\n", pCmdTab[i].name);
    }
    ioStream.print("\n");

    return 0;
}

CLI_COMMAND(bell)
{
    cli.sendBell();
    ioStream.printf("Sent a vt100 bell signal.\n");

    return 0;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println();
    cmd_ver(Serial, 0, 0);

    cli.begin();
}

void loop()
{
  cli.loop();
}
