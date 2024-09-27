# Configure the project for use in an IDE that supports CMakeUserPresets.json
#
# usage:
# 1. Copy presets.txt from coco/support/conan/[windows] to project root (next to this file) and adjust to own needs
# 2. python configure.py
#

import sys
import json
from pathlib import Path
import shlex
import subprocess


# configuration
home = Path.home()

# read presets from presets.txt
file = open('presets.txt', 'r')
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
        subprocess.run(f"conan install -pr:b default -pr:h {profile} -b missing -o:a platform={platform} -of build/{name} .", shell=True)

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
                    "CMAKE_INSTALL_PREFIX": str(home / ".local")
                },
                #"toolchainFile": str(os.getcwd() / f"build/{name}/conan_toolchain.cmake"),
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
