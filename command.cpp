/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2025 Julian Friedrich
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

#include "cli/command.hpp"

cliCmd_t Command::CmdTab[CLI_COMMANDS_MAX];

size_t Command::CmdCnt = 0;

size_t Command::OvCnt = 0;

Command::Command(const char* name, CmdFuncPtr function) 
{
    if (CmdCnt < CLI_COMMANDS_MAX) 
    {
        CmdTab[CmdCnt++] = {name, function};
    } 
    else 
    {
        OvCnt++;
    }
}

CmdFuncPtr Command::find(const char* name)
{
    for (size_t i = 0; i < CmdCnt; i++) 
    {
        if (strcmp(name, CmdTab[i].name) == 0) 
        {
            return CmdTab[i].pfunc;
        }
    }

    return nullptr;
}

void Command::list() 
{
    Serial.printf("Registered commands (%d):\n", CmdCnt);
    for (size_t i = 0; i < CmdCnt; i++) 
    {
        Serial.printf("  %s\n", CmdTab[i].name);
    }
}