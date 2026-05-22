# Determines all conan dependencies for the presets in cpresets.txt
# May take a while if cpresets.txt contains several presets
#
# usage:
# $ python cdeps.py
#

import platform
import json
from pathlib import Path
import shlex
import subprocess


# get system (Linux, Darwin, Windows)
system = platform.system()
#print(f"system {system}")

# read presets from presets.txt
file = open('cpresets.txt', 'r')
presets = file.readlines()
file.close()

# iterate over presets
all_deps = []
for preset in presets:
    p = shlex.split(preset)
    if preset.startswith('#') or len(p) < 3:
       continue

    # check optional system
    if len(p) >= 4 and p[3] != system:
       continue

    profile = p[0]
    platform = p[1]
    generator = p[2]

    # get build_type (Debug/Release) from profile
    result = subprocess.run(f"conan graph info -pr:h={profile} --format=json -o:a \"&:platform={platform}\" .", shell=True, capture_output=True, check=True)
    j = json.loads(result.stdout)
    #print(json.dumps(j, indent=4, ensure_ascii=False))

    for node_id, node in j["graph"]["nodes"]["0"]["dependencies"].items():
        ref = node.get("ref")
        libs = node.get("libs")
        if libs:
            all_deps.append(ref)

# remove duplicates and print
all_deps = sorted(set(all_deps))
for dep in all_deps:
    print(dep)
