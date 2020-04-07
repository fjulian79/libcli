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

#include <stdio.h>
#include <string.h>

#include "cli/cli.hpp"

/**
 * If the output is buffered fflush has to be called after printf's without
 * a new line termination.
 */
#ifdef CLI_BUFFEREDIO

#define cli_fflush()                fflush(stdout)

#else

#define cli_fflush()

#endif

/**
 * @brief Use for single character terminal echos.
 */
#define echo(_val)                      putchar(_val)

/**
 * @brief Used for echo a delete to the terminal.
 */
#define echo_del()                      printf("\b\033[K");

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

Cli::Cli() :
    EscMode(esc_false)
   , BufIdx(0)
   , Argc(0)
   , pCmdTab(0)
   , CmdTabSiz(0)
{ 
    argReset();
}

void Cli::init(cliCmd_t *pTable, uint8_t size)
{
    setCmdTable(pTable, size);
    reset();
}

void Cli::setCmdTable(cliCmd_t *pTable, uint8_t size)
{
    pCmdTab = pTable;
    CmdTabSiz = size;
}

int8_t Cli::procByte(char data)
{
    int8_t ret=0;

    /* No escape so far but ESC received */
    if ((EscMode == esc_false) && (data == ascii.esc))
    {
        EscMode = esc_true;
    }
    /* No escape so far but comand terminator received */
    else if ((EscMode == esc_false) && (data == ascii.ret))
    {
        echo(ascii.newline);
        
        if (BufIdx != 0) 
        {
            Buffer[BufIdx] = '\0';
        }

        ret=checkCmdTable();
    }
    /* No escape so fat but now DEL received */
    else if ((EscMode == esc_false) && (data == ascii.del))
        {
            if(BufIdx > 0)
            {
                BufIdx--;
            echo_del();
            }
            else
            {
            echo(ascii.bell);
            }
        }
    /* Escape received and now the CSI character */
    else if ((EscMode == esc_true) && (data == ascii.csi))
    {
        EscMode = esc_csi;
    }
    /* Handle a ANSI escape sequence */
    else if (EscMode == esc_csi)
    {
        switch (data)
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
                Buffer[BufIdx++] = data;
            echo(data);
            }
            else
            {
            echo(ascii.bell);
            }
        
        EscMode = esc_false;
        }

        cli_fflush();
    
    return ret;
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

    printf("%s", Buffer);

    return true;
}

int8_t Cli::checkCmdTable(void)
{
    uint8_t i = 0;
    int8_t ret=0;

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
                printf("Error, to many arguments (max: %d)\n", CLI_ARGVSIZ);
                ret=INT8_MIN;
                goto out_2;
            }

            ret=pCmdTab[i].p_cmd_func(Argv, Argc);
            goto out;
        }
    }

    printf("Error, unknown command: %s\n", Buffer);
    /* Setting Buffer[0] to zero prevents printing the invalid command again */
    Buffer[0] = 0;
    ret=INT8_MIN;
    goto out_2;

    out:
    if (ret != 0)
    {   
        printf("Error, cmd fails: %d\n", ret);
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

bool Cli::toSigned(char *pArg, void *pData, size_t siz)
{
    bool negative=false;
    union tmpData
    {
        uint64_t u64;
        uint32_t u32[2];
        uint16_t u16[4];
        uint8_t  u8[8];
    }tmp;

    if (*pArg == '-')
    {
        negative=true;
        pArg++;
    }

    /* parse as it would be a positive value but dont accept hex valus
     * when a negitve sign has been found */
    if (!parseInt(pArg, (uint64_t*) &tmp.u64, !negative))
        return false;

    if (negative)
    {
        /* If negaive sign has been found check if the parsed value
         * is in the alowed range, abort if not. */
        if (tmp.u64 & ((uint64_t)UINT64_MAX << ((siz*8)-1)))
            return false;

        /* convert postive to negative */
        tmp.u64 = 0 - tmp.u64;
    }

    if (siz == 1)
        *((uint8_t*)pData) = tmp.u8[0];
    else if (siz == 2)
        *((uint16_t*)pData) = tmp.u16[0];
    else if (siz == 4)
        *((uint32_t*)pData) = tmp.u32[0];
    else if (siz == 8)
        *((uint64_t*)pData) = tmp.u64;
    else
        return false;

    return true;
}

bool Cli::toUnsigned(char *pArg, void *pData, size_t siz)
{
    union tmpData
    {
        uint64_t u64;
        uint32_t u32[2];
        uint16_t u16[4];
        uint8_t  u8[8];
    }tmp;

    if (!parseInt(pArg, &tmp.u64, true))
        return false;

    if (siz == 1)
        *((uint8_t*)pData) = tmp.u8[0];
    else if (siz == 2)
        *((uint16_t*)pData) = tmp.u16[0];
    else if (siz == 4)
        *((uint32_t*)pData) = tmp.u32[0];
    else if (siz == 8)
        *((uint64_t*)pData) = tmp.u64;
    else
        return false;

    return true;
}

bool Cli::parseInt(char *pArg, uint64_t *pVal, bool allowHex)
{
    bool hex=false;
    uint8_t pos=0;
    uint8_t cnt=0;

    *pVal = 0;

    while (   pArg[pos] != ascii.argsep
           && pArg[pos] != ascii.ret
           && pArg[pos] != '\0')
    {
        if (*pVal == 0 && pArg[pos] == 'x')
        {
            if(!allowHex)
                return false;

            hex=true;
            pos++;
        }

        if (!hex)
        {
            if (!(pArg[pos] >= '0' && pArg[pos] <= '9'))
                return false;

            if (((*pVal*10)+(pArg[pos]-'0')) < *pVal)
                return false;
            else
                *pVal=(*pVal*10)+(pArg[pos]-'0');
        }
        else
        {
            if (cnt > (sizeof(uint64_t)*2)-1)
                return false;
            cnt++;

            *pVal = *pVal << 4;
            if (pArg[pos] >= '0' && pArg[pos] <= '9')
                *pVal+=pArg[pos]-'0';
            else if (pArg[pos] >= 'a' && pArg[pos] <= 'f')
                *pVal+=pArg[pos]-'a'+10;
            else if (pArg[pos] >= 'A' && pArg[pos] <= 'F')
                *pVal+=pArg[pos]-'A'+10;
            else
                return false;
        }

        pos++;
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
    BufIdx=0;
    EscMode = esc_false;
    printf(CLI_PROMPT);
    cli_fflush();
}
