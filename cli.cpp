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
const struct {
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

} ascii;

/**
 * @brief Defines special vt100 control sequences.
 * See https://vt100.net/docs/vt510-rm/chapter4.html for details.
 */
const struct {
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

} vt100;

Cli::Cli()
    : pStream(0),
      EscMode(esc_false),
      BufIdx(0),
      Argc(0),
      pCmdTab(0),
      CmdTabSiz(0),
      EchoEnabled(true) {
    argReset();
}

void Cli::begin(Stream *pIoStr) {
    BufIdx = 0;
    pCmdTab = CliCommand::getTable();
    CmdTabSiz = CliCommand::getCmdCnt();
    size_t dropped = CliCommand::getDropCnt();

    if (dropped != 0) {
        pIoStr->printf("WARNING: Cli Command table overflow, %d dropped!\n\n",
            dropped);
    }

    setStream(pIoStr);
}

void Cli::setStream(Stream *pIoStr) {
    pStream = pIoStr;
    reset();
}

int8_t Cli::loop(void) {
    if(pStream && pStream->available()) {
        return read(pStream->read());
    }

    return 0;
}

int8_t Cli::read(char byte) {
    int8_t ret = 0;

    /* No escape so far but ESC received */
    if ((EscMode == esc_false) && (byte == ascii.esc)) {
        EscMode = esc_true;
    }
    /* No escape so far but comand terminator received */
    else if ((EscMode == esc_false) && (byte == ascii.ret)) {
        if(EchoEnabled) {
            pStream->write(ascii.newline);
        }

        if (BufIdx != 0) {
            Buffer[BufIdx] = '\0';
        }

        ret=checkCmdTable();
    }
    /* No escape so far but now DEL or BS received */
    else if ((EscMode == esc_false) &&
             ((byte == ascii.del) || (byte == ascii.bs)))
    {
        if(BufIdx > 0) {
            BufIdx--;
            if(EchoEnabled) {
                pStream->write(vt100.del);
            }
        } else {
            sendBell();
        }
    }
    /* No escape so far but now Form feed has been received. */
    else if ((EscMode == esc_false) && (byte == ascii.ff)) {
        pStream->write(vt100.clrscr);
        refreshPrompt();
    }
    /* Escape received and now the CSI character */
    else if ((EscMode == esc_true) && (byte == ascii.csi)) {
        EscMode = esc_csi;
    }
    /* Handle a ANSI escape sequence */
    else if (EscMode == esc_csi) {
        /* See https://vt100.net/docs/vt510-rm/chapter4.html for a list thing
         * which could be done here
         */
        switch (byte) {
            case 'A':
                /* Up Key pressed */
                restoreLastCmd();
                break;

            case 'B':
                /* Down Key pressed */
                restoreNextCmd();
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
    else {
        if (BufIdx == 0 && byte == ascii.newline) {
            /* Ignore the new line of a \r\n combination */
        } else if ( BufIdx < ( CLI_COMMANDSIZ - 1) ) {
            /* The index is incremented after storing the byte, there must be
             * space for the null terminator which is added by the user by
             * pressing enter later on. therefore the check above must respect
             * this: ( BufIdx < ( CLI_COMMANDSIZ - 1) )
             */
            Buffer[BufIdx++] = byte;
            if(EchoEnabled) {
                pStream->write(byte);
            }
        } else {
            sendBell();
        }

        EscMode = esc_false;
    }

    cli_fflush();
    return ret;
}

void Cli::setEcho(bool state) {
    EchoEnabled = state;
}

void Cli::sendBell(void) {
    pStream->write(ascii.bell);
}

void Cli::refreshPrompt(void) {
    pStream->write(CLI_PROMPT);
    pStream->write(Buffer, BufIdx);
}

void Cli::clearLine(void) {
    pStream->write(vt100.clrline);
}

bool Cli::restoreLastCmd(void) {
    if (History.is_used == true) {
        if (History.seek_backward() == false) {
            goto err_out;
        }
    }

    BufIdx = (uint8_t) History.read(Buffer, sizeof(Buffer));
    if (BufIdx == 0) {
        goto err_out;
    }
    clearLine();
    refreshPrompt();
    History.is_used = true;
    return true;

    err_out:
    sendBell();
    return false;
}

bool Cli::restoreNextCmd(void) {
    if (History.is_used == true) {
        if (History.seek_forward() == false) {
            History.is_used = false;
            BufIdx = 0;
            Buffer[0] = 0;
            clearLine();
            refreshPrompt();
            return true;
        }
    } else {
        /* No history used so far, restoring the next command is not possible */
        goto err_out;
    }

    BufIdx = (uint8_t) History.read(Buffer, sizeof(Buffer));
    if (BufIdx == 0) {
        goto err_out;
    }
    clearLine();
    refreshPrompt();
    return true;

    err_out:
    sendBell();
    return false;
}

int8_t Cli::checkCmdTable(void)
{
    uint8_t i = 0;
    int8_t ret = 0;
    bool hasContent = false;

    if (BufIdx == 0) {
        goto out;
    }

    /* Skip whitespace-only commands from history */
    for (uint8_t k = 0; k < BufIdx; k++) {
        if (Buffer[k] != ascii.argsep && Buffer[k] != '\t') {
            hasContent = true;
            break;
        }
    }
    
    if (hasContent == false) {
        /* Processing a whitespace-only command does not make sense
         * Just ignore it and return to the prompt
         */
        ret = 0;
        goto out_2;
    }

    History.append(Buffer, BufIdx);
    History.is_used = false;

    for(i=0; i<CmdTabSiz; i++) {
        if (checkCmd(&pCmdTab[i])) {
            if (Argc > CLI_ARGVSIZ){
                pStream->printf("Error, to many arguments (max: %d)\n", CLI_ARGVSIZ);
                ret=INT8_MIN;
                goto out_2;
            }

            ret=pCmdTab[i].pfunc(*pStream, (const char **)Argv, Argc);
            goto out;
        }
    }

    pStream->printf("Error, unknown command: %s\n", Buffer);
    /* Setting Buffer[0] to zero prevents printing the invalid command again */
    Buffer[0] = 0;
    ret=INT8_MIN;
    goto out_2;

    out:
    if (ret != 0) {
        pStream->printf("Error, cmd fails: %d\n", ret);
    }

    out_2:
    reset();
    return ret;
}

bool Cli::checkCmd(cliCmd_t *p_cmd) {
    uint8_t i = 0;
    uint8_t j = 0;
    bool string=false;

    if(!p_cmd->name[0]) {
        /* the command is empty */
        return false;
    }

    while (p_cmd->name[i]) {
        if (p_cmd->name[i] != Buffer[i]) {
            /* the given command does not match the expected command */
            return false;
        }

        i++;
    }

    if (Buffer[i] != '\0' && Buffer[i] != ascii.argsep) {
        /* the given command is longer then the expected command */
        return false;
    }

    argReset();

    while (Buffer[i] != 0) {
        /* Handle escape sequences within strings */
        if (Buffer[i] == '\\' && Buffer[i+1] != 0 && string) {
            /* Remove the backslash by shifting remaining chars */
            j = i;
            while (Buffer[j] != 0) {
                Buffer[j] = Buffer[j+1];
                j++;
            }
            /* The escaped character is now at position i
             * Don't increment i, let the normal flow handle the next char
             */
        } else if (Buffer[i] == ascii.stresc) {
            if (string) {
                /* End of string */
                string = false;
                Buffer[i] = 0;
            } else {
                /* This is a starting quote, but not at the beginning of an argument
                 * Ignore and treat it as part of the argument
                 */
            }
        } else if ((Buffer[i] == ascii.argsep) && (string == false)) {
            /* Assumtion: A new argument starts after the argument separator */
            while (Buffer[i] == ascii.argsep) {
                /* Consume all argument separators to find the start of the argument */
                Buffer[i++] = 0;
            }

            if (Buffer[i] == 0) {
                /* EOB reached, assumtion above was wrong, nothing left to parse, return */
                return true;
            }

            if (Argc == CLI_ARGVSIZ) {
                pStream->printf("Error, to many arguments (max: %d)\n", CLI_ARGVSIZ);
                return false;
            }

            if (Buffer[i] == ascii.stresc){
                string = true;
                Buffer[i++] = 0;
                StringArg[Argc] = true;
            }

            Argv[Argc] = &Buffer[i];
            Argc++;
        }

        i++;
    }

    if (string) {
        /* Unterminated string detected */
        pStream->printf("Error, unterminated string argument\n");
        return false;
    }

    return true;
}

void Cli::argReset(void) {
    Argc=0;
    memset(Argv, 0, sizeof(Argv));

    for(uint8_t i = 0; i < CLI_ARGVSIZ; i++) {
        StringArg[i] = false;
    }
}

void Cli::reset(void) {
    BufIdx = 0;
    EscMode = esc_false;
    History.is_used = false;
    refreshPrompt();
    cli_fflush();
}
