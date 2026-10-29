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
 * @brief Defines special vt100 control sequences.
 * See https://vt100.net/docs/vt510-rm/chapter4.html for details.
 */
const struct {
    /**
     * @brief Used for echo a single delete to the terminal.
     * Backspace + CSI Ps K, PS = 0
     */
    const char del[6] = "\b\033[0K";

    /**
     * @brief Used for echo a delete line to the terminal.
     * CUB (cursor backward) Pn = 999 + EL (erase in line) Ps = 2
     *
     * No carriage return on purpose, some terminals map a received CR to LF,
     * e.g. the PlatformIO monitor with monitor_eol = CR. The cursor is moved
     * to the first column by CUB instead, the terminal stops at the margin.
     */
    const char clrline[11] = "\033[999D\033[2K";

    /**
     * @brief Used to clear the screen and rest the cursor to 1:1
     * ED (Top to bottom) + CUP (line 1, column 1)
     */
    const char clrscr[11] = "\033[2J\033[1;1H";

    /**
     * @brief Save cursor position (DECSC)
     */
    const char savecur[3] = "\0337";

    /**
     * @brief Restore cursor position (DECRC)
     */
    const char restorecur[3] = "\0338";

} vt100;