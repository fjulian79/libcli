/*
 * cli.h
 *
 *  Created on: Jan 29, 2015
 *      Author: julian
 *
 *  Used to provide a tiny shell/command line interface.
 *  Commands are defined by the cli_cmd_t structure, a array
 *  of this structure shall be passed to the constructor.
 */

#ifndef CLI_H_
#define CLI_H_

#include "cli_config.h"

#define __STDC_LIMIT_MACROS
#include <stdint.h>

/**
 * If the output is buffered fflusch has to be called after printf's without
 * a new line termination.
 */
#if defined CLI_BUFFERED_IO
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
     * Function pointer to be called if the command has been detected 
     */
    int8_t (*p_cmd_func)(void *args);

}cli_cmd_t;


class Cli
{
    public:

        Cli();

        void init(cli_cmd_t* p_table, uint8_t size);

        /**
         * Used to install a new command table.
         */
        void set_cmd_table(cli_cmd_t* p_table, uint8_t size);

        /**
         * Handle a new incoming data byte.
         */
        int8_t proc_byte(char _data);

        /**
         * Used to get a pointer to the next argument appended the command.
         * arguments have to separated by a blank, e.g.:
         *
         *   test_cmd abc def 123
         *   |        |   |   +--- arg 3
         *   |        |   +------- arg 2
         *   |        +----------- arg 1
         *   +-------------------- command
         *
         * Returns a void pointer to the position where the argument starts if
         * there is one.
         */
        char* get_parg(void);

        /**
         * Used to convert the current argument into a 16 bit signed integer.
         */
        bool arg2int16(int16_t *p_val);

        /**
         * Used to convert the current argument into a 16 bit unsigned integer.
         */
        bool arg2uint16(uint16_t *p_val);

        /**
         * Used to wait for the user pressing any key.
         */
        bool wait_anykey(void);

        /**
         * Wait for user confirmation.
         */
        bool wait_userconfirm(void);

    private:

        /**
         * Internal constant, buffer size.
         */
        static const uint8_t max_cmd_len = 100;

        /**
         * Backspace definition
         */
        static const char del = CLI_BACKSPACE;

        /**
         * Command escape, used to escape the cmd_term symbol.
         */
        static const char cmd_esc = CLI_ESC;

        /**
         * Used to separate commands from arguments and arguments from other
         * arguments.
         */
        static const char arg_sep = CLI_ARG_SEP;

        /**
         *
         */
        static const char cmd_term = CLI_TERM;

        /**
         * Used to step through the command table.
         */
        int8_t check_cmd_table(void);

        /**
         * Used to check the for a specific command.
         */
        bool check_cmd(cli_cmd_t *pcmd);

        /**
         * Used to reset the internal state.
         */
        void reset(void);

        /**
         * True when the next byte has been escaped.
         */
        bool esc;

        /**
         * Command buffer.
         */
        char buffer[max_cmd_len];

        /**
         * Current position in the buffer.
         */
        uint8_t buf_idx;

        /**
         * Index of the last detected argument.
         * If zero no argument has been detected.
         */
        uint8_t arg_idx;

        /**
         * Pointer to a array of commands.
         */
        cli_cmd_t* p_cmd_tab;

        /**
         * Size of the command array.
         */
        uint8_t cmd_tab_siz;

        /**
         * Pointer to the last accepted command.
         */
        int8_t (*p_last_cmd)(void *args);
};

#endif /* CLI_H_ */
