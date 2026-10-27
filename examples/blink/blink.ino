/**
 * This example controls the built-in LED via the CLI, while it keeps blinking
 * in the background. It shows that cli.loop() never blocks, so the CLI can be
 * added to any sketch which does other things in loop() as well. Try:
 *
 *      blink 200
 *      blink 1000
 *      led on
 *      led toggle
 *      status
 *
 * Do not use delay() in loop() when using the CLI, it would make the CLI slow
 * to respond. Use millis() instead, as shown below.
 */

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <cli/cli.hpp>

#ifndef LED_BUILTIN
/* Not every board defines it, e.g. the ESP32 DevKit has the LED on GPIO 2 */
#define LED_BUILTIN 2
#endif

Cli cli;

/**
 * The blink period in milliseconds, zero if blinking is turned off.
 */
uint32_t blinkPeriod = 500;

uint32_t lastToggle = 0;

bool ledState = false;

void setLed(bool on)
{
    ledState = on;
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

CLI_COMMAND(led)
{
    if (argc != 1)
    {
        ioStream.printf("Usage: led <on|off|toggle>\n");
        return -1;
    }

    if (strcmp(argv[0], "on") == 0)
    {
        setLed(true);
    }
    else if (strcmp(argv[0], "off") == 0)
    {
        setLed(false);
    }
    else if (strcmp(argv[0], "toggle") == 0)
    {
        setLed(!ledState);
    }
    else
    {
        ioStream.printf("Unknown state: %s\n", argv[0]);
        return -2;
    }

    /* Switching the LED manually stops blinking */
    blinkPeriod = 0;
    return 0;
}

CLI_COMMAND(blink)
{
    long period;

    if (argc != 1)
    {
        ioStream.printf("Usage: blink <period in ms|off>\n");
        return -1;
    }

    if (strcmp(argv[0], "off") == 0)
    {
        blinkPeriod = 0;
        setLed(false);
        return 0;
    }

    period = atol(argv[0]);
    if (period < 10)
    {
        ioStream.printf("The period must be at least 10 ms\n");
        return -2;
    }

    blinkPeriod = (uint32_t) period;
    return 0;
}

CLI_COMMAND(status)
{
    ioStream.printf("LED:    %s\n", ledState ? "on" : "off");

    if (blinkPeriod != 0)
    {
        ioStream.printf("Blink:  %lu ms\n", (unsigned long) blinkPeriod);
    }
    else
    {
        ioStream.printf("Blink:  off\n");
    }

    ioStream.printf("Uptime: %lu ms\n", (unsigned long) millis());
    return 0;
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while (!Serial);
    Serial.println();
    Serial.println("libcli blink example, try: blink 200");

    cli.begin();
}

void loop()
{
    cli.loop();

    if (blinkPeriod != 0 && millis() - lastToggle >= blinkPeriod)
    {
        lastToggle = millis();
        setLed(!ledState);
    }
}
