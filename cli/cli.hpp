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

#include <Arduino.h>

#if __has_include ("cli_config.hpp")
#include "cli_config.hpp"
#endif
#include "cli/config.hpp"
#include "cli/version.h"
#include "cli/command.hpp"
#include "cli/history.hpp"

#define __STDC_LIMIT_MACROS
#include <stdint.h>

class Cli {

    public:

        /**
         * @brief Construct a new Cli object
         */
        Cli();

        /**
         * @brief Used to initialize lib cli by using the automatic generated
         * command table, see README for further infos.
         *
         * @param pIoStr Optional, the stream to use for read and write.
         * @param sortCmdTab Optional, whether to sort the command table during 
         * initialization. If set to false, the command table will be used in 
         * the order of registration, which is undefined. This is only relevant
         * if command completion is enabled, as the command table should be 
         * sorted to provide sorted coammnd suggestions. Sorting suggestions 
         * on every command completion is not ideal, so the design is based on 
         * the assumption that the command table is sorted during 
         * initialization.
         */
        void begin(Stream *pIoStr = &Serial, 
                bool sortCmdTab = CLI_CMDTAB_SORTING_DEFAULT);

        /**
         * @brief Used to configure the io stream to use.
         *
         * The io stream used per default is the Serial object.
         * This call can be used if a other stream object shall be used.
         *
         * @param pIoStr Pointer to a Stream based object.
         */
        void setStream(Stream *pIoStr);

        /**
         * @brief The function to call in loop()
         *
         * Checks if data can be read from the stream object. If there
         * is data the data will be read from the stream. Finally, if a
         * command has been detected the corresponding function will be
         * called.
         *
         * @return  Zero if no comamnd has been recognized.
         *          INT8_MIN in case of an parsing related error.
         *          The return code of the command which has been recognized,
         *          zero is expected in case of success.
         */
        int8_t loop(void);

        /**
         * @brief Handle a new incoming data byte.
         *
         * @return  Zero if no comamnd has been recognized.
         *          INT8_MIN in case of an parsing related error.
         *          The return code of the command which has been recognized,
         *          zero is expected in case of success.
         */
        int8_t read(char byte);

        /**
         * @brief Turn echo either on or off. If disabled all kind of echo by
         * this library is supressed. Inteded for interaction with a host
         * application.
         *
         * @param state
         */
        void setEcho(bool state);

        /**
         * @brief Send a bell signal to the host termainal.
         */
        void sendBell(void);

        /**
         * @brief Used to refresh the prompt including the current buffer
         * content (What the user has typed) in the host terminal.
         */
        void refreshPrompt(void);

        /**
         * @brief Used to clear the current line and move the cursor to the
         * first column.
         */
        void clearLine(void);

        /**
         * @brief Used to clear the entire screen and move the cursor to the
         * first row and column.
         */
        void clearScreen(void);

        /**
         * @brief Used to save the current terminal cursor position.
         * 
         * This is a VT100 escape sequence, the position can be restored by 
         * calling restoreCursor().
         */
        void saveCursor(void);

        /**
         * @brief Used to restore the terminal cursor to the position saved by
         * saveCursor().
         */
        void restoreCursor(void);

        /**
         * @brief Used to reset the internal state and print a clean prompt
         */
        void reset(void);

    private:

        /**
         * @brief Used to restore the previous command in the users terminal.
         *
         * @return  true in case of sucess
         *          false in case of a error.
         */
        bool restoreLastCmd(void);

        /**
         * @brief Used to restore the next command in the users terminal.
         *
         * @return  true in case of sucess
         *          false in case of a error.
         */
        bool restoreNextCmd(void);

        /**
         * @brief Used to step through the command table.
         *
         * @return  Zero if no comamnd has been recognized.
         *          INT8_MIN in case of an parsing related error.
         *          The return code of the command which has been recognized,
         *          zero is expected in case of success.
         */
        int8_t checkCmdTable(void);

        /**
         * @brief Used to check the for a specific command.
         *
         * @param pCmd      Pointer to a command defintion
         *
         * @return true     In case of a match.
         * @return false    In case of a missmatch, or parsing error.
         */
        bool checkCmd(cliCmd_t *pCmd);

        /**
         * @brief Used reset argc and argv
         */
        void argReset(void);

        /**
         * @brief Handle tab completion for commands.
         * 
         * Finds matching commands in the command table and either:
         * - Auto-completes if there's exactly one match
         * - Completes to the common prefix if there are multiple matches
         * - Shows all matches if no further completion is possible
         */
        void handleTabCompletion(void);

        /**
         * @brief Find all commands in the command table matching the current 
         * buffer content.
         * 
         * @param matches Array to store pointers to matching command names
         * @return Number of matches found
         */
        uint8_t findMatchingCommands(const char* matches[]);

        /**
         * @brief Complete a portion of a match.
         * 
         * This function updates the command buffer and prints the completed
         * portion of the match to the terminal.
         * 
         * @param match The command name to complete
         * @param len The length of the portion to complete
         * @param addSpace Whether to add a space after completion (true for
         *                 complete matches, false for partial/common prefix)
         */
        void completeMatch(const char* match, uint8_t len, bool addSpace);

        /**
         * @brief Compleate to the longest common prefix among multiple matches.
         * 
         * This function completes the command buffer to the longest common 
         * prefix among the given matches.
         * 
         * @param matches Array of matching command names
         * @param matchCount Number of matches in the array
         */
        void completeToCommonPrefix(const char* matches[], uint8_t matchCount);

        /**
         * @brief Display a list of matching commands with line wrapping.
         * 
         * @param matches Array of matching command names
         * @param matchCount Number of matches to display
         */
        void displayMatchList(const char* matches[], uint8_t matchCount);

        /**
         * @brief The stream object to use for io operations.
         */
        Stream *pStream;

        /**
         * @brief Used to represent the states of escaping.
         *
         * esc_false is used as long no escape has been detected.
         * esc_true is used if a escape character has ben detected.
         * esc_csi is used when a escape has been followed by a csi character.
         */
        enum
        {
            esc_false = 0,
            esc_true = 1,
            esc_csi = 2
        }
        EscMode;

#if CLI_HISTORYSIZ > 0
        /**
         * @brief The command history buffer instance
         */
        CliHistory History;
#endif

        /**
         * @brief The internal buffer.
         */
        char Buffer[CLI_COMMANDSIZ];

        /**
         * @brief Current write position in the internal buffer.
         */
        uint8_t BufIdx;

        /**
         * @brief The argument filed to be passed to called user functins.
         *
         * Limitation: dont want to use malloc, so define the size statically.
         * This means that the maximum number of arguments is limited, thats OK
         * for now.
         */
        char *Argv[CLI_ARGVSIZ];

        /**
         * @brief States if the particular argument is a sting or not.
         */
        bool StringArg[CLI_ARGVSIZ];

        /**
         * @brief The number of detected arguments.
         */
        uint8_t Argc;

        /**
         * @brief Pointer to a array of user commands.
         */
        cliCmd_t* pCmdTab;

        /**
         * @brief Size of the command array.
         */
        uint8_t CmdTabSiz;

        /**
         * @brief The current echo state.
         */
        bool EchoEnabled;
};
