/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2025 Julian Friedrich
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

#ifndef CLI_COMMAND_H_
#define CLI_COMMAND_H_

#include <Arduino.h>
#if __has_include ("cli_config.hpp")
#include "cli_config.hpp"
#endif
#include "cli/config.hpp"

/**
 * @brief Generates a libcli command definition based on the given name.
 */
#define CLI_COMMAND_DEF(_name)                                      \
                                                                    \
    int8_t cmd_ ## _name (Stream& ioStream, const char *argv[], uint8_t argc)

/**
 * @brief Used to define and register a libcli command.
 */
#define CLI_COMMAND(_name)                                          \
                                                                    \
    CLI_COMMAND_DEF(_name);                                         \
    static CliCommand _name ## _registrar(#_name, cmd_ ## _name);   \
    CLI_COMMAND_DEF(_name)

/**
 * @brief Defines a libcli command function pointer.
 */
typedef int8_t (*CmdFuncPtr)(Stream& ioStream, const char *argv[], uint8_t argc);

/**
 * @brief The command structure used to store the command name and the 
 * corresponding function pointer.
 */
typedef struct
{
    /** 
     * @brief Command name as entered by the user. 
     */
    const char *name;

    /**
     * @brief Pointer to the command function.
     */
    CmdFuncPtr pfunc;

}cliCmd_t;

/**
 * @brief The command class used to register and find commands.
 */
class CliCommand 
{
    public:

        /**
         * @brief Construct a new Command object and automatically register it
         * in the global command table.
         */
        CliCommand(const char* name, CmdFuncPtr function);

        /**
         * @brief Used to get the global command table.
         */
        static cliCmd_t* getTable(void);

        /**
         * @brief Used to get the number of registered commands.
         */
        static size_t getCmdCnt(void);

        /**
         * @brief Used to get the number of commands which could not be registered.
         */
        static size_t getDropCnt(void);

        /**
         * @brief Used to find a command by its name.
         * 
         * @param name The name of the command to find.
         * 
         * @return The function pointer of the command or nullptr if not found.
         */
        static CmdFuncPtr getCmd(const char* name);

        /**
         * @brief Used to execute a command by its name.
         * 
         * @param ioStream The stream to use for io operations.
         * @param name The name of the command to execute.
         * @param argv The arguments of the command.
         * @param argc The number of arguments.
         * 
         * @return The return value of the command.
         */
        static int8_t exec(Stream& ioStream, const char* name, const char* argv[], uint8_t argc);

    private:

        /**
         * @brief The global command table.
         */
        static cliCmd_t CmdTab[CLI_COMMANDS_MAX];

        /**
         * @brief The number of registered commands.
         */
        static size_t CmdCnt;

        /**
         * @brief The number of commands which could not be registered.
         */
        static size_t DropCnt;
};

#endif /* CLI_COMMAND_H_ */