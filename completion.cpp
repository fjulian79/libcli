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

#include <string.h>
#include <Arduino.h>
#include "cli/cli.hpp"
#include "cli/ascii.hpp"

#if CLI_TAB_COMPLETION != 0

uint8_t Cli::findMatchingCommands(const char* matches[]) {
    uint8_t matchCount = 0;

    for (uint8_t i = 0; i < CmdTabSiz; i++) {
        if (strncmp(pCmdTab[i].name, Buffer, BufIdx) == 0) {
            matches[matchCount++] = pCmdTab[i].name;
        }
    }

    return matchCount;
}

void Cli::completeMatch(const char* match, uint8_t len, bool addSpace) {
    while(BufIdx < len && BufIdx < (CLI_COMMANDSIZ - 1)) {
        Buffer[BufIdx] = match[BufIdx];
        if (EchoEnabled) {
            pStream->write(match[BufIdx]);
        }
        BufIdx++;
    }
    
    /* Add a space after the completed command if requested and there's room */
    if (addSpace && BufIdx < (CLI_COMMANDSIZ - 1)) {
        Buffer[BufIdx++] = ascii.argsep;
        if (EchoEnabled) {
            pStream->write(ascii.argsep);
        }
    }

    cli_fflush();
}

void Cli::completeToCommonPrefix(const char* matches[], uint8_t matchCount) {
    uint8_t commonLen = BufIdx;

    /* Find length of common prefix among all matches */
    while (true) {
        char refChar = matches[0][commonLen];

        if (refChar == '\0') {
            /* We've reached the end of the first match, so the common prefix 
             * can't be longer than this. */
            break; 
        }

        for (uint8_t i = 1; i < matchCount; i++) {
            char testChar = matches[i][commonLen];
            if (testChar == '\0' || testChar != refChar) {
                /* Either we've reached the end of this match, or it differs 
                 * from the first match at this position. Either way, we've 
                 * found a difference and are done. */
                goto out;
            }
        }

        commonLen++;
    }

    out:
    /* Only complete if there is actually a longer common prefix to add.
     * commonLen can equal BufIdx in two cases:
     * 1. Matches differ immediately after current input (e.g., "dummy_" with 
     *    "dummy_2", "dummy_3" differs at position 6)
     * 2. One match is exactly as long as the current input (e.g., "led" when
     *    a command named "led" exists alongside "led_on", "led_off")
     * In both cases, there's nothing to complete. */
    if (commonLen > BufIdx) {
        /* using matches[0] is perfectly fine as all matches share the 
         * common prefix and it must be part of this match */
        completeMatch(matches[0], commonLen, false);
    }
}

void Cli::displayMatchList(const char* matches[], uint8_t matchCount) {
    /* Display matches in aligned columns, similar to bash completion.
     * Uses column-wise layout (filling down first, then right) rather than
     * row-wise. This makes it easier to scan sorted lists vertically for
     * similar command names. */
    
    const uint8_t spacing = 3;  /* Minimum spaces between columns */
    uint8_t maxLen = 0;
    uint8_t colWidth = 0;
    uint8_t numCols = 0;
    uint8_t numRows = 0;

    /* Find the longest match to determine column width */
    for (uint8_t i = 0; i < matchCount; i++) {
        uint8_t len = strlen(matches[i]);
        if (len > maxLen) {
            maxLen = len;
        }
    }
    
    /* Calculate column width, number of columns and number of rows, but ensure 
     * at least one column and one row */
    colWidth = maxLen + spacing;
    numCols = CLI_TERMINAL_WIDTH / colWidth;
    if (numCols == 0) {
        numCols = 1;
    }
    numRows = (matchCount + numCols - 1) / numCols;
    
    /* Output matches column-wise: iterate rows, then columns */
    pStream->write(ascii.newline);
    for (uint8_t row = 0; row < numRows; row++) {
        for (uint8_t col = 0; col < numCols; col++) {
            /* Calculate index: column-wise means idx = row + col * numRows */
            uint8_t idx = row + col * numRows;
            
            /* Check if this cell has a valid match. The last row may be 
             * incomplete */
            if (idx < matchCount) {
                pStream->printf("%s", matches[idx]); 
                /* Add padding to align columns, except for last column */
                if (col < numCols - 1 && idx + numRows < matchCount) {
                    uint8_t len = strlen(matches[idx]);
                    uint8_t padding = colWidth - len;
                    for (uint8_t j = 0; j < padding; j++) {
                        pStream->write(' ');
                    }
                }
            }
        }
        pStream->write(ascii.newline);
    }
    
    refreshPrompt();
}

void Cli::handleTabCompletion(void) {
    const char* matches[CLI_COMMANDS_MAX];
    uint8_t matchCount = 0;
    uint8_t commonLen = 0;

    if (BufIdx == 0) {
        /* No input yet, nothing to complete. What could be done is to show all 
         * available commands. Maybe a future enhancement. */
        sendBell();
        return;
    }

    /* Terminate the buffer for string comparisons done by findMatchingCommands.
     * This is ok as any further input of the user or completion will overwrite 
     * this null terminator. */
    Buffer[BufIdx] = '\0';
    matchCount = findMatchingCommands(matches);

    if (matchCount == 0) {
        /* No matches found */
        sendBell();
    } else if (matchCount == 1) {
        /* Exactly one match, complete it and add a space */
        completeMatch(matches[0], strlen(matches[0]), true);
    } else {
        /* Multiple matches, complete to the longest common prefix */
        completeToCommonPrefix(matches, matchCount);                
        /* There is more than one match possible, show the list */
        displayMatchList(matches, matchCount);
    }
}

#else /* CLI_TAB_COMPLETION != 0 */

/**
 * Tab completion disabled - provide stub implementations that do nothing
 * or just send a bell. This keeps the core code (cli.cpp) free of #if's.
 */

void Cli::clearMatchLines(void) {
    /* Do nothing when tab completion is disabled */
}

void Cli::handleTabCompletion(void) {
    /* Send bell to indicate tab is not supported */
    sendBell();
}

#endif /* CLI_TAB_COMPLETION != 0 */
