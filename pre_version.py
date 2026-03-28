# libcli, a simple and generic command line interface with small footprint for
# bare metal embedded projects.
#
# Copyright (C) 2026 Julian Friedrich
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
Import('env')
import os
import time 
import subprocess

if not os.path.exists("cli"):
    # abort if git is not available or this is not a git repository
    print("cli folder not found, aborting version header generation")
    exit(1)

targetFileName= os.getcwd() + "/cli/version.h"

# get the library version from library.json as <major>.<minor>.<patch>
with open("library.json", "r") as f:
    for line in f:
        if '"version"' in line:
            version = line.split(":")[1].strip().strip('"').strip(',')
            # remove trailing " if present
            if version.endswith('"'):
                version = version[:-1]
            break

#check if we got <major>.<minor>.<patch> format
if not version.count(".") == 2:
    print("version in library.json is not in <major>.<minor>.<patch> " \
    "format, aborting version header generation")
    print("version: " + version)
    exit(1)

version = "v" + version
version_source = "library.json"

# if libcli is a git repo, use that instead
try:
    gitVersion = subprocess.check_output(
        ["git", "describe", "--abbrev", "--dirty", "--always", "--tags"], 
        stderr=subprocess.STDOUT).decode().strip()
    if gitVersion:
        version = gitVersion
        version_source = "git"
except subprocess.CalledProcessError:
    # git command failed, probably not a git repository
    pass

file = open(targetFileName, "w")

file.write("""/*
 * Auto generated version header file for libCli.
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
 * This project is hosted on GitHub:
 *   https://github.com/fjulian79/libcli
 * Please feel free to file issues, open pull requests, or contribute there.
 */
 
/****************************************************************************** 
 *********** WARNING: AUTO GENERATED FILE, DO NOT EDIT IT MANUALLY! ***********
 *****************************************************************************/

#pragma once""")

file.write("""\n\n/**
 * @brief libCli version.
 *
 * If libcli is a git repo "git describe --abbrev --dirty --always --tags" is 
 * used to create this string, otherwise the version defined in library.json is
 * used. The used source is stated below.
 *
 * Version source: """ + version_source + """ 
 */
#define CLI_VERSION                         \"""" + version + "\"")

file.close()
