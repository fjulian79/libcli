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

#ifndef CLI_CONFIG_H_
#define CLI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines the maximum length of a command including all arguments in 
 * bytes.
 */
#define CLI_COMMANDSIZ              100

/**
 * @brief Defines the maximum number of arguments.
 */
#define CLI_ARGVSIZ                 4

/**
 * @brief Defines the command line prompt after a new line.
 */
#define CLI_PROMPT                  "#>"

/**
 * @brief Enable if stdio is buffered to call fflush(stdout) if needed.
 */
#define CLI_BUFFEREDIO

/**
 * @brief Used to enable serial echo.
 * 
 */
#define CLI_ECHO

/**
 * @brief Used to enable printgin the last command when invoked.
 */
#define CLI_PRINTLASTCMD


#ifdef __cplusplus
}
#endif

#endif /* CLI_CONFIG_H_ */
