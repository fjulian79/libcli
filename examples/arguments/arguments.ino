/**
 * This example shows how to work with the arguments of a command.
 *
 * libcli splits the input at spaces, the command name itself is not part of
 * argv. Text in double quotes is passed as one argument, a quote within it
 * can be escaped with a backslash. Try:
 *
 *      echo one two
 *      echo "hello world" "say \"hi\""
 *      add 1 2 0x10
 *      set speed 100
 *      get speed
 *      get
 *
 * A command returns zero in case of success. Any other value makes libcli
 * print "Error, cmd fails: <value>" after the command has returned, so a
 * command only needs to print why it failed.
 *
 * At most CLI_ARGVSIZ (default: 4) arguments are passed to a command, see
 * cli/config.hpp on how to change it.
 */

#include <Arduino.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <cli/cli.hpp>

Cli cli;

/**
 * Some named settings, which can be read and written by the get and set
 * commands.
 */
struct {
    const char *name;
    long value;
} settings[] = {
    {"speed",   50},
    {"limit",   1000},
    {"verbose", 0},
};

/**
 * Converts the given string into a number, decimal or hexadecimal with a
 * leading 0x. Prints an error and returns false if it is no valid number.
 */
bool parseNumber(Stream &ioStream, const char *str, long &value)
{
    char *end;

    errno = 0;
    value = strtol(str, &end, 0);

    if (end == str || *end != '\0' || errno != 0)
    {
        ioStream.printf("Not a valid number: %s\n", str);
        return false;
    }

    return true;
}

/**
 * Returns the setting with the given name or nullptr if it does not exist.
 */
long *findSetting(const char *name)
{
    for (size_t i = 0; i < sizeof(settings) / sizeof(settings[0]); i++)
    {
        if (strcmp(settings[i].name, name) == 0)
        {
            return &settings[i].value;
        }
    }

    return nullptr;
}

CLI_COMMAND(echo)
{
    for (uint8_t i = 0; i < argc; i++)
    {
        ioStream.printf("argv[%u] = '%s'\n", (unsigned int) i, argv[i]);
    }

    return 0;
}

CLI_COMMAND(add)
{
    long sum = 0;
    long value;

    if (argc < 2)
    {
        ioStream.printf("Usage: add <number> <number> ...\n");
        return -1;
    }

    for (uint8_t i = 0; i < argc; i++)
    {
        if (!parseNumber(ioStream, argv[i], value))
        {
            return -2;
        }

        sum += value;
    }

    ioStream.printf("%ld\n", sum);
    return 0;
}

CLI_COMMAND(set)
{
    long *pSetting;
    long value;

    if (argc != 2)
    {
        ioStream.printf("Usage: set <name> <value>\n");
        return -1;
    }

    pSetting = findSetting(argv[0]);
    if (pSetting == nullptr)
    {
        ioStream.printf("Unknown setting: %s\n", argv[0]);
        return -2;
    }

    if (!parseNumber(ioStream, argv[1], value))
    {
        return -3;
    }

    *pSetting = value;
    return 0;
}

CLI_COMMAND(get)
{
    long *pSetting;

    if (argc == 0)
    {
        /* No argument, print all settings */
        for (size_t i = 0; i < sizeof(settings) / sizeof(settings[0]); i++)
        {
            ioStream.printf("%-8s %ld\n", settings[i].name, settings[i].value);
        }

        return 0;
    }

    for (uint8_t i = 0; i < argc; i++)
    {
        pSetting = findSetting(argv[i]);
        if (pSetting == nullptr)
        {
            ioStream.printf("Unknown setting: %s\n", argv[i]);
            return -2;
        }

        ioStream.printf("%-8s %ld\n", argv[i], *pSetting);
    }

    return 0;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println();
    Serial.println("libcli arguments example, try: echo \"hello world\"");

    cli.begin();
}

void loop()
{
    cli.loop();
}
