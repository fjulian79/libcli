/*
 * cli-config.h
 *
 *  Created on: Jun 25, 2017
 *      Author: julian
 *
 *  Provides the project specific configuration for libcli.
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
 * Command termination, used to mark the end of a command.
 */
#define CLI_TERM                    '\r'

#ifdef __cplusplus
}
#endif

#endif /* CLI_CONFIG_H_ */
