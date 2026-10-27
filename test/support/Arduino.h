/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2026 Julian Friedrich
 *
 * This file is licensed under the MIT License, see LICENSE.txt for
 * details.
 *
 * SPDX-License-Identifier: MIT
 *
 * This project is hosted on GitHub:
 *   https://github.com/fjulian79/libcli
 * Please feel free to file issues, open pull requests, or contribute there.
 */

/*
 * Minimal replacement of the Arduino API, only used to run the unit tests on
 * the host. Provides just what libcli needs, TestStream collects everything
 * written to it in Output and hands out the bytes of Input to read().
 */

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

typedef std::string String;

/**
 * Like the Print class of Arduino, all output ends up in write(uint8_t).
 */
class Print
{
    public:

        virtual ~Print() {}

        virtual size_t write(uint8_t c) = 0;

        /* Stops at the first character not written, like Arduino */
        virtual size_t write(const uint8_t *buffer, size_t size)
        {
            size_t n = 0;

            while (size-- && write(*buffer++))
                n++;

            return n;
        }

        size_t write(const char *buffer, size_t size)
        {
            return write((const uint8_t *) buffer, size);
        }

        size_t write(const char *str)
        {
            return str == 0 ? 0 : write(str, strlen(str));
        }

        size_t print(const char *str)
        {
            return write(str);
        }

        size_t print(char c)
        {
            return write((uint8_t) c);
        }

        /* Provided by the ESP32, STM32 and RP2040 cores, not by every core */
        __attribute__((format(printf, 2, 3)))
        size_t printf(const char *format, ...)
        {
            char tmp[256];
            va_list args;
            int len;

            va_start(args, format);
            len = vsnprintf(tmp, sizeof(tmp), format, args);
            va_end(args);

            if (len < 0)
                return 0;

            if ((size_t) len >= sizeof(tmp))
                len = sizeof(tmp) - 1;

            return write(tmp, (size_t) len);
        }

        virtual void flush(void) {}
};

class Stream : public Print
{
    public:

        virtual int available(void) = 0;

        virtual int read(void) = 0;

        virtual int peek(void) = 0;
};

/**
 * A Stream reading from Input and collecting everything written to it in
 * Output, e.g. as terminal.
 */
class TestStream : public Stream
{
    public:

        using Print::write;

        size_t write(uint8_t c) override
        {
            Output += (char) c;
            return 1;
        }

        int available(void) override
        {
            return (int) Input.length();
        }

        int read(void) override
        {
            int c = peek();

            if (c >= 0)
                Input.erase(0, 1);

            return c;
        }

        int peek(void) override
        {
            return Input.empty() ? -1 : (uint8_t) Input[0];
        }

        void clear(void)
        {
            Input.clear();
            Output.clear();
        }

        String Input;

        String Output;
};

inline TestStream &testSerial(void)
{
    static TestStream serial;
    return serial;
}

#define Serial testSerial()
