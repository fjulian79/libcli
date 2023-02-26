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

#include <stdio.h>
#include <string.h>

#include <Arduino.h>
#include "cli/cli.hpp"

/**
 * If the output is buffered fflush has to be called after printf's without
 * a new line termination.
 */
#if CLI_BUFFEREDIO != 0

#define cli_fflush()                pStream->flush()

#else

#define cli_fflush()

#endif

/**
 * @brief Defines special characters which are used in this context.
 */
const struct 
{
    /**
     * @brief Definition of the character used to sepperate 
     */
    const char argsep = ' ';
    
    /**
     * @brief Defines the sequence to echo to trigger ther terminal bell.
     */
    const char bell = '\a';

    /**
     * @brief Definition of the backspace character.
     */
    const char bs = '\b';
    
    /**
     * @brief Definition of the form feed character. Sent in case of ctrl-L.
     */
    const char ff = 0x0c;

    /**
     * @brief Defnition on the csi control sequence character.
     */
    const char csi = '[';
    
    /**
     * @brief Definition of the delete character.
     */
    const char del = 0x7f;

    /**
     * @brief Definition of the escape character.
     */    
    const char esc = '\033';

    /**
     * @brief Defines the sequence to echo for a new line.
     */
    const char newline = '\n';
    
    /**
     * @brief Defintion of the character used to terminate a line.
     */
    const char ret = '\r';

    /**
     * @brief Character which is used to mark the begin and the end of a 
     * string which shall be recognized as singe argument althow it contains
     * the character used to sperate arguments.
     */    
    const char stresc = '"';

}ascii;

/**
 * @brief Defines special vt100 control sequences.
 * See https://vt100.net/docs/vt510-rm/chapter4.html for details.
 */
const struct 
{
    /**
     * @brief Used for echo a single delete to the terminal.
     * Backspace + CSI Ps K, PS = 0
     */
    const char del[6] = "\b\033[0K";

    /**
     * @brief Used for echo a delete line to the terminal.
     * Carriage Return + CSI Ps K, PS = 2
     */
    const char clrline[6] = "\r\033[2K";

    /**
     * @brief Used to clear the screen and rest the cursor to 1:1
     * ED (Top to bottom) + CUP (line 1, column 1)
     */
    const char clrscr[11] = "\033[2J\033[1;1H";

}vt100;

Cli::Cli() :
     pStream(0)
   , EscMode(esc_false)
   , BufIdx(0)
   , Argc(0)
   , pCmdTab(0)
   , CmdTabSiz(0)
   , EchoEnabled(true)
{ 
    argReset();
}

void Cli::setStream(Stream *pIoStr)
{
    pStream = pIoStr;
    reset();
}

int8_t Cli::loop(void)
{
    if(pStream->available())
    {
        return read(pStream->read());
    } 

    return 0;
}

int8_t Cli::read(char byte)
{
    int8_t ret = 0;

    /* No escape so far but ESC received */
    if ((EscMode == esc_false) && (byte == ascii.esc))
    {
        EscMode = esc_true;
    }
    /* No escape so far but comand terminator received */
    else if ((EscMode == esc_false) && (byte == ascii.ret))
    {
        echo(ascii.newline);
        
        if (BufIdx != 0) 
        {
            Buffer[BufIdx] = '\0';
        }

        ret=checkCmdTable();
    }
    /* No escape so far but now DEL or BS received */
    else if ((EscMode == esc_false) && 
             ((byte == ascii.del) || (byte == ascii.bs)))
    {
        if(BufIdx > 0)
        {
            BufIdx--;
            echo(vt100.del);
        }
        else
        {
            sendBell();
        }   
    }
    /* No escape so far but now Form feed has been received. */
    else if ((EscMode == esc_false) && (byte == ascii.ff))
    {
        echo(vt100.clrscr);
        refreshPrompt();
    }
    /* Escape received and now the CSI character */
    else if ((EscMode == esc_true) && (byte == ascii.csi))
    {
        EscMode = esc_csi;
    }
    /* Handle a ANSI escape sequence */
    else if (EscMode == esc_csi)
    {
        /* See https://vt100.net/docs/vt510-rm/chapter4.html for a list thing 
         * which could be done here 
         */
        switch (byte)
        {
            case 'A':
                /* Up Key pressed */
                restoreLastCmd();
                break;
            
            case 'B':
                /* Down Key pressed */
                break;
            
            case 'C':
                /* Right Key pressed */
                break;
            
            case 'D':
                /* Left Key pressed */
                break;

            default:
                break;
        }

        EscMode = esc_false;
    }
    /* All special cases processed treat it like data */
    else 
    {
        if (BufIdx < CLI_COMMANDSIZ)
        {
            Buffer[BufIdx++] = byte;
            echo(byte);
        }
        else
        {
            sendBell();
        }
        
        EscMode = esc_false;
    }

    cli_fflush();
    
    return ret;
}

void Cli::setEcho(bool state)
{
    EchoEnabled = state;
}

void Cli::sendBell(void)
{
    echo(ascii.bell);
}

void Cli::refreshPrompt(void)
{
    if(EchoEnabled == false)
    {
        return;
    }

    echo(CLI_PROMPT);
    pStream->write(Buffer, BufIdx);
}

void Cli::clearLine(void)
{
    echo(vt100.clrline);
}

bool Cli::restoreLastCmd(void)
{
    uint8_t i = 0;

    if (BufIdx != 0)
    {
        return false;
    }

    /* the buffer still starts with the last command */
    BufIdx = strlen(Buffer);

    if (BufIdx == 0)
    {
        return false;
    }

    for(i = 0; i < Argc; i++)
    {
        if(StringArg[i] == false)
        {
            BufIdx += sprintf(&Buffer[BufIdx],"%c%s", 
                    ascii.argsep, Argv[i]);
        }
        else
        {
            BufIdx += sprintf(&Buffer[BufIdx],"%c\"%s\"", 
                    ascii.argsep, Argv[i]);
        }
    }

    echo(Buffer);
    
    return true;
}

int8_t Cli::checkCmdTable(void)
{
    uint8_t i = 0;
    int8_t ret = 0;

    if (BufIdx == 0)
    {
        goto out;
    }

    for(i=0; i<CmdTabSiz; i++)
    {
        if (checkCmd(&pCmdTab[i]))
        {
            if (Argc > CLI_ARGVSIZ)
            {
                pStream->printf("Error, to many arguments (max: %d)\n", CLI_ARGVSIZ);
                ret=INT8_MIN;
                goto out_2;
            }

            ret=pCmdTab[i].p_cmd_func((const char **)Argv, Argc);
            goto out;
        }
    }

    pStream->printf("Error, unknown command: %s\n", Buffer);
    /* Setting Buffer[0] to zero prevents printing the invalid command again */
    Buffer[0] = 0;
    ret=INT8_MIN;
    goto out_2;

    out:
    if (ret != 0)
    {   
        pStream->printf("Error, cmd fails: %d\n", ret);
    }

    out_2:
    reset();
    return ret;
}

bool Cli::checkCmd(cliCmd_t *p_cmd)
{
    uint8_t i = 0;
    bool string=false;

    if(!p_cmd->cmd_text[0])
    {
        return false;
    }

    while (p_cmd->cmd_text[i])
    {
        if (p_cmd->cmd_text[i] != Buffer[i])
        {
            return false;
        }
            
        i++;
    }

    if (Buffer[i] != '\0' && Buffer[i] != ascii.argsep)
    {
        return false;
    }
        
    argReset();

    while (Buffer[i] != 0)
    {
        if (Buffer[i] == ascii.stresc)
        {
            string = false;
            Buffer[i] = 0;
        }
        else if ((Buffer[i] == ascii.argsep) && (string == false))
        {
            while (Buffer[i] == ascii.argsep)
            {
                Buffer[i++] = 0;
            }

            if (Buffer[i] == 0)
            {   
                break;
            }

            if (Argc == CLI_ARGVSIZ)
            {
                /* violate the limitation to signalize the error */
                Argc++;
                break;
            }

            if (Buffer[i] == ascii.stresc)
            {
                string = true;
                Buffer[i++] = 0;
                StringArg[Argc] = true;
            }

            Argv[Argc] = &Buffer[i];
            Argc++;
        }

        i++;
    }

    return true;
}

void Cli::argReset(void)
{
    Argc=0;
    memset(Argv, 0, sizeof(Argv));

    for(uint8_t i = 0; i < CLI_ARGVSIZ; i++)
    {
        StringArg[i] = false;
    }
}

void Cli::reset(void)
{
    BufIdx = 0;
    EscMode = esc_false;
    refreshPrompt();
    cli_fflush();
}
