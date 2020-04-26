/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2020 Julian Friedrich
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
 * You can file issues at https://github.com/fjulian79/libcli/issues
 */

#ifndef CLI_H_
#define CLI_H_

#if __has_include ("cli_config.hpp")
#include "cli_config.hpp"
#else
#include "cli/cli_config_template.hpp"
#endif

#define __STDC_LIMIT_MACROS
#include <stdint.h>

/**
 * @brief Used to describe a single command.
 * 
 * Build a array of this struct to define all supported commands.
 */
typedef struct
{
    /** 
     * @brief Command text as entered by the user. 
     */
    const char *cmd_text;

    /**
     * @brief Function pointer to be called if the command has been detected.
     */
    int8_t (*p_cmd_func)(char *argv[], uint8_t argc);

}cliCmd_t;

class Cli
{
    public:

        /**
         * @brief Construct a new Cli object
         */
        Cli();

        /**
         * @brief 
         * 
         * @param pTable 
         * @param size 
         */
        void init(cliCmd_t* pTable, uint8_t size);

        /**
         * @brief Used to install a new command table.
         */
        void setCmdTable(cliCmd_t* pTable, uint8_t size);

        /**
         * @brief Handle a new incoming data byte.
         */
        int8_t procByte(char _data);

        /**
         * @brief To parse a unsigned value of the given size.
         * 
         * @param pArg      The argument string.
         * @param pData     The variable to write to.
         * @param siz       The size of the variable.
         *
         * @return true     In case of success.
         * @return false    In case of a error.
         */
        bool toUnsigned(char *pArg, void *pData, size_t siz);

        /**
         * @brief To parse a signed value of the given size.
         * 
         * @param pArg      The argument string.
         * @param pData     The variable to write to.
         * @param siz       The size of the variable.
         *
         * @return true     In case of success.
         * @return false    In case of a error.
         */
        bool toSigned(char *pArg, void *pData, size_t siz);

    private:

        /**
         * @brief Used to restore the last valid command in the users terminal.
         */
        bool restoreLastCmd(void);

        /**
         * @brief Used to step through the command table.
         */
        int8_t checkCmdTable(void);

        /**
         * @brief Used to check the for a specific command.
         * 
         * @param pCmd      Pointer to a command defintion
         * 
         * @return true     In case of a positive match. 
         * @return false    In case of a negative match.
         */
        bool checkCmd(cliCmd_t *pCmd);

        /**
         * @brief Generic parser fpr integer values
         * 
         * @param pArg      Start of the argument string
         * @param pVal      The variable to write to
         * @param allowHex  set to true if hex values shall be possible.
         * 
         * @return true     in case of success
         * @return false    in case of any error
         */
        bool parseInt(char *pArg, uint64_t *pVal, bool allowHex);

        /**
         * @brief Used reset argc and argv
         */
        void argReset(void);

        /**
         * @brief Used to reset the internal state.
         */
        void reset(void);

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
};

#endif /* CLI_H_ */
