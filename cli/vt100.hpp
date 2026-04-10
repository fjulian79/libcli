/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2026 Julian Friedrich
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
     * Carriage Return + CSI Ps K, PS = 2
     */
    const char clrline[6] = "\r\033[2K";

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