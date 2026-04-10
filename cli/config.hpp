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

/**
 * ATTENTION: This file is the default lib cli configuration and shall serve as
 * template if a project specific configuration is needed. In this case copt
 * the file, rename it to cli_config.h and make it directly available on the
 * include path.
 */

#pragma once

#ifndef CLI_COMMANDS_MAX
/**
 * @brief The maximum number of commands which can be registered in the global
 * comamnd table, see Readme.md for more infos.
 */
#define CLI_COMMANDS_MAX            10
#endif

#ifndef CLI_COMMANDSIZ
/**
 * @brief Defines the maximum length of a command including all arguments in
 * bytes.
 */
#define CLI_COMMANDSIZ              100
#endif

#ifndef CLI_HISTORYSIZ
/**
 * @brief Defines the size of the command history ringbuffer in bytes.
 * 
 * Set to 0 to completely disable history support and save memory.
 * 
 * Must be >= CLI_COMMANDSIZ to ensure at least one full command can be stored.
 * 
 * Values between 1 and CLI_COMMANDSIZ-1 will trigger a compile warning and be 
 * forced to 0 to prevent inconsistent behavior. Othervise inconsistency may 
 * occur when short commands can be stored in the history (because they fit) 
 * while long commands cannot be stored. This leads to the decision that the 
 * history buffer shall offer at least enough space to store the longest 
 * possible command.
 */
#define CLI_HISTORYSIZ              (CLI_COMMANDSIZ * 2)
#endif

/**
 * @brief Validate CLI_HISTORYSIZ and disable if invalid
 */
#if (CLI_HISTORYSIZ > 0) && (CLI_HISTORYSIZ < CLI_COMMANDSIZ)
    #warning "CLI_HISTORYSIZ < CLI_COMMANDSIZ: History forced disabled. Use 0 to disable intentionally, or >= CLI_COMMANDSIZ to enable."
    #undef CLI_HISTORYSIZ
    #define CLI_HISTORYSIZ 0
#endif

#ifndef CLI_ARGVSIZ
/**
 * @brief Defines the maximum number of arguments.
 */
#define CLI_ARGVSIZ                 4
#endif

#ifndef CLI_PROMPT
/**
 * @brief Defines the command line prompt after a new line.
 */
#define CLI_PROMPT                  "#>"
#endif

#ifndef CLI_BUFFEREDIO
/**
 * @brief Enable if stdio is buffered to call fflush(stdout) if needed.
 */
#define CLI_BUFFEREDIO              0
#endif

/**
 * @brief Define cli_flush() based on the given CLI_BUFFEREDIO setting.
 * 
 * If the output is buffered fflush has to be called after printf's without
 * a new line termination.
 */
#if CLI_BUFFEREDIO != 0
#define cli_fflush()                pStream->flush()
#else
#define cli_fflush()
#endif
