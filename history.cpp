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

#include "cli/history.hpp"
#include "string.h"

CliHistory::CliHistory(void) {
    is_used = false;
    memset(Buffer, 0, sizeof(Buffer));
    clear();
}

CliHistory::~CliHistory(void) {
}

bool CliHistory::append(const char *str, size_t len) {
    if (str == nullptr || len == 0 ||
        len > sizeof(Buffer) - 1 ||
        str[len] != '\0') {
        return false;
    }

    /* Check if the new string is identical to the last added entry to avoid
     * duplicate consecutive entries in the history */
    if (pLast != 0) {
        char *pTemp = pLast;
        const char *pStr = str;
        bool identical = true;

        /* Compare character by character, handling wrap-around */
        for (size_t i = 0; i < len; i++) {
            if (*pTemp != *pStr) {
                identical = false;
                break;
            }
            increment_position(pTemp);
            pStr++;
        }

        /* Check if we reached the null terminator at pTemp */
        if (identical && *pTemp == '\0') {
            /* Strings are identical, no need to append */
            reset_navigation();
            return true;
        }
    }

    while (get_free_space() < len + 1) {
        /* Just for safety, This should not happen, as get_free_space should
         * return sizeof(Buffer) in this case */
        if (pTail == 0) {
            return false;
        }

        /* find the next null terminator and set the tail to the next byte as
         * this is the start of the next line */
        while (*pTail != '\0') {
            increment_position(pTail);
        }
        increment_position(pTail);

        /* if tail and head are now equal, the buffer is empty. Reset to a clean
         * state as safety measure */
        if (pTail == pHead) {
            clear();
        }
    }

    /* Read and Tail can be set now as needed and the new line can be written.
     * By incrementing the length by one, we also write the null terminator of
     * the line, without the need to handle it as special case.
     */
    pLast = pHead;
    pTail = pTail == 0 ? pHead : pTail;
    len++;
    while (len > 0) {
        size_t space_to_end = sizeof(Buffer) - (pHead - Buffer);
        size_t to_write = (len < space_to_end) ? len : space_to_end;

        memcpy(pHead, str, to_write);
        increment_position(pHead, to_write);
        str += to_write;
        len -= to_write;
    }

    reset_navigation();
    return true;
}

bool CliHistory::seek_backward(void) {
    /* Currently the read pointer points to line n, what we want is to set it to
     * line n-1 if it exists */
    char *pTemp = pRead;

    if (pTemp == 0 || pTemp == pTail) {
        /* If the read pointer is equal to the tail it allready points to the
         * oldest line, it is zero, the buffer is empty, however can't move
         * backward */
        return false;
    }

    /* below the current position is the null terminator of line n-1, pass it
     * directly to start looking for the null terminator of line n-2 or the tail
     * if line n-1 is the oldest line in the buffer */
    decrement_position(pTemp, 2);

    while (*pTemp != '\0') {
        if (pTemp == pTail) {
            /* The previous line is the oldest line in the buffer and we don't
             * need to seek further, we are done */
            pRead = pTail;
            return true;
        }
        decrement_position(pTemp);
    }

    /* Found the null terminator of the line n-2, set the read pointer to
     * the start of the line n-1 which is the next byte in forward direction */
    increment_position(pTemp);
    pRead = pTemp;
    return true;
}

bool CliHistory::seek_forward(void) {
    /* Currently the read pointer points to line n, what we want is to set it to
     * line n+1 if it exists */
    char *pTemp = pRead;

    if (pTemp == 0) {
        /* the buffer is empty */
        return false;
    }

    /* Move the read pointer forward until the null terminator of line n has
     * been found. one step further is the start of line n+1 */
    while (*pTemp != '\0') {
        increment_position(pTemp);
        if (pTemp == pHead) {
            /* Just for safety, this should not happen because there should
             * be a null terminator at the end of every line and the head is
             * one position past this null-terminator */
            return false;
        }
    }
    increment_position(pTemp);

    if (pTemp == pHead) {
        /* Found the end of a line n but what comes next is the head, so there
         * is no next line in the buffer, abort */
        return false;
    }

    /* Update the read pointer only in case of success */
    pRead = pTemp;
    return true;
}

size_t CliHistory::read(char *line, size_t len) {
    size_t numRead = 0;
    char *pTemp = pRead;

    if (pRead == 0 || line == nullptr || len == 0) {
        return 0;
    }

    while(*pTemp != '\0') {
        if (len == 1) {
            /* The provided buffer is too small to hold the line including the
             * null terminator, abort. */
            return 0;
        }

        *line++ = *pTemp;
        increment_position(pTemp);
        len--;
        numRead++;
    }

    *line = '\0';
    return numRead;
}

size_t CliHistory::get_free_space(void) {
    if (pTail == 0) {
        /* The buffer is empty, all space is free. */
        return sizeof(Buffer);
    } else if (pHead > pTail) {
        /* The head is ahead of the tail, the free space is the space after the
         * head plus the space before the tail. */
        return (sizeof(Buffer) - (pHead - Buffer)) + (pTail - Buffer);
    } else if (pHead < pTail) {
        /* The tail is ahead of the head, the free space is the space between
         * the head and the tail. */
        return pTail - pHead;
    } else {
        /* pTail != 0 but equal to pHead, the buffer is full */
        return 0;
    }
}

void CliHistory::clear(void) {
    /* a memset zero is not necessary, just reset the pointers */
    pHead = Buffer;
    pRead = 0;
    pTail = 0;
    pLast = 0;
}