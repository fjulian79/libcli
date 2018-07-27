/*
 * cli.cpp
 *
 *  Created on: Jan 29, 2015
 *      Author: julian
 */

#include <stdio.h>
#include <string.h>

#include "cli/cli.h"

Cli::Cli() :
     Esc(false)
   , BufIdx(0)
   , Argc(0)
   , pCmdTab(0)
   , CmdTabSiz(0)
   , pLastCmd(0)
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

    if (!Esc && data == CmdEsc)
    {
        Esc=true;
    }
    else if (Esc || data != CmdTerm)
    {
        if (data==Del)
            BufIdx = BufIdx>0 ? BufIdx-1 : 0;
        else
            Buffer[BufIdx++] = data;
        Esc=false;
    }
    else if (pCmdTab)
    {
        if (BufIdx)
            Buffer[BufIdx] = '\0';
        ret=checkCmdTable();
    }

    if (BufIdx == MaxCmdLen-1)
        reset();

    return ret;
}

int8_t Cli::checkCmdTable(void)
{
    uint8_t i;
    int8_t ret=0;

    if (BufIdx == 0 && pLastCmd != 0)
    {
        Argc=0;

#ifdef CLI_PRINTLASTCMD
        printf("%s\n", Buffer);
#endif

        ret=pLastCmd(Argv, Argc);
        goto out;
    }
    else if (BufIdx == 0)
    {
        goto out;
    }

    for(i=0; i<CmdTabSiz; i++)
    {
        if (checkCmd(&pCmdTab[i]))
        {
            if (Argc > ArgvSize)
            {
                printf("Error, to mutch arguments (max: %d)\n", ArgvSize);
                pLastCmd=0;
                ret=INT8_MIN;
                goto out_2;
            }

            pLastCmd=pCmdTab[i].p_cmd_func;
            ret=pLastCmd(Argv, Argc);
            goto out;
        }
    }

    printf("Error, unknown command: %s\n", Buffer);
    pLastCmd=0;
    ret=INT8_MIN;
    goto out_2;

    out:
    if (ret != 0)
    {   
        printf("Error, cmd fails: %d\n", ret);
        pLastCmd = 0;
    }

    out_2:
    reset();
    return ret;
}

bool Cli::checkCmd(cliCmd_t *p_cmd)
{
    uint8_t i = 0;
    bool string=false;

    while (p_cmd->cmd_text[i])
    {
        if (p_cmd->cmd_text[i] != Buffer[i])
            return false;
        i++;
    }

    if (Buffer[i] != '\0' && Buffer[i] != ArgSep)
        return false;
    
    argReset();

    while (Buffer[i] != 0)
    {
        if (Buffer[i] == CmdEsc)
            Esc = true;
        else if (Esc)
            continue;
        else if (Buffer[i] == StringEsc)
        {
            string = false;
            Buffer[i] = 0;
        }
        else if ((Buffer[i] == ArgSep) && (string == false))
        {
            while (Buffer[i] == ArgSep)
                i++;

            if (Buffer[i] == CmdTerm || Buffer[i] == '\0')
            {   
                break;
            }

            if (Argc == ArgvSize)
            {
                /* violate the limitation to signalize the error */
                Argc++;
                break;
            }

            if (Buffer[i] == StringEsc)
            {
                string = true;
                i++;
                printf("(%d) 1 string is %d\n", i , string);
            }

            Buffer[i-1] = 0;
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

    while (   pArg[pos] != ArgSep
           && pArg[pos] != CmdTerm
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
   memset(Argv,0, ArgvSize);
}

void Cli::reset(void)
{
    argReset();
    BufIdx=0;
    Esc=false;
    printf(CLI_PROMPT);
    cli_fflush();
}
