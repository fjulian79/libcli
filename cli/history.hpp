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

#include <stdint.h>
#include <stddef.h>

#if __has_include ("cli_config.hpp")
#include "cli_config.hpp"
#endif
#include "cli/config.hpp"

/**
 * @brief A class to store lines of text in a fixed-size circular buffer.
 *
 * This calls is designed to act as a shell history for libcli and is not
 * intended for general use. It provides the following functionality:
 * - Store lines of text in a fixed-size circular buffer.
 * - Maximum memory efficiency
 *   - only storing the lines and a null terminator, no additional metadata.
 *   - Stored lines may warp around the end of the buffer.
 * - Every write operation will ...
 *   - Set the read pointer to the new line.
 *   - Automatically free space as needed by removing the old lines.
 * - Seek forward and backward through the stored lines.
 * - Read the line at the current read pointer position into a provided buffer.
 */
class CliHistory {

    public:
        CliHistory(void);
        ~CliHistory(void);

        /**
         * @brief Store the given string in the internal buffer.
         *
         * This function will ...
         *   - automatically free space as needed by removing the oldest line in
         *     the buffer until the new data can be stored.
         *   - set the the read pointer to the new line which is then the newest
         *     line in the buffer.
         *
         * @param str   Pointer to the string to store, must be null-terminated.
         * @param len   The length of the string to store excluding the null
         *              terminator.
         *
         * @return  true in case of sucess.
         *          false in case of a error, e.g. if the data to store is
         *          larger than the internal buffer.
         */
        bool append(const char *str, size_t len);

        /**
         * @brief Move the read pointer to the previous (older) line.
         *
         * @return  true if the read pointer was successfully moved
         *          false if there is no previous line
         */
        bool seek_backward(void);

        /**
         * @brief Move the read pointer to the next (more recent) line.
         *
         * @return  true if the read pointer was successfully moved
         *          false if there is no next line
         */
        bool seek_forward(void);

        /**
         * @brief Read the line at the current read pointer position into the
         * provided buffer.
         *
         * This function will copy the line at the current read pointer
         * position into the provided buffer while taking care of the potential
         * wrap-around of the internal buffer. The line is copied including the
         * null terminator, so the provided buffer must be large enough to hold
         * the line including the null terminator. If the provided buffer is
         * too small, the function will return 0, but the given buffer will be
         * modified.
         *
         * @param line  Pointer to the buffer where the line should be copied,
         *              must be large enough to hold the line including the null
         *              terminator.
         * @param len   The maximum length of the given buffer.
         *
         * @return  The number of bytes read, excluding the null terminator.
         *          Returns 0 if there is no line to read or if the provided
         *          buffer is too small.
         */
        size_t read(char *line, size_t max_len);

        /**
         * @brief Get the amount of free space available in the buffer.
         *
         * @return The number of bytes available for writing.
         */
        size_t get_free_space(void);

        /**
         * @brief Clear the internal buffer, all data will be lost.
         */
        void clear(void);

        /**
         * @brief Used to indicate if the history is currently used for
         * restoring commands. It is provided by this class as it belongs to
         * it's context but has to be used and managed outisde of this class.
         */
        bool is_used;

    private:

        /**
         * @brief Increment the given pointer while respecting the wrap around.
         *
         * @param ptr           The pointer to increment
         * @param increment     The number of bytes to increment, default is 1
         */
        inline void increment_position(char *&ptr, size_t increment = 1) {
            const size_t size = sizeof(Buffer);
            const size_t index = static_cast<size_t>(ptr - Buffer);
            ptr = Buffer + ((index + increment) % size);
        }

        /**
         * @brief Decrement the given pointer while respecting the wrap around.
         *
         * @param ptr           The pointer to decrement
         * @param decrement     The number of bytes to decrement, default is 1
         */
        inline void decrement_position(char *&ptr, size_t decrement = 1) {
            const size_t size = sizeof(Buffer);
            const size_t index = static_cast<size_t>(ptr - Buffer);
            decrement %= size;
            ptr = Buffer + ((index + size - decrement) % size);
        }

        /**
         * @brief Reset the navigation state to the most recent entry.
         * 
         * Sets pRead to pLast and clears the is_used flag.
         */
        inline void reset_navigation(void) {
            pRead = pLast;
            is_used = false;
        }

        /**
         * @brief Internal buffer to store the lines.
         */
        char Buffer [CLI_HISTORYSIZ];

        /**
         * @brief Pointer to the position in the buffer where the next data will
         * be written. Is always set to a valid position.
         */
        char *pHead;

        /**
         * @brief Pointer to the position in the buffer where the last written
         * line starts as this is first to read. Note that this is not the tail
         * of the buffer. If it is zero, the buffer is considered empty.
         */
        char *pRead;
        /* data */

        /**
         * @brief Pointer to the position in the buffer where the oldest line
         * starts. This is the tail of the buffer. If it is zero, the buffer is
         * considered empty.
         */
        char *pTail;

        /**
         * @brief Pointer to the start of the most recently added line in the
         * buffer. Used to detect and prevent duplicate consecutive entries.
         * If it is zero, the buffer is considered empty.
         */
        char *pLast;
};