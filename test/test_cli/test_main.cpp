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
#include <vector>

#include "cli/cli.hpp"

#define UP              "\033[A"
#define DOWN            "\033[B"
#define DEL             "\b\033[0K"
#define CLRLINE         "\033[999D\033[2K"
#define CLRSCR          "\033[2J\033[1;1H"

static TestStream Io;

static Cli *pCli;

static int CapCalls;

static std::vector<std::string> CapArgs;

/**
 * Captures its arguments.
 */
CLI_COMMAND(cap)
{
    CapCalls++;
    CapArgs.clear();
    for (uint8_t i = 0; i < argc; i++)
        CapArgs.push_back(argv[i]);

    ioStream.print("ok");
    return 0;
}

CLI_COMMAND(fail)
{
    (void) ioStream;
    (void) argv;
    (void) argc;
    return 5;
}

static int PeekedByCmd;

static Stream *ParentOfCmd;

static CliStream *StreamOfCmd;

/**
 * Captures what a command reading the input itself sees.
 */
CLI_COMMAND(peek)
{
    (void) argv;
    (void) argc;
    PeekedByCmd = ioStream.peek();
    ParentOfCmd = ioStream.parent();
    StreamOfCmd = &ioStream;
    return 0;
}

/**
 * Runs the peek command by exec() with its own ioStream.
 */
CLI_COMMAND(nested)
{
    (void) argv;
    (void) argc;
    return CliCommand::exec(ioStream, "peek", nullptr, 0);
}

static int8_t feed(const char *str)
{
    int8_t ret = 0;

    while (*str)
        ret = pCli->read(*str++);

    return ret;
}

static void assertArgs(const std::vector<std::string> &expected)
{
    TEST_ASSERT_EQUAL(expected.size(), CapArgs.size());
    for (size_t i = 0; i < expected.size(); i++)
        TEST_ASSERT_EQUAL_STRING(expected[i].c_str(), CapArgs[i].c_str());
}

static void assertOutput(const std::string &expected)
{
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), Io.Output.c_str());
    Io.Output.clear();
}

void setUp(void)
{
    pCli = new Cli();
    pCli->begin(&Io);
    Io.clear();
    CapCalls = 0;
    CapArgs.clear();
}

void tearDown(void)
{
    delete pCli;
}

void test_begin_prints_the_prompt(void)
{
    Cli cli;

    Io.clear();
    cli.begin(&Io);
    assertOutput(CLI_PROMPT);
}

void test_loop_without_stream(void)
{
    Cli cli;

    TEST_ASSERT_EQUAL(0, cli.loop());
}

void test_loop_reads_the_stream(void)
{
    Io.Input = "cap\r";
    while (Io.available())
        pCli->loop();

    TEST_ASSERT_EQUAL(1, CapCalls);
    TEST_ASSERT_EQUAL(0, pCli->loop());
}

void test_command_without_arguments(void)
{
    TEST_ASSERT_EQUAL(0, feed("cap\r"));
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertArgs({});
    assertOutput("cap\nok" CLI_PROMPT);
}

void test_command_with_arguments(void)
{
    feed("cap a bc d\r");
    assertArgs({"a", "bc", "d"});
}

void test_multiple_separators(void)
{
    feed("cap   a  b   \r");
    assertArgs({"a", "b"});
}

void test_string_argument(void)
{
    feed("cap \"a b\" c\r");
    assertArgs({"a b", "c"});
}

void test_escaped_quotes_in_string(void)
{
    feed("cap \"say \\\"hi\\\"\"\r");
    assertArgs({"say \"hi\""});
}

void test_escaped_quote_at_string_start(void)
{
    feed("cap \"\\\"hi\\\"\"\r");
    assertArgs({"\"hi\""});
}

void test_empty_string_argument(void)
{
    TEST_ASSERT_EQUAL(0, feed("cap \"\" x\r"));
    assertArgs({"", "x"});
}

void test_backslash_outside_string_is_kept(void)
{
    feed("cap a\\b\r");
    assertArgs({"a\\b"});
}

void test_quote_inside_argument_is_kept(void)
{
    feed("cap a\"b\r");
    assertArgs({"a\"b"});
}

void test_max_arguments(void)
{
    std::string cmd = "cap";
    std::vector<std::string> expected;

    for (int i = 0; i < CLI_ARGVSIZ; i++) {
        expected.push_back(std::to_string(i));
        cmd += " " + expected.back();
    }

    TEST_ASSERT_EQUAL(0, feed((cmd + "\r").c_str()));
    assertArgs(expected);
}

void test_too_many_arguments(void)
{
    std::string cmd = "cap";

    for (int i = 0; i <= CLI_ARGVSIZ; i++)
        cmd += " x";

    TEST_ASSERT_EQUAL(INT8_MIN, feed((cmd + "\r").c_str()));
    TEST_ASSERT_EQUAL(0, CapCalls);
    assertOutput(cmd + "\nError, too many arguments (max: "
        + std::to_string(CLI_ARGVSIZ) + ")\n" CLI_PROMPT);
}

void test_unterminated_string(void)
{
    TEST_ASSERT_EQUAL(INT8_MIN, feed("cap \"abc\r"));
    TEST_ASSERT_EQUAL(0, CapCalls);
    assertOutput("cap \"abc\nError, unterminated string argument\n" CLI_PROMPT);
}

void test_unknown_command(void)
{
    TEST_ASSERT_EQUAL(INT8_MIN, feed("foo\r"));
    assertOutput("foo\nError, unknown command: foo\n" CLI_PROMPT);
}

void test_command_name_must_match_completely(void)
{
    TEST_ASSERT_EQUAL(INT8_MIN, feed("ca\r"));
    TEST_ASSERT_EQUAL(INT8_MIN, feed("capx\r"));
    TEST_ASSERT_EQUAL(INT8_MIN, feed("cap\"x\"\r"));
    TEST_ASSERT_EQUAL(0, CapCalls);
}

void test_failing_command(void)
{
    TEST_ASSERT_EQUAL(5, feed("fail\r"));
    assertOutput("fail\nError, cmd fails: 5\n" CLI_PROMPT);
}

void test_empty_line(void)
{
    TEST_ASSERT_EQUAL(0, feed("\r"));
    assertOutput("\n" CLI_PROMPT);
}

void test_whitespace_only_line(void)
{
    TEST_ASSERT_EQUAL(0, feed("   \r"));
    TEST_ASSERT_EQUAL(0, CapCalls);
    assertOutput("\n" CLI_PROMPT);
}

void test_leading_whitespace_is_ignored(void)
{
    TEST_ASSERT_EQUAL(0, feed("  cap  a\r"));
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertArgs({"a"});
    assertOutput("cap  a\nok" CLI_PROMPT);
}

void test_leading_whitespace_after_backspace_is_ignored(void)
{
    feed("x\b cap\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
}

void test_crlf(void)
{
    feed("cap a\r\ncap b\r\n");
    TEST_ASSERT_EQUAL(2, CapCalls);
    assertArgs({"b"});
    assertOutput("cap a\nok" CLI_PROMPT "cap b\nok" CLI_PROMPT);
}

void test_backspace(void)
{
    feed("caq\bp\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertOutput("caq" DEL "p\nok" CLI_PROMPT);

    feed("caq\x7fp\r");
    TEST_ASSERT_EQUAL(2, CapCalls);
}

void test_backspace_on_empty_line_rings(void)
{
    feed("\b");
    assertOutput("\a");
}

void test_buffer_overflow_rings(void)
{
    std::string input(CLI_COMMANDSIZ + 4, 'x');
    std::string stored(CLI_COMMANDSIZ - 1, 'x');

    feed(input.c_str());
    assertOutput(stored + std::string(5, '\a'));

    feed("\r");
    assertOutput("\nError, unknown command: " + stored + "\n" CLI_PROMPT);
}

void test_ctrl_l_clears_the_screen(void)
{
    feed("ca");
    Io.clear();
    feed("\f");
    assertOutput(CLRSCR CLI_PROMPT "ca");

    feed("p\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
}

void test_ctrl_k_clears_the_line(void)
{
    feed("abc");
    Io.clear();
    feed("\v");
    assertOutput(CLRLINE CLI_PROMPT);

    feed("cap\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
}

void test_csi_sequences_are_consumed(void)
{
    /* right, left and delete key */
    feed("ca\033[C\033[D\033[3~p\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertOutput("cap\nok" CLI_PROMPT);
}

void test_nul_is_ignored(void)
{
    feed("ca");
    pCli->read('\0');
    feed("p\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertOutput("cap\nok" CLI_PROMPT);

    /* Telnet sends Enter as \r\0 */
    pCli->read('\0');
    feed("cap\r");
    TEST_ASSERT_EQUAL(2, CapCalls);
}

void test_command_does_not_see_the_second_byte_of_enter(void)
{
    Io.Input = "peek\r\n";
    while (Io.available())
        pCli->loop();
    TEST_ASSERT_EQUAL(-1, PeekedByCmd);
    TEST_ASSERT_EQUAL(0, Io.available());

    Io.Input = std::string("peek\r\0", 6);
    while (Io.available())
        pCli->loop();
    TEST_ASSERT_EQUAL(-1, PeekedByCmd);

    Io.Input = "peek\rx";
    while (Io.Input.size() > 1)
        pCli->loop();
    TEST_ASSERT_EQUAL('x', PeekedByCmd);
}

void test_command_gets_the_parent(void)
{
    feed("peek\r");
    TEST_ASSERT_EQUAL_PTR(&Io, ParentOfCmd);
}

void test_exec_wraps_the_stream(void)
{
    TestStream other;

    TEST_ASSERT_EQUAL(0, CliCommand::exec(other, "peek", nullptr, 0));
    TEST_ASSERT_EQUAL_PTR(&other, ParentOfCmd);
}

void test_exec_from_a_command_keeps_the_stream(void)
{
    CliStream *pOuter;

    feed("peek\r");
    pOuter = StreamOfCmd;

    feed("nested\r");
    TEST_ASSERT_EQUAL_PTR(pOuter, StreamOfCmd);
    TEST_ASSERT_EQUAL_PTR(&Io, ParentOfCmd);
}

void test_telnet_mode(void)
{
    pCli->setEcho(false);
    Io.clear();
    pCli->setStream(&Io, CliStream::telnet);

    /* Negotiation, then the prompt */
    assertOutput("\xff\xfb\x01\xff\xfb\x03" CLI_PROMPT);

    /* Echo is on again, the new line is sent as CR LF */
    Io.Input = std::string("cap a\r\0", 8);
    while (Io.available())
        pCli->loop();
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertArgs({"a"});
    assertOutput("cap a\r\nok" CLI_PROMPT);
}

void test_detached_stream(void)
{
    pCli->setStream(nullptr);
    TEST_ASSERT_NULL(pCli->getStream());
    TEST_ASSERT_EQUAL(0, pCli->loop());
}

void test_get_stream(void)
{
    feed("peek\r");
    TEST_ASSERT_EQUAL_PTR(StreamOfCmd, pCli->getStream());
    TEST_ASSERT_EQUAL_PTR(&Io, pCli->getStream()->parent());
}

void test_alt_key_is_ignored(void)
{
    feed("ca\033xp\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertOutput("cap\nok" CLI_PROMPT);
}

void test_esc_followed_by_control_character(void)
{
    /* Enter is processed as usual */
    TEST_ASSERT_EQUAL(0, feed("cap\033\r"));
    TEST_ASSERT_EQUAL(1, CapCalls);

    /* A second ESC starts a new escape sequence */
    feed("ca\033\033[3~p\r");
    TEST_ASSERT_EQUAL(2, CapCalls);

    feed("caq\033\bp\r");
    TEST_ASSERT_EQUAL(3, CapCalls);
}

void test_echo_off(void)
{
    pCli->setEcho(false);

    feed("caq\bp\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
    assertOutput("ok" CLI_PROMPT);

    feed("\b");
    assertOutput("\a");

    feed("foo\r");
    assertOutput("Error, unknown command: foo\n" CLI_PROMPT);
}

void test_terminal_control(void)
{
    feed("ab");
    Io.clear();

    pCli->refreshPrompt();
    assertOutput(CLI_PROMPT "ab");
    pCli->clearLine();
    assertOutput(CLRLINE);
    pCli->clearScreen();
    assertOutput(CLRSCR);
    pCli->saveCursor();
    assertOutput("\0337");
    pCli->restoreCursor();
    assertOutput("\0338");
    pCli->sendBell();
    assertOutput("\a");
}

void test_reset_discards_the_input(void)
{
    feed("ca");
    Io.clear();
    pCli->reset();
    assertOutput(CLI_PROMPT);

    TEST_ASSERT_EQUAL(INT8_MIN, feed("p\r"));
}

#if CLI_HISTORYSIZ > 0

void test_up_restores_the_last_command(void)
{
    feed("cap a b\r");
    Io.clear();

    feed(UP);
    assertOutput(CLRLINE CLI_PROMPT "cap a b");

    /* the history must not be affected by parsing the arguments */
    feed("\r");
    TEST_ASSERT_EQUAL(2, CapCalls);
    assertArgs({"a", "b"});
}

void test_history_navigation(void)
{
    feed("cap 1\r");
    feed("cap 2\r");
    Io.clear();

    feed(UP);
    assertOutput(CLRLINE CLI_PROMPT "cap 2");
    feed(UP);
    assertOutput(CLRLINE CLI_PROMPT "cap 1");
    feed(UP);
    assertOutput("\a");
    feed(DOWN);
    assertOutput(CLRLINE CLI_PROMPT "cap 2");
    feed(DOWN);
    assertOutput(CLRLINE CLI_PROMPT);
    feed(DOWN);
    assertOutput("\a");
}

void test_history_navigation_in_application_cursor_mode(void)
{
    feed("cap 1\r");
    Io.clear();

    feed("\033OA");
    assertOutput(CLRLINE CLI_PROMPT "cap 1");
    feed("\033OB");
    assertOutput(CLRLINE CLI_PROMPT);
}

void test_down_without_up_rings(void)
{
    feed("cap\r");
    Io.clear();

    feed(DOWN);
    assertOutput("\a");
}

void test_restored_command_can_be_edited(void)
{
    feed("cap 1\r");
    feed(UP "\b2\r");
    assertArgs({"2"});
}

void test_whitespace_only_line_is_not_stored(void)
{
    feed("cap\r");
    feed("  \r");
    Io.clear();

    feed(UP);
    assertOutput(CLRLINE CLI_PROMPT "cap");
}

void test_failed_command_is_stored(void)
{
    feed("foo\r");
    Io.clear();

    feed(UP);
    assertOutput(CLRLINE CLI_PROMPT "foo");
}

#else

void test_history_disabled_rings(void)
{
    feed("cap\r");
    Io.clear();

    feed(UP);
    assertOutput("\a");
    feed(DOWN);
    assertOutput("\a");
}

#endif

#if CLI_TAB_COMPLETION == 0

void test_tab_completion_disabled_rings(void)
{
    feed("ca");
    Io.clear();

    feed("\t");
    assertOutput("\a");

    feed("p\r");
    TEST_ASSERT_EQUAL(1, CapCalls);
}

#endif

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_begin_prints_the_prompt);
    RUN_TEST(test_loop_without_stream);
    RUN_TEST(test_loop_reads_the_stream);
    RUN_TEST(test_command_without_arguments);
    RUN_TEST(test_command_with_arguments);
    RUN_TEST(test_multiple_separators);
    RUN_TEST(test_string_argument);
    RUN_TEST(test_escaped_quotes_in_string);
    RUN_TEST(test_escaped_quote_at_string_start);
    RUN_TEST(test_empty_string_argument);
    RUN_TEST(test_backslash_outside_string_is_kept);
    RUN_TEST(test_quote_inside_argument_is_kept);
    RUN_TEST(test_max_arguments);
    RUN_TEST(test_too_many_arguments);
    RUN_TEST(test_unterminated_string);
    RUN_TEST(test_unknown_command);
    RUN_TEST(test_command_name_must_match_completely);
    RUN_TEST(test_failing_command);
    RUN_TEST(test_empty_line);
    RUN_TEST(test_whitespace_only_line);
    RUN_TEST(test_leading_whitespace_is_ignored);
    RUN_TEST(test_leading_whitespace_after_backspace_is_ignored);
    RUN_TEST(test_crlf);
    RUN_TEST(test_backspace);
    RUN_TEST(test_backspace_on_empty_line_rings);
    RUN_TEST(test_buffer_overflow_rings);
    RUN_TEST(test_ctrl_l_clears_the_screen);
    RUN_TEST(test_ctrl_k_clears_the_line);
    RUN_TEST(test_csi_sequences_are_consumed);
    RUN_TEST(test_nul_is_ignored);
    RUN_TEST(test_command_does_not_see_the_second_byte_of_enter);
    RUN_TEST(test_command_gets_the_parent);
    RUN_TEST(test_exec_wraps_the_stream);
    RUN_TEST(test_exec_from_a_command_keeps_the_stream);
    RUN_TEST(test_telnet_mode);
    RUN_TEST(test_detached_stream);
    RUN_TEST(test_get_stream);
    RUN_TEST(test_alt_key_is_ignored);
    RUN_TEST(test_esc_followed_by_control_character);
    RUN_TEST(test_echo_off);
    RUN_TEST(test_terminal_control);
    RUN_TEST(test_reset_discards_the_input);
#if CLI_HISTORYSIZ > 0
    RUN_TEST(test_up_restores_the_last_command);
    RUN_TEST(test_history_navigation);
    RUN_TEST(test_history_navigation_in_application_cursor_mode);
    RUN_TEST(test_down_without_up_rings);
    RUN_TEST(test_restored_command_can_be_edited);
    RUN_TEST(test_whitespace_only_line_is_not_stored);
    RUN_TEST(test_failed_command_is_stored);
#else
    RUN_TEST(test_history_disabled_rings);
#endif
#if CLI_TAB_COMPLETION == 0
    RUN_TEST(test_tab_completion_disabled_rings);
#endif
    return UNITY_END();
}
