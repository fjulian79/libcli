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

#include <unity.h>
#include <Arduino.h>
#include <string>

#include "cli/cli.hpp"

#define IAC             "\xff"
#define WILL            "\xfb"
#define WONT            "\xfc"
#define DO              "\xfd"
#define DONT            "\xfe"
#define SB              "\xfa"
#define SE              "\xf0"
#define NOP             "\xf1"
#define OPT_ECHO        "\x01"
#define OPT_SGA         "\x03"
#define OPT_TTYPE       "\x18"
#define OPT_NAWS        "\x1f"

#define NEGOTIATION     IAC WILL OPT_ECHO IAC WILL OPT_SGA

/* A string literal which may contain NUL bytes */
#define BIN(_lit)       std::string(_lit, sizeof(_lit) - 1)

static TestStream Io;

static CliStream Stream_;

/**
 * Reads all available data bytes.
 */
static std::string readAll(void)
{
    std::string str;

    while (Stream_.available())
        str += (char) Stream_.read();

    return str;
}

static void setInput(const std::string &str)
{
    Io.Input = str;
}

static void assertOutput(const std::string &expected)
{
    TEST_ASSERT_EQUAL(expected.size(), Io.Output.size());
    TEST_ASSERT_TRUE(expected == Io.Output);
    Io.Output.clear();
}

void setUp(void)
{
    Io.clear();
    Stream_.begin(&Io);
}

void tearDown(void) {}

void test_detached(void)
{
    CliStream stream;

    TEST_ASSERT_NULL(stream.parent());
    TEST_ASSERT_EQUAL(0, stream.available());
    TEST_ASSERT_EQUAL(-1, stream.read());
    TEST_ASSERT_EQUAL(-1, stream.peek());
    TEST_ASSERT_EQUAL(0, stream.write('x'));
}

void test_parent_and_mode(void)
{
    TEST_ASSERT_EQUAL_PTR(&Io, Stream_.parent());
    TEST_ASSERT_EQUAL(CliStream::terminal, Stream_.getMode());

    Stream_.begin(&Io, CliStream::telnet);
    TEST_ASSERT_EQUAL(CliStream::telnet, Stream_.getMode());
}

void test_data_passes(void)
{
    setInput("abc");
    TEST_ASSERT_EQUAL_STRING("abc", readAll().c_str());
}

void test_peek_does_not_consume(void)
{
    setInput("ab");
    TEST_ASSERT_EQUAL('a', Stream_.peek());
    TEST_ASSERT_EQUAL('a', Stream_.peek());
    TEST_ASSERT_EQUAL('a', Stream_.read());
    TEST_ASSERT_EQUAL('b', Stream_.read());
}

void test_cr_lf_is_read_as_cr(void)
{
    setInput("a\r\nb\r\n");
    TEST_ASSERT_EQUAL_STRING("a\rb\r", readAll().c_str());
    TEST_ASSERT_EQUAL(0, Io.available());
}

void test_cr_nul_is_read_as_cr(void)
{
    setInput(BIN("a\r\0b"));
    TEST_ASSERT_EQUAL_STRING("a\rb", readAll().c_str());
}

void test_cr_alone_is_kept(void)
{
    setInput("a\rb\r");
    TEST_ASSERT_EQUAL_STRING("a\rb\r", readAll().c_str());
}

void test_late_lf_is_dropped(void)
{
    /* On a UART the LF arrives one byte time after the CR */
    setInput("a\r");
    TEST_ASSERT_EQUAL_STRING("a\r", readAll().c_str());

    setInput("\nb");
    TEST_ASSERT_EQUAL('b', Stream_.peek());
    TEST_ASSERT_EQUAL_STRING("b", readAll().c_str());
}

void test_pending_lf_is_not_available(void)
{
    setInput("\r");
    Stream_.read();

    setInput("\n");
    TEST_ASSERT_EQUAL(0, Stream_.available());
    TEST_ASSERT_EQUAL(-1, Stream_.peek());
    TEST_ASSERT_EQUAL(0, Io.available());
}

void test_lf_not_after_cr_is_kept(void)
{
    setInput("a\nb\n\n");
    TEST_ASSERT_EQUAL_STRING("a\nb\n\n", readAll().c_str());
}

void test_second_lf_is_kept(void)
{
    setInput("\r\n\n");
    TEST_ASSERT_EQUAL_STRING("\r\n", readAll().c_str());
}

void test_nul_is_dropped(void)
{
    setInput(BIN("\0a\0b\0"));
    TEST_ASSERT_EQUAL_STRING("ab", readAll().c_str());
}

void test_begin_resets_the_state(void)
{
    setInput("\r");
    Stream_.read();

    Stream_.begin(&Io);
    setInput("\nx");
    TEST_ASSERT_EQUAL_STRING("\nx", readAll().c_str());
}

void test_terminal_output_is_unchanged(void)
{
    const char str[] = "a\nb\r\xff";

    TEST_ASSERT_EQUAL(sizeof(str) - 1, Stream_.write(str));
    assertOutput(str);
}

void test_telnet_begin_negotiates(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    assertOutput(NEGOTIATION);
}

void test_telnet_commands_are_removed(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    setInput(BIN("a" IAC NOP "b" IAC SB OPT_NAWS "\x00\x50\x00\x18" IAC SE "c"));
    TEST_ASSERT_EQUAL_STRING("abc", readAll().c_str());
}

void test_telnet_subnegotiation_with_escaped_iac(void)
{
    Stream_.begin(&Io, CliStream::telnet);

    setInput("a" IAC SB OPT_NAWS IAC IAC "\x01" IAC SE "b");
    TEST_ASSERT_EQUAL_STRING("ab", readAll().c_str());
}

void test_telnet_escaped_iac_is_data(void)
{
    Stream_.begin(&Io, CliStream::telnet);

    setInput("a" IAC IAC "b");
    TEST_ASSERT_EQUAL('a', Stream_.read());
    TEST_ASSERT_EQUAL(0xff, Stream_.peek());
    TEST_ASSERT_EQUAL(0xff, Stream_.read());
    TEST_ASSERT_EQUAL('b', Stream_.read());
    TEST_ASSERT_EQUAL(-1, Stream_.read());
}

void test_telnet_acknowledgements_are_not_answered(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    setInput(IAC DO OPT_ECHO IAC DO OPT_SGA IAC DONT OPT_TTYPE IAC WONT OPT_TTYPE "x");
    TEST_ASSERT_EQUAL_STRING("x", readAll().c_str());
    assertOutput("");
}

void test_telnet_unsupported_options_are_refused(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    setInput(IAC DO OPT_TTYPE IAC WILL OPT_NAWS "x");
    TEST_ASSERT_EQUAL_STRING("x", readAll().c_str());
    assertOutput(IAC WONT OPT_TTYPE IAC DONT OPT_NAWS);
}

void test_telnet_client_sga_is_accepted_once(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    setInput(IAC WILL OPT_SGA IAC WILL OPT_SGA "x");
    TEST_ASSERT_EQUAL_STRING("x", readAll().c_str());
    assertOutput(IAC DO OPT_SGA);
}

void test_telnet_enter(void)
{
    Stream_.begin(&Io, CliStream::telnet);

    setInput(BIN("a\r\0b\r\n"));
    TEST_ASSERT_EQUAL_STRING("a\rb\r", readAll().c_str());
}

void test_telnet_command_between_cr_and_lf(void)
{
    Stream_.begin(&Io, CliStream::telnet);

    setInput("a\r" IAC NOP "\nb");
    TEST_ASSERT_EQUAL_STRING("a\rb", readAll().c_str());
}

void test_telnet_output(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    TEST_ASSERT_EQUAL(4, Stream_.write("a\nb\n"));
    assertOutput("a\r\nb\r\n");

    TEST_ASSERT_EQUAL(4, Stream_.write("a\r\nb"));
    assertOutput("a\r\nb");

    TEST_ASSERT_EQUAL(3, Stream_.write("\rab"));
    assertOutput(BIN("\r\0ab"));

    TEST_ASSERT_EQUAL(2, Stream_.write("a\r"));
    assertOutput(BIN("a\r\0"));

    TEST_ASSERT_EQUAL(3, Stream_.write("a\xff" "b"));
    assertOutput("a" IAC IAC "b");
}

void test_telnet_long_output(void)
{
    std::string in;
    std::string out;

    /* Longer than the internal buffer, every byte is doubled */
    for (int i = 0; i < 100; i++)
    {
        in += '\n';
        out += "\r\n";
    }

    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    TEST_ASSERT_EQUAL(in.size(), Stream_.write(in.c_str()));
    assertOutput(out);
}

void test_printf(void)
{
    Stream_.begin(&Io, CliStream::telnet);
    Io.Output.clear();

    Stream_.printf("%d\n", 42);
    assertOutput("42\r\n");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_detached);
    RUN_TEST(test_parent_and_mode);
    RUN_TEST(test_data_passes);
    RUN_TEST(test_peek_does_not_consume);
    RUN_TEST(test_cr_lf_is_read_as_cr);
    RUN_TEST(test_cr_nul_is_read_as_cr);
    RUN_TEST(test_cr_alone_is_kept);
    RUN_TEST(test_late_lf_is_dropped);
    RUN_TEST(test_pending_lf_is_not_available);
    RUN_TEST(test_lf_not_after_cr_is_kept);
    RUN_TEST(test_second_lf_is_kept);
    RUN_TEST(test_nul_is_dropped);
    RUN_TEST(test_begin_resets_the_state);
    RUN_TEST(test_terminal_output_is_unchanged);

    RUN_TEST(test_telnet_begin_negotiates);
    RUN_TEST(test_telnet_commands_are_removed);
    RUN_TEST(test_telnet_subnegotiation_with_escaped_iac);
    RUN_TEST(test_telnet_escaped_iac_is_data);
    RUN_TEST(test_telnet_acknowledgements_are_not_answered);
    RUN_TEST(test_telnet_unsupported_options_are_refused);
    RUN_TEST(test_telnet_client_sga_is_accepted_once);
    RUN_TEST(test_telnet_enter);
    RUN_TEST(test_telnet_command_between_cr_and_lf);
    RUN_TEST(test_telnet_output);
    RUN_TEST(test_telnet_long_output);
    RUN_TEST(test_printf);

    return UNITY_END();
}
