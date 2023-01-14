# libversion, a version header file generator providing infos on the 
# projects git repo.
#
# Copyright (C) 2021 Julian Friedrich
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>. 
#
# You can file issues at https://github.com/fjulian79/libversion/issues

Import('env')
import os
import re
import time 
import subprocess
from collections import namedtuple

cwd = os.getcwd()
targetFileName= cwd + "/cmdTable.cpp"

projectPath = env['PROJECT_DIR']
projectName = os.path.basename(projectPath)

Command = namedtuple("Command", "file line cmd")
cmdList = []

# Search for commands in the entire project
for folder, dirs, files in os.walk(projectPath):
    if folder == cwd:
        continue
    for file in files:
        if file.endswith('.cpp'):
            fullpath = os.path.join(folder, file)
            with open(fullpath, 'r') as f:
                for line, text in enumerate(f):
                    match = re.search('^CLI_COMMAND\((.+?)\)', text)
                    if match:
                        filename = os.path.relpath(fullpath, projectPath)
                        cmd = match.group(1)
                        cmdList.append(Command(filename, line, cmd))

# Write the output file
file = open(targetFileName, "w")
file.write("""/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * This is a generated file for project """ + projectName + """.
 *
 * Copyright (C) """ + time.strftime("%Y") + """ Julian Friedrich
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

/****************************************************************************** 
 *********** WARNING: AUTO GENERATED FILE, DO NOT EDIT IT MANUALLY! ***********
 *****************************************************************************/

#include "cli/cli.hpp"
#include <generic/generic.hpp>

/** 
 * Declaration of functions found in cpp files within the project.
 */
""")

if len(cmdList) > 0:
    for item in cmdList:
        code = "CLI_COMMAND(" + item.cmd + "); "
        file.write(code.ljust(32, ' ') + "/* Found in " + item.file + " at line " + str(item.line) + " */\n")
else:
    file.write("#warning No cli commands found, see libcli README\n")

file.write("""
/**
 * The generated command table
 */
cliCmd_t cmdTable_generated[] =
{
""")

if len(cmdList) > 0:
    for index, item in enumerate(cmdList):
        if index > 0:
            file.write(",\n")
        file.write("    CLI_CMD_DEF(" + item.cmd + ")")
    index = index + 1
else:
    file.write("    {0, 0}")
    index = 0

file.write("""
};

void Cli::begin(Stream *pIoStr)
{
    pCmdTab = cmdTable_generated;
    CmdTabSiz = """ + str(index) + """;
    setStream(pIoStr);
}
""")
