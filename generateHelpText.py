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

def CliCommandsInFolder(searchPath):
    results = []

    for folder, dirs, files in os.walk(searchPath):

        ignoreList = [".git", ".vscode", "libCli"]
        if any(key in folder for key in ignoreList):
            continue

        for file in files:
            if file.endswith('.cpp'):
                fullpath = os.path.join(folder, file)
                with open(fullpath, 'r') as f:
                    content = f.read()
                commands = re.findall(r'(/\*\*(.*?)\*/\s*)?CLI_COMMAND\((.*?)\)', content, re.DOTALL)
                for command in commands:
                    doc, _, name = command
                    syntax_text = name
                    if doc:
                        brief = re.search(r'@brief\s+(.*)', doc)
                        args = re.findall(r'@arg\s+(\S+)\s+(.*)', doc)
                        brief_text = brief.group(1).strip() if brief else "No help available."
                        help_text = brief_text + "\n"
                        if args:
                            syntax_text += ' ' + ' '.join(arg[0] for arg in args)
                            for arg in args:
                                help_text += f"{arg[0]} ... {arg[1]}\n"
                    else:
                        help_text = "\n"
                    results.append((name, syntax_text, help_text.strip()))

    return results

def remove_first_word(text):
    words = text.split()
    return ' '.join(words[1:]) if len(words) > 1 else ''

def remove_leading_whitespace_before_newline(text):
    # Entfernt führende Leerzeichen, wenn sie direkt von einem \n gefolgt werden
    return re.sub(r'^[ \t]+(?=\n)', '', text, flags=re.MULTILINE)

def format_help_text(commands):
    # Bestimme die maximale Länge der Syntax für die Einrückung
    max_syntax_length = max(len(syntax) for _, syntax, _ in commands)
    print("Max Syntax Length: ", max_syntax_length)

    formatted_commands = []
    for name, syntax, help_text in commands:
        indent = ' ' * (max_syntax_length - len(syntax) + 4)  # 4 Leerzeichen als Abstand
        help_lines = help_text.split('\n')
       # syntax = remove_first_word(syntax)
        formatted_help = f"{syntax}{indent}{help_lines[0]}"
        subsequent_indent = ' ' * (max_syntax_length + 6)
        if len(help_lines) > 1:
            formatted_help += ''.join(f'\\n{subsequent_indent}{line}' for line in help_lines[1:])
        #formatted_help += '\\n'  # Füge einen Zeilenumbruch am Ende hinzu
        formatted_commands.append((name, formatted_help))

    return formatted_commands

def generate_c_strings(commands):
    formatted_commands = format_help_text(commands)
    c_strings = []
    for name, formatted_help in formatted_commands:
        var_name = f"cmd_{name}_help"
        c_strings.append(f'const char *{var_name} = "{formatted_help}";')
    return "\n".join(c_strings)

pioEnv = env['PIOENV']
projectPath = env['PROJECT_DIR']
projectName = os.path.basename(projectPath)

targetFileName= os.getcwd() + "/help.cpp"

searchList = [
    projectPath + "/src",
    projectPath + "/lib",
    projectPath + "/include",
    projectPath + "/.pio/libdeps/" + pioEnv
]

cmdList = []

print("libcli: Generate help text ...")

for path in searchList:
    cmdList.extend(CliCommandsInFolder(path))

cmdList = sorted(cmdList, key=lambda x: x[0])

# Generiere die C-Strings
c_strings = generate_c_strings(cmdList)

# Schreibe die C-Strings in die Ausgabedatei
with open(targetFileName, 'w') as f:
    f.write(f'{c_strings}\n')