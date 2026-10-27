/**
 * This example makes the CLI available via telnet, while it is still usable
 * via Serial as long as no telnet client is connected. It works on the ESP32,
 * the ESP8266 and the Raspberry Pi Pico W. Set your WiFi credentials below,
 * the IP address is printed via Serial, then connect with:
 *
 *      telnet <ip address>
 *
 * It shows how to use libcli with any other stream:
 *  - cli.setStream() selects where libcli writes to.
 *  - cli.read() hands a single received byte to libcli, so the input can be
 *    filtered before. This is used here to remove the telnet protocol bytes,
 *    which are no user input.
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

/**
 * Telnet expects "\r\n" as line end, libcli writes "\n". This stream adds the
 * missing "\r" when writing to the client and passes everything else through.
 */
class TelnetStream : public Stream
{
    public:

        size_t write(uint8_t c) override
        {
            return write(&c, 1);
        }

        size_t write(const uint8_t *buffer, size_t size) override
        {
            size_t start = 0;

            /* Write the data in chunks, as each write may send a TCP packet */
            for (size_t i = 0; i < size; i++)
            {
                if (buffer[i] == '\n')
                {
                    client.write(&buffer[start], i - start);
                    client.write((const uint8_t *) "\r\n", 2);
                    start = i + 1;
                }
            }

            client.write(&buffer[start], size - start);
            return size;
        }

        int available() override
        {
            return client.available();
        }

        int read() override
        {
            return client.read();
        }

        int peek() override
        {
            return client.peek();
        }

        void flush() override
        {
            client.flush();
        }
};

TelnetStream telnetStream;

/**
 * The telnet protocol, see RFC 854. Commands start with IAC, option
 * negotiation is followed by one option byte, subnegotiation ends with IAC SE.
 */
const uint8_t IAC = 255;
const uint8_t DONT = 254;
const uint8_t DO = 253;
const uint8_t WONT = 252;
const uint8_t WILL = 251;
const uint8_t SB = 250;
const uint8_t SE = 240;
const uint8_t OPT_ECHO = 1;
const uint8_t OPT_SUPPRESS_GO_AHEAD = 3;

enum
{
    tn_data,
    tn_command,
    tn_option,
    tn_subneg,
    tn_subneg_iac
}
telnetState = tn_data;

/**
 * Removes the telnet protocol bytes and passes the user input to libcli.
 */
void telnetInput(uint8_t byte)
{
    switch (telnetState)
    {
        case tn_data:
            if (byte == IAC)
            {
                telnetState = tn_command;
            }
            else if (byte != '\0')
            {
                /* Enter is sent as "\r\0" or "\r\n", libcli ignores the
                 * "\n" but not the "\0" */
                cli.read(byte);
            }
            break;

        case tn_command:
            if (byte == SB)
            {
                telnetState = tn_subneg;
            }
            else if (byte >= WILL && byte <= DONT)
            {
                telnetState = tn_option;
            }
            else
            {
                /* Any other command, or an escaped 255, which is no valid
                 * input for a CLI anyway */
                telnetState = tn_data;
            }
            break;

        case tn_option:
            telnetState = tn_data;
            break;

        case tn_subneg:
            if (byte == IAC)
            {
                telnetState = tn_subneg_iac;
            }
            break;

        case tn_subneg_iac:
            telnetState = (byte == SE) ? tn_data : tn_subneg;
            break;
    }
}

void checkClient()
{
    /* The server echoes the input and the client sends every character
     * immediately instead of whole lines, as libcli expects it */
    const uint8_t negotiation[] = {
        IAC, WILL, OPT_ECHO,
        IAC, WILL, OPT_SUPPRESS_GO_AHEAD
    };

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
            telnetState = tn_data;
            client.write(negotiation, sizeof(negotiation));

            Serial.printf("Telnet client connected: %s\n",
                client.remoteIP().toString().c_str());
            Serial.println("The CLI is available via telnet only now");

            telnetStream.printf("Welcome to libcli via telnet\n");
            cli.setStream(&telnetStream);
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

    if (clientConnected)
    {
        while (client.available())
        {
            telnetInput(client.read());
        }
    }
    else
    {
        cli.loop();
    }
}
