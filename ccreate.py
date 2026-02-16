# Calls "conan create" for all presets in cpresets.txt
# This creates packages that can be used by dependent projects
#
# usage: $ python ccreate.py
#

import subprocess
import shlex


# read presets from presets.txt
file = open('cpresets.txt', 'r')
presets = file.readlines()
file.close()

# get version from git tag or branch
try:
    # get modified files and tag
    modified = subprocess.check_output("git diff --name-only", shell=True).decode().strip()
    staged = subprocess.check_output("git diff --name-only --staged", shell=True).decode().strip()
    version = subprocess.check_output("git tag --points-at HEAD", shell=True).decode().strip()
    if modified != "" or staged != "" or version == "":
        # get branch if modified/staged or no tag found
        version = subprocess.check_output("git rev-parse --abbrev-ref HEAD", shell=True).decode().strip()
except:
    # not a git repository
    version = "none"
#print(f"Version: >{version}<")

for preset in presets:
    p = shlex.split(preset)
    if not preset.startswith('#') and len(p) == 4:
        profile = p[0]
        platform = p[1]
        #print(f"Platform: >{platform}< Profile: >{profile}<")

        # create
        result = subprocess.run(f"conan create -nr -pr:b default -pr:h {profile} -b missing -o:a \"&:platform={platform}\" . --version {version}", shell=True)
        if result.returncode != 0:
            exit()
