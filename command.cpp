/*
 * libcli, a simple and generic CliCommand line interface with small footprint for
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
 * This project is hosted on GitHub:
 *   https://github.com/fjulian79/libcli
 * Please feel free to file issues, open pull requests, or contribute there.
 */

#include "cli/command.hpp"

cliCmd_t CliCommand::CmdTab[CLI_COMMANDS_MAX];

size_t CliCommand::CmdCnt = 0;

size_t CliCommand::DropCnt = 0;

CliCommand::CliCommand(const char* name, CmdFuncPtr function) 
{
    if (CmdCnt < CLI_COMMANDS_MAX) 
    {
        CmdTab[CmdCnt++] = {name, function};
    } 
    else 
    {
        DropCnt++;
    }
}

cliCmd_t* CliCommand::getTable(void)
{
    return CmdTab;
}

size_t CliCommand::getCmdCnt(void)
{
    return CmdCnt;
}

size_t CliCommand::getDropCnt(void)
{
    return DropCnt;
}

CmdFuncPtr CliCommand::getCmd(const char* name)
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

int8_t CliCommand::exec(Stream& ioStream, const char* name, const char* argv[], uint8_t argc) 
{
    CmdFuncPtr pFunc = getCmd(name);

    if (pFunc != nullptr) 
    {
        return pFunc(ioStream, argv, argc);
    }

    return -1;
}
