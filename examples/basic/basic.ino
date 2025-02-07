/**
 * This is a basic example on how to use the libcli library.
 * It shows how to define commands and how to use them.
 * 
 * Note that CLI_COMMAND is a macro that defines a function and registers it
 * with the CliCommand class. The macro takes the name of the function as an
 * argument. The function is created with the following signature:
 * 
 *      int cmd_<name>(Stream &ioStream, int argc, char *argv[])
 * 
 * Using CLI_COMMAND(name), the command is automatically added to the command 
 * table—no need to manually register it!
 * 
 * The function must return an integer and take the following arguments:
 *  - ioStream: A reference to the stream object that is used for input/output.
 *  - argc: The number of arguments passed to the command.
 *  - argv: An array of strings containing the arguments.
 * 
 * The function can use the ioStream object to print output to the console or 
 * whatever is used. In this example the used stream is defined by the 
 * cli.begin() function, as it takes a reference to the stream object as 
 * argument and Serial is used as default.
 * 
 * The other two arguments are created by libcli based on your input.
 * libcli has a configurable command separator (default: space). You can change 
 * it in config.hpp if needed.
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
    ioStream.printf("Build:    %s, %s\n", __DATE__, __TIME__);
    ioStream.printf("\n");

    return 0;
}

CLI_COMMAND(info)
{
    cliCmd_t *pCmdTab = CliCommand::getTable();
    size_t cmdCnt = CliCommand::getCmdCnt();
    size_t dropCnt = CliCommand::getDropCnt();

    ioStream.printf("  Supported commands:    %d\n", CLI_COMMANDS_MAX);
    ioStream.printf("  Registered commands:   %d\n", cmdCnt);
    ioStream.printf("  Dropped commands:      %d\n", dropCnt);
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
