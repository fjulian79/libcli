/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2023 Julian Friedrich
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

#include <Arduino.h>

#if __has_include ("cli_config.hpp")
#include "cli_config.hpp"
#endif

#include "cli/config.hpp"

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
    int8_t (*p_cmd_func)(Stream& ioStream, const char *argv[], uint8_t argc);

}cliCmd_t;

/**
 * @brief Used to declare or define a cli command function. 
 */
#define CLI_COMMAND_FUNCTION(_name)                                            \
                                                                               \
    int8_t cmd_ ## _name (Stream& ioStream, const char *argv[], uint8_t argc)

#define CLI_COMMAND_REGISTER(_name)                                            \
                                                                               \
    __attribute__((used, section(".cli_cmd_section")))                         \
    extern const cliCmd_t cli_cmd_##_name = {#_name, cmd_ ## _name}

#define CLI_COMMAND(_name)                                                     \
                                    \
    CLI_COMMAND_FUNCTION(_name);    \
    CLI_COMMAND_REGISTER(_name);    \
    CLI_COMMAND_FUNCTION(_name) 

extern const cliCmd_t __start_cli_cmd_section[];
extern const cliCmd_t __stop_cli_cmd_section[];

class Cli
{
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
         */
        void begin(Stream *pIoStr = &Serial)
        {
            uint8_t size = __stop_cli_cmd_section - __start_cli_cmd_section;
            begin((cliCmd_t *)__start_cli_cmd_section, size, pIoStr);
        }

        /**
         * @brief Used to initialize lib cli with a given command table
         * 
         * @param pTable The comamnd table to use.   
         * @param pIoStr Optional, the stream to use for read and write.
         */
        template <size_t size>
        void begin(cliCmd_t (&cmdTab)[size], Stream *pIoStr = &Serial)
        {
            begin(cmdTab, size, pIoStr);
        }

        /**
         * @brief Used to initialize lib cli with a given command table in C 
         * style
         * 
         * @param pTable The comamnd table to use.   
         * @param size The size of the command table.
         * @param pIoStr Optional, the stream to use for read and write.
         */
        void begin(cliCmd_t *cmdTab, uint8_t size, Stream *pIoStr = &Serial)
        {
            extern cliCmd_t cli_cmd_ver;

            BufIdx = 0;
            pCmdTab = cmdTab;
            CmdTabSiz = size;
            Serial.printf("cmdTab: %p, size: %d\n", pCmdTab, CmdTabSiz);
            Serial.printf("cmd[0]: %s, %p\n", pCmdTab[0].cmd_text, pCmdTab[0].p_cmd_func);
            setStream(pIoStr);
            reset();
        }

        /**
         * @brief Used to set the command table.
         * 
         * @param pTable The comamnd table to use   
         */
        template <size_t size>
        void setCmdTab(cliCmd_t (&cmdTab)[size])
        {
            pCmdTab = cmdTab;
            CmdTabSiz = size;
        }

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
         * @brief Used to reset the internal state and print a clean prompt
         */
        void reset(void);

    private:

        /**
         * @brief Used to restore the last valid command in the users terminal.
         * 
         * @return  true in case of sucess
         *          false in case of a error.
         */
        bool restoreLastCmd(void);

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
         * @return true     In case of a positive match. 
         * @return false    In case of a negative match.
         */
        bool checkCmd(cliCmd_t *pCmd);

        /**
         * @brief Used reset argc and argv
         */
        void argReset(void);

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
         * @brief Ehe current echo state.
         */
        bool EchoEnabled;
};

#endif /* CLI_H_ */
