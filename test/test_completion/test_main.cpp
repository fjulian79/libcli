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

static TestStream Io;

static Cli *pCli;

static std::string LastCmd;

/**
 * Defines a command which just records its name in LastCmd.
 */
#define TEST_COMMAND(_name)                                         \
                                                                    \
    CLI_COMMAND(_name)                                              \
    {                                                               \
        (void) ioStream;                                            \
        (void) argv;                                                \
        (void) argc;                                                \
        LastCmd = #_name;                                           \
        return 0;                                                   \
    }

/* Registered unsorted on purpose, begin() sorts them */
TEST_COMMAND(status)
TEST_COMMAND(led_on)
TEST_COMMAND(help)
TEST_COMMAND(led)
TEST_COMMAND(set)
TEST_COMMAND(led_off)

/* 30 characters each, only two of them fit into one line */
TEST_COMMAND(wrap_around_the_terminal_cmd_c)
TEST_COMMAND(wrap_around_the_terminal_cmd_a)
TEST_COMMAND(wrap_around_the_terminal_cmd_b)

static void feed(const char *str)
{
    while (*str)
        pCli->read(*str++);
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
    LastCmd.clear();
}

void tearDown(void)
{
    delete pCli;
}

void test_tab_on_empty_line_rings(void)
{
    feed("\t");
    assertOutput("\a");
}

void test_no_match_rings(void)
{
    feed("x");
    Io.clear();

    feed("\t");
    assertOutput("\a");
}

void test_single_match_is_completed(void)
{
    feed("he");
    Io.clear();

    feed("\t");
    assertOutput("lp ");

    feed("\r");
    TEST_ASSERT_EQUAL_STRING("help", LastCmd.c_str());
}

void test_leading_whitespace_is_ignored(void)
{
    feed("  he");
    Io.clear();

    feed("\t");
    assertOutput("lp ");
}

void test_single_match_among_similar_ones(void)
{
    feed("led_of");
    Io.clear();

    feed("\t");
    assertOutput("f ");

    feed("\r");
    TEST_ASSERT_EQUAL_STRING("led_off", LastCmd.c_str());
}

void test_common_prefix_is_completed_and_matches_listed(void)
{
    feed("l");
    Io.clear();

    feed("\t");
    assertOutput("ed\nled       led_off   led_on\n" CLI_PROMPT "led");

    feed("\r");
    TEST_ASSERT_EQUAL_STRING("led", LastCmd.c_str());
}

void test_matches_listed_without_common_prefix(void)
{
    feed("s");
    Io.clear();

    feed("\t");
    assertOutput("\nset      status\n" CLI_PROMPT "s");

    feed("\vled_o");
    Io.clear();
    feed("\t");
    assertOutput("\nled_off   led_on\n" CLI_PROMPT "led_o");
}

void test_matches_listed_column_wise(void)
{
    const std::string prefix = "wrap_around_the_terminal_cmd_";

    feed("w");
    Io.clear();

    feed("\t");
    assertOutput(prefix.substr(1) + "\n"
        + prefix + "a   " + prefix + "c\n"
        + prefix + "b\n"
        + CLI_PROMPT + prefix);
}

void test_tab_after_arguments_rings(void)
{
    feed("led on");
    Io.clear();

    feed("\t");
    assertOutput("\a");
}

void test_echo_off(void)
{
    pCli->setEcho(false);

    feed("he\t");
    assertOutput("");
    feed("\r");
    TEST_ASSERT_EQUAL_STRING("help", LastCmd.c_str());
    Io.clear();

    /* The list of matches is no echo, it is always printed */
    feed("s\t");
    assertOutput("\nset      status\n" CLI_PROMPT "s");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tab_on_empty_line_rings);
    RUN_TEST(test_no_match_rings);
    RUN_TEST(test_single_match_is_completed);
    RUN_TEST(test_leading_whitespace_is_ignored);
    RUN_TEST(test_single_match_among_similar_ones);
    RUN_TEST(test_common_prefix_is_completed_and_matches_listed);
    RUN_TEST(test_matches_listed_without_common_prefix);
    RUN_TEST(test_matches_listed_column_wise);
    RUN_TEST(test_tab_after_arguments_rings);
    RUN_TEST(test_echo_off);
    return UNITY_END();
}
