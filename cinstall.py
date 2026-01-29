# Calls "conan install" for all presets in cpresets.txt
# Also creates a CMakeUserPresets.json which is supported by IDEs such as VSCode
#
# usage:
# 1: Copy cpresets.txt containing a list of presets from support/conan/[operating system] to project root (next to this file)
# 2: Optional: Open cpresets.txt in an editor and adjust to own needs
# 3: $ python cinstall.py
#

import sys
import json
from pathlib import Path
import shlex
import subprocess


# configuration
installPrefix = str(Path.home() / ".local")


# read presets from presets.txt
file = open('cpresets.txt', 'r')
presets = file.readlines()
file.close()

# cmake presets
cmakePresets = {
    "version": 3,
    "configurePresets": [],
    "buildPresets": [],
    "testPresets": []
}

# add a preset to the cmake presets
def addPreset(type, name):
    cmakePresets[type].append(
        {
            "name": name,
            "configurePreset": name
        }
    )

def addPresetWithConfig(type, name, config):
    cmakePresets[type].append(
        {
            "name": name,
            "configurePreset": name,
            "configuration": config
        }
    )

# iterate over presets
for preset in presets:
    p = shlex.split(preset)
    if not preset.startswith('#') and len(p) == 4:
        profile = p[0]
        platform = p[1]
        config = p[2]
        generator = p[3]
        if config == 'Release':
            name = platform
        else:
            name = f"{platform}-{config}"

        # install dependencies using conan
        print(f"*** Installing dependencies for {profile} {platform} ***")
        subprocess.run(f"conan install -pr:b default -pr:h {profile} -b missing -o:a *:platform={platform} -of build/{name} .", shell=True)

        # create cmake presets
        cmakePresets["configurePresets"].append(
            {
                "name": name,
                "description": f"({generator})",
                "generator": generator,
                "cacheVariables": {
                    #"CMAKE_POLICY_DEFAULT_CMP0077": "NEW",
                    "CMAKE_POLICY_DEFAULT_CMP0091": "NEW",
                    "CMAKE_BUILD_TYPE": config,
                    "CMAKE_INSTALL_PREFIX": installPrefix
                },
                "toolchainFile": f"build/{name}/conan_toolchain.cmake",
                "binaryDir": f"build/{name}"
            }
        )
        if "Visual Studio" in generator:
            addPresetWithConfig("buildPresets", name, config)
            addPresetWithConfig("testPresets", name, config)
        else:
            addPreset("buildPresets", name)
            addPreset("testPresets", name)

# save cmake presets to CMakeUserPresets.json
file = open("CMakeUserPresets.json", "w")
file.write(json.dumps(cmakePresets, indent=4))
file.close()
