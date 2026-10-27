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
 * ATTENTION: The command table is global and static, the tests depend on each
 * other and must be run in the given order.
 */

#include <unity.h>
#include <Arduino.h>

#include "cli/cli.hpp"

static TestStream Io;

CLI_COMMAND(zeta)
{
    (void) ioStream;
    (void) argv;
    (void) argc;
    return 3;
}

CLI_COMMAND(alpha)
{
    for (uint8_t i = 0; i < argc; i++)
        ioStream.print(argv[i]);

    return argc;
}

CLI_COMMAND(mid)
{
    (void) ioStream;
    (void) argv;
    (void) argc;
    return 0;
}

void setUp(void)
{
    Io.clear();
}

void tearDown(void) {}

void test_registered_in_definition_order(void)
{
    cliCmd_t *pTab = CliCommand::getTable();

    TEST_ASSERT_EQUAL(3, CliCommand::getCmdCnt());
    TEST_ASSERT_EQUAL(0, CliCommand::getDropCnt());
    TEST_ASSERT_EQUAL_STRING("zeta", pTab[0].name);
    TEST_ASSERT_EQUAL_STRING("alpha", pTab[1].name);
    TEST_ASSERT_EQUAL_STRING("mid", pTab[2].name);
    TEST_ASSERT_EQUAL_PTR(cmd_zeta, pTab[0].pfunc);
}

void test_get_cmd(void)
{
    TEST_ASSERT_EQUAL_PTR(cmd_alpha, CliCommand::getCmd("alpha"));
    TEST_ASSERT_EQUAL_PTR(cmd_mid, CliCommand::getCmd("mid"));
    TEST_ASSERT_NULL(CliCommand::getCmd("alp"));
    TEST_ASSERT_NULL(CliCommand::getCmd("alphax"));
    TEST_ASSERT_NULL(CliCommand::getCmd(""));
}

void test_exec(void)
{
    const char *argv[] = {"x", "y"};

    TEST_ASSERT_EQUAL(2, CliCommand::exec(Io, "alpha", argv, 2));
    TEST_ASSERT_EQUAL_STRING("xy", Io.Output.c_str());
    TEST_ASSERT_EQUAL(3, CliCommand::exec(Io, "zeta", nullptr, 0));
    TEST_ASSERT_EQUAL(-1, CliCommand::exec(Io, "unknown", nullptr, 0));
}

void test_sort_table(void)
{
    cliCmd_t *pTab = CliCommand::getTable();

    CliCommand::sortTable();
    TEST_ASSERT_EQUAL_STRING("alpha", pTab[0].name);
    TEST_ASSERT_EQUAL_STRING("mid", pTab[1].name);
    TEST_ASSERT_EQUAL_STRING("zeta", pTab[2].name);
    TEST_ASSERT_EQUAL_PTR(cmd_alpha, pTab[0].pfunc);
    TEST_ASSERT_EQUAL_PTR(cmd_zeta, CliCommand::getCmd("zeta"));
}

void test_overflow_is_counted(void)
{
    while (CliCommand::getCmdCnt() < CLI_COMMANDS_MAX)
        CliCommand("fill", cmd_mid);

    TEST_ASSERT_EQUAL(0, CliCommand::getDropCnt());

    CliCommand("dropped", cmd_mid);
    CliCommand("dropped", cmd_mid);
    TEST_ASSERT_EQUAL(CLI_COMMANDS_MAX, CliCommand::getCmdCnt());
    TEST_ASSERT_EQUAL(2, CliCommand::getDropCnt());
    TEST_ASSERT_NULL(CliCommand::getCmd("dropped"));
}

void test_begin_warns_about_dropped_commands(void)
{
    Cli cli;

    cli.begin(&Io);
    TEST_ASSERT_EQUAL_STRING(
        "WARNING: Cli Command table overflow, 2 dropped!\n\n" CLI_PROMPT,
        Io.Output.c_str());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_registered_in_definition_order);
    RUN_TEST(test_get_cmd);
    RUN_TEST(test_exec);
    RUN_TEST(test_sort_table);
    RUN_TEST(test_overflow_is_counted);
    RUN_TEST(test_begin_warns_about_dropped_commands);
    return UNITY_END();
}
