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

#ifndef CLI_CONFIG_H_
#define CLI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable if stdio is buffered.
 * Then flush will be called when needed.
 */
#define CLI_BUFFERED_IO

/**
 * Enable if printing the last command is desired in case of repetition.
 */
#define CLI_PRINTLASTCMD

/**
 * Defines the command line prompt after a new line.
 */
#define CLI_PROMPT                  "#>"

/**
 * Backspace definition
 */
#define CLI_BACKSPACE               0x7F

/**
 * Command escape, used to escape the cmd_term symbol.
 */
#define CLI_ESC                     '\\'

/**
 * Separates commands from arguments and arguments from other
 * arguments.
 */
#define CLI_ARG_SEP                 ' '

/**
 * Defines the caracter used for strings with spaces
 */
#define CLI_STRING_ESC              '"'

/**
 * Defines the maximum number of arguments.
 */
#define CLI_ARGV_SIZ                4

/**
 * Command termination, used to mark the end of a command.
 */
#define CLI_TERM                    '\r'

#ifdef __cplusplus
}
#endif

#endif /* CLI_CONFIG_H_ */
