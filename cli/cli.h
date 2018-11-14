/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2015 Julian Friedrich
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

#include "cli_config.h"

#define __STDC_LIMIT_MACROS
#include <stdint.h>

/**
 * If the output is buffered fflush has to be called after printf's without
 * a new line termination.
 */
#ifdef CLI_BUFFERED_IO

#define cli_fflush()        fflush(stdout)

#else

#define cli_fflush()

#endif

/**
 * Used to describe a single command, build a array of this struct to define
 * all supported commands.
 */
typedef struct
{
    /** 
     * Command text as entered by the user. 
     */
    const char *cmd_text;

    /**
     * Function pointer to be called if the command has been detected.
     */
    int8_t (*p_cmd_func)(char *argv[], uint8_t argc);

}cliCmd_t;

class Cli
{
    public:

        Cli();

        void init(cliCmd_t* pTable, uint8_t size);

        /**
         * Used to install a new command table.
         */
        void setCmdTable(cliCmd_t* pTable, uint8_t size);

        /**
         * Handle a new incoming data byte.
         */
        int8_t procByte(char _data);

        /**
         * To parse a unsigned value of the given size.
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
         * To parse a signed value of the given size.
         * 
         * @param pArg      The argument string.
         * @param pData     The variable to write to.
         * @param siz       The size of the variable.
         *
         * @return true     In case of success.
         * @return false    In case of a error.
         */
        bool toSigned(char *pArg, void *pData, size_t siz);

        /**
         * Used to wait for the user pressing any key.
         * 
         * TBD
         */
        //bool wait_anykey(void);

        /**
         * Wait for user confirmation.
         * 
         * TBD
         */
        //bool wait_userconfirm(void);

    private:

        /**
         * Internal constant, buffer size.
         */
        static const uint8_t MaxCmdLen = 100;

        /**
         * 
         */
        static const uint8_t ArgvSize = CLI_ARGV_SIZ;

        /**
         * Backspace definition
         */
        static const char Del = CLI_BACKSPACE;

        /**
         * Command escape, used to escape the cmd_term symbol.
         */
        static const char CmdEsc = CLI_ESC;

        /**
         * Used to separate commands from arguments and arguments from other
         * arguments.
         */
        static const char ArgSep = CLI_ARG_SEP;

        /**
         * Caracter which is used to mark the begin and the end of a string which
         * shall be recognized as singe argument althow it contains CLI_ARG_SEP
         */
        static const char StringEsc = CLI_STRING_ESC;

        /**
         * Character which is used to terminate a line.
         */
        static const char CmdTerm = CLI_TERM;

        /**
         * Used to step through the command table.
         */
        int8_t checkCmdTable(void);

        /**
         * Used to check the for a specific command.
         */
        bool checkCmd(cliCmd_t *pCmd);

        /**
         * Generic parser fpr integer values
         * 
         * @param pArg      Start of the argument string
         * @param pVal      The variable to write to
         * @param allowHex  set to true if hex values shall be possible.
         * 
         * @return true     in case of usccess
         * @return false    in case of any error
         */
        bool parseInt(char *pArg, uint64_t *pVal, bool allowHex);

        /**
         * Used reset argc and argv
         */
        void argReset(void);

        /**
         * Used to reset the internal state.
         */
        void reset(void);

        /**
         * True when the next byte has been escaped.
         */
        bool Esc;

        /**
         * Command buffer.
         */
        char Buffer[MaxCmdLen];

        /**
         * Current position in the buffer.
         */
        uint8_t BufIdx;

        /**
         * The argument filed to pass to functins.
         * 
         * Limitation: dont want to use malloc, de define the size statically.
         * This means that the maximum number of arguments is limmited.
         * Thats Ok for now
         */
        char *Argv[ArgvSize];

        /**
         * The argument counter.
         */
        uint8_t Argc;

        /**
         * Pointer to a array of commands.
         */
        cliCmd_t* pCmdTab;

        /**
         * Size of the command array.
         */
        uint8_t CmdTabSiz;

        /**
         * Pointer to the last accepted command.
         */
        int8_t (*pLastCmd)(char *argv[], uint8_t argc);
};

#endif /* CLI_H_ */
