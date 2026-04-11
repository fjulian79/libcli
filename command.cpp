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

#include "cli/command.hpp"

cliCmd_t CliCommand::CmdTab[CLI_COMMANDS_MAX];

size_t CliCommand::CmdCnt = 0;

size_t CliCommand::DropCnt = 0;

CliCommand::CliCommand(const char* name, CmdFuncPtr function) {
    if (CmdCnt < CLI_COMMANDS_MAX) {
        CmdTab[CmdCnt++] = {name, function};
    } else {
        DropCnt++;
    }
}

/**
 * @brief Used to compare two commands for sorting the command table.
 * 
 * Needed in this form for qsort, see sortTable function below.
 */
static int cmdCompare(const void* a, const void* b) {
    return strcmp(((cliCmd_t*)a)->name, ((cliCmd_t*)b)->name);
}

void CliCommand::sortTable(void) {
    /* Using qsort for O(n log n) performance instead of simpler alternatives:
     * - Bubble Sort: O(n²) - simple but ~1225 comparisons at 50 commands
     * - Insertion Sort: O(n²) - better for small/sorted data, still quadratic
     * - Shell Sort: ~O(n^1.3) - good middle ground, but more complex
     * - qsort: O(n log n) - only ~280 comparisons at 50 commands
     * 
     * While this is only called once during init, qsort scales better if users
     * increase CLI_COMMANDS_MAX. The stdlib implementation is expected to be 
     * well-tested and optimized. Stack usage from recursion is acceptable for
     * one-time init.
     */
    qsort(CmdTab, CmdCnt, sizeof(cliCmd_t), cmdCompare);  
}

cliCmd_t* CliCommand::getTable(void) {
    return CmdTab;
}

size_t CliCommand::getCmdCnt(void) {
    return CmdCnt;
}

size_t CliCommand::getDropCnt(void) {
    return DropCnt;
}

CmdFuncPtr CliCommand::getCmd(const char* name) {
    for (size_t i = 0; i < CmdCnt; i++) {
        if (strcmp(name, CmdTab[i].name) == 0) {
            return CmdTab[i].pfunc;
        }
    }

    return nullptr;
}

int8_t CliCommand::exec(Stream& ioStream, const char* name, const char* argv[],
                        uint8_t argc) {
    CmdFuncPtr pFunc = getCmd(name);

    if (pFunc != nullptr) {
        return pFunc(ioStream, argv, argc);
    }

    return -1;
}
