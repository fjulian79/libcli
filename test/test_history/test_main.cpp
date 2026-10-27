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
#include <deque>
#include <string>
#include <vector>

#include "cli/history.hpp"

#define HISTSIZ         ((size_t) CLI_HISTORYSIZ)

static CliHistory *pHist;

static bool append(const std::string &str)
{
    return pHist->append(str.c_str(), str.length());
}

static std::string readLine(void)
{
    char buf[HISTSIZ];
    size_t len = pHist->read(buf, sizeof(buf));

    return std::string(buf, len);
}

/**
 * Returns all lines from the oldest to the newest one, leaves the read pointer
 * at the newest line.
 */
static std::vector<std::string> lines(void)
{
    std::vector<std::string> result;

    if (readLine().empty())
        return result;

    while (pHist->seek_backward());

    do {
        result.push_back(readLine());
    } while (pHist->seek_forward());

    return result;
}

static void assertLines(const std::vector<std::string> &expected)
{
    std::vector<std::string> actual = lines();

    TEST_ASSERT_EQUAL(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); i++)
        TEST_ASSERT_EQUAL_STRING(expected[i].c_str(), actual[i].c_str());
}

void setUp(void)
{
    pHist = new CliHistory();
}

void tearDown(void)
{
    delete pHist;
}

void test_initially_empty(void)
{
    char buf[8] = "x";

    TEST_ASSERT_EQUAL(0, pHist->read(buf, sizeof(buf)));
    TEST_ASSERT_FALSE(pHist->seek_backward());
    TEST_ASSERT_FALSE(pHist->seek_forward());
    TEST_ASSERT_EQUAL(HISTSIZ, pHist->get_free_space());
    TEST_ASSERT_FALSE(pHist->is_used);
}

void test_append_rejects_invalid_input(void)
{
    TEST_ASSERT_FALSE(pHist->append(nullptr, 3));
    TEST_ASSERT_FALSE(pHist->append("", 0));
    /* len must point to the null terminator */
    TEST_ASSERT_FALSE(pHist->append("abc", 2));
    /* there must be room for the null terminator */
    TEST_ASSERT_FALSE(append(std::string(HISTSIZ, 'a')));
    TEST_ASSERT_EQUAL(HISTSIZ, pHist->get_free_space());
}

void test_append_and_read(void)
{
    TEST_ASSERT_TRUE(append("abc"));
    TEST_ASSERT_EQUAL_STRING("abc", readLine().c_str());
    TEST_ASSERT_EQUAL(HISTSIZ - 4, pHist->get_free_space());
}

void test_read_needs_room_for_the_terminator(void)
{
    char buf[8];

    append("abc");
    TEST_ASSERT_EQUAL(0, pHist->read(buf, 3));
    TEST_ASSERT_EQUAL(3, pHist->read(buf, 4));
    TEST_ASSERT_EQUAL_STRING("abc", buf);
    TEST_ASSERT_EQUAL(0, pHist->read(nullptr, sizeof(buf)));
    TEST_ASSERT_EQUAL(0, pHist->read(buf, 0));
}

void test_seek(void)
{
    append("a");
    append("bb");
    append("ccc");

    TEST_ASSERT_EQUAL_STRING("ccc", readLine().c_str());
    TEST_ASSERT_FALSE(pHist->seek_forward());
    TEST_ASSERT_TRUE(pHist->seek_backward());
    TEST_ASSERT_EQUAL_STRING("bb", readLine().c_str());
    TEST_ASSERT_TRUE(pHist->seek_backward());
    TEST_ASSERT_EQUAL_STRING("a", readLine().c_str());
    TEST_ASSERT_FALSE(pHist->seek_backward());
    TEST_ASSERT_EQUAL_STRING("a", readLine().c_str());
    TEST_ASSERT_TRUE(pHist->seek_forward());
    TEST_ASSERT_EQUAL_STRING("bb", readLine().c_str());
    TEST_ASSERT_TRUE(pHist->seek_forward());
    TEST_ASSERT_EQUAL_STRING("ccc", readLine().c_str());
    TEST_ASSERT_FALSE(pHist->seek_forward());
}

void test_append_resets_navigation(void)
{
    append("a");
    append("b");
    pHist->seek_backward();
    pHist->is_used = true;

    append("c");
    TEST_ASSERT_EQUAL_STRING("c", readLine().c_str());
    TEST_ASSERT_FALSE(pHist->is_used);
}

void test_consecutive_duplicates_are_dropped(void)
{
    append("abc");
    TEST_ASSERT_TRUE(append("abc"));
    assertLines({"abc"});
    TEST_ASSERT_EQUAL(HISTSIZ - 4, pHist->get_free_space());

    append("x");
    append("abc");
    assertLines({"abc", "x", "abc"});
}

void test_duplicate_resets_navigation(void)
{
    append("a");
    append("b");
    pHist->seek_backward();
    pHist->is_used = true;

    append("b");
    TEST_ASSERT_EQUAL_STRING("b", readLine().c_str());
    TEST_ASSERT_FALSE(pHist->is_used);
}

void test_prefix_is_no_duplicate(void)
{
    append("abc");
    append("ab");
    append("abc");
    assertLines({"abc", "ab", "abc"});
}

void test_longest_line_fills_the_buffer(void)
{
    std::string longest(HISTSIZ - 1, 'a');

    TEST_ASSERT_TRUE(append(longest));
    TEST_ASSERT_EQUAL(0, pHist->get_free_space());
    assertLines({longest});

    TEST_ASSERT_TRUE(append("b"));
    assertLines({"b"});
    TEST_ASSERT_EQUAL(HISTSIZ - 2, pHist->get_free_space());
}

void test_oldest_lines_are_evicted(void)
{
    /* 13 bytes per line, does not divide the buffer size, so the lines wrap
     * around the end of the buffer at different positions */
    std::deque<std::string> expected;
    size_t used = 0;
    char line[16];

    for (int i = 0; i < 100; i++) {
        snprintf(line, sizeof(line), "line-%07d", i);
        TEST_ASSERT_TRUE(append(line));

        expected.push_back(line);
        used += strlen(line) + 1;
        while (used > HISTSIZ) {
            used -= expected.front().length() + 1;
            expected.pop_front();
        }

        assertLines(std::vector<std::string>(expected.begin(), expected.end()));
        TEST_ASSERT_EQUAL(HISTSIZ - used, pHist->get_free_space());
    }
}

void test_clear(void)
{
    append("a");
    append("b");
    pHist->clear();

    TEST_ASSERT_EQUAL(0, lines().size());
    TEST_ASSERT_EQUAL(HISTSIZ, pHist->get_free_space());

    append("c");
    assertLines({"c"});
}

/**
 * Compares the history with a simple model for random lines of random length,
 * including consecutive duplicates.
 */
void test_matches_model(void)
{
    std::deque<std::string> model;
    size_t used = 0;
    uint32_t seed = 12345;

    for (int i = 0; i < 3000; i++) {
        std::string str;

        seed = seed * 1103515245 + 12345;
        if (!model.empty() && (seed >> 16) % 5 == 0) {
            str = model.back();
        } else {
            seed = seed * 1103515245 + 12345;
            size_t len = 1 + (seed >> 16) % (HISTSIZ / 3);

            for (size_t j = 0; j < len; j++) {
                seed = seed * 1103515245 + 12345;
                str += (char) ('a' + (seed >> 16) % 3);
            }
        }

        TEST_ASSERT_TRUE(append(str));

        if (model.empty() || model.back() != str) {
            while (HISTSIZ - used < str.length() + 1) {
                used -= model.front().length() + 1;
                model.pop_front();
            }
            model.push_back(str);
            used += str.length() + 1;
        }

        TEST_ASSERT_EQUAL_STRING(str.c_str(), readLine().c_str());
        TEST_ASSERT_EQUAL(HISTSIZ - used, pHist->get_free_space());
        assertLines(std::vector<std::string>(model.begin(), model.end()));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_initially_empty);
    RUN_TEST(test_append_rejects_invalid_input);
    RUN_TEST(test_append_and_read);
    RUN_TEST(test_read_needs_room_for_the_terminator);
    RUN_TEST(test_seek);
    RUN_TEST(test_append_resets_navigation);
    RUN_TEST(test_consecutive_duplicates_are_dropped);
    RUN_TEST(test_duplicate_resets_navigation);
    RUN_TEST(test_prefix_is_no_duplicate);
    RUN_TEST(test_longest_line_fills_the_buffer);
    RUN_TEST(test_oldest_lines_are_evicted);
    RUN_TEST(test_clear);
    RUN_TEST(test_matches_model);
    return UNITY_END();
}
