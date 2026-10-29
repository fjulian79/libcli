/**
 * This example shows how to execute commands from code, e.g. to run them at
 * startup, from a button or for commands received via MQTT, HTTP or any other
 * source which is no stream. Try:
 *
 *      hello
 *      hello libcli
 *      repeat 3 hello world
 *
 * CliCommand::exec() looks up a command by its name and executes it. Like an
 * interactively typed command, the command name is not part of argv.
 *
 * If the command is known at compile time, cmd_<name>() can also be called
 * directly. To call a command defined in another file, declare it with
 * CLI_COMMAND_DEF(name).
 */

#include <Arduino.h>
#include <stdlib.h>
#include <cli/cli.hpp>

Cli cli;

CLI_COMMAND(hello)
{
    ioStream.printf("Hello %s!\n", argc > 0 ? argv[0] : "world");
    return 0;
}

/**
 * Executes the command given as arguments several times, e.g.
 * "repeat 3 hello world" executes "hello world" three times.
 */
CLI_COMMAND(repeat)
{
    int8_t ret;
    long count;

    if (argc < 2)
    {
        ioStream.printf("Usage: repeat <count> <command> [arguments]\n");
        return -1;
    }

    count = atol(argv[0]);
    for (long i = 0; i < count; i++)
    {
        /* argv[1] is the name of the command, its arguments follow */
        ret = CliCommand::exec(ioStream, argv[1], &argv[2], argc - 2);
        if (ret != 0)
        {
            /* exec() returns -1 if the command does not exist */
            ioStream.printf("%s failed: %d\n", argv[1], ret);
            return -2;
        }
    }

    return 0;
}

void setup()
{
    const char *args[] = {"from setup()"};

    Serial.begin(115200);
    while (!Serial);
    Serial.println();

    /* Executes a command by its name, e.g. at startup */
    CliCommand::exec(Serial, "hello", args, 1);

    /* Calls a command directly, as it is known at compile time. A command
     * takes a CliStream, which wraps the stream to use. */
    CliStream stream(&Serial);
    cmd_hello(stream, nullptr, 0);

    cli.begin();
}

void loop()
{
    cli.loop();
}
