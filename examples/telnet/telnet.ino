/**
 * This example makes the CLI available via telnet, while it is still usable
 * via Serial as long as no telnet client is connected. It works on the ESP32,
 * the ESP8266 and the Raspberry Pi Pico W. Set your WiFi credentials below,
 * the IP address is printed via Serial, then connect with:
 *
 *      telnet <ip address>
 *
 * It shows how to use libcli with a telnet client: cli.setStream() with
 * CliStream::telnet makes libcli handle the telnet protocol. It switches the
 * client to character mode, so the cursor keys, the command history and the
 * tab completion work as on Serial, removes the telnet commands from the input
 * and sends a new line as "\r\n". See doc/line-endings.md for the details.
 *
 * ATTENTION: Telnet is not encrypted and there is no password, anybody in
 * your network can connect. Only use it in trusted networks.
 */

#include <Arduino.h>
#include <cli/cli.hpp>

#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "This example needs WiFi, it supports the ESP32, the ESP8266 and the Pi Pico W"
#endif

const char *ssid = "your-ssid";
const char *password = "your-password";

Cli cli;

WiFiServer server(23);

WiFiClient client;

bool clientConnected = false;

void checkClient()
{
    if (clientConnected && !client.connected())
    {
        client.stop();
        clientConnected = false;
        cli.setStream(&Serial);
        Serial.println("Telnet client disconnected");
    }

    if (!clientConnected)
    {
        WiFiClient newClient = server.accept();

        if (newClient)
        {
            client = newClient;
            clientConnected = true;

            Serial.printf("Telnet client connected: %s\n",
                client.remoteIP().toString().c_str());
            Serial.println("The CLI is available via telnet only now");

            client.print("Welcome to libcli via telnet\r\n");
            cli.setStream(&client, CliStream::telnet);
        }
    }
}

CLI_COMMAND(hello)
{
    ioStream.printf("Hello %s!\n", argc > 0 ? argv[0] : "world");
    return 0;
}

CLI_COMMAND(wifi)
{
    ioStream.printf("SSID: %s\n", WiFi.SSID().c_str());
    ioStream.printf("IP:   %s\n", WiFi.localIP().toString().c_str());
    ioStream.printf("RSSI: %d dBm\n", (int) WiFi.RSSI());
    return 0;
}

CLI_COMMAND(quit)
{
    if (!clientConnected)
    {
        ioStream.printf("No telnet client connected\n");
        return -1;
    }

    ioStream.printf("Bye\n");
    client.stop();
    return 0;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println();

    Serial.printf("Connecting to %s ", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    server.begin();
    server.setNoDelay(true);
    Serial.printf("Connect with: telnet %s\n", WiFi.localIP().toString().c_str());

    cli.begin();
}

void loop()
{
    checkClient();
    cli.loop();
}
