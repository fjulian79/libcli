Import("env")
import os

def action(source, target, env):
    pioEnv = env['PIOENV']
    projectPath = env['PROJECT_DIR']
    libPath = os.path.join(projectPath, ".pio", "libdeps", pioEnv, "libCli")
    linker_script = os.path.join(libPath, "cli_cmd_section.ld")
    env.Append(LINKFLAGS=[f"-T{linker_script}"])

global_env = DefaultEnvironment()
global_env.AddPreAction("$PROGPATH", action)