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

#pragma once

/**
 * @brief Defines special characters which are used in this context.
 */
const struct {
    /**
     * @brief Definition of the character used to sepperate arguments.
     */
    const char argsep = ' ';

    /**
     * @brief Defines the sequence to echo to trigger the terminal bell.
     */
    const char bell = '\a';

    /**
     * @brief Definition of the backspace character.
     */
    const char bs = '\b';

    /**
     * @brief Definition of the form feed character. Sent in case of ctrl-L.
     */
    const char ff = 0x0c;

    /**
     * @brief Defnition on the csi control sequence character.
     */
    const char csi = '[';

    /**
     * @brief Definition of the delete character.
     */
    const char del = 0x7f;

    /**
     * @brief Definition of the escape character.
     */
    const char esc = '\033';

    /**
     * @brief Defines the sequence to echo for a new line.
     */
    const char newline = '\n';

    /**
     * @brief Defintion of the character used to terminate a line.
     */
    const char ret = '\r';

    /**
     * @brief Definition of the ss3 control sequence character, used by
     * terminals in application cursor mode for the cursor keys.
     */
    const char ss3 = 'O';

    /**
     * @brief Character which is used to mark the begin and the end of a
     * string which shall be recognized as singe argument althow it contains
     * the character used to sperate arguments.
     */
    const char stresc = '"';

    /**
     * @brief Definition of the tab character.
     */
    const char tab = '\t';

    /**
     * @brief Definition of the vertical tab character. Sent in case of ctrl-K.
     */
    const char vt = '\v';

} ascii;
