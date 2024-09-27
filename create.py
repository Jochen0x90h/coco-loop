# Call conan create for all presets given in presets.txt
#
# usage: python create.py
#

import subprocess
import shlex


# read presets from presets.txt
file = open('presets.txt', 'r')
presets = file.readlines()
file.close()

# get version from git tag or branch
# get tag
try:
    version = subprocess.check_output("git tag --points-at HEAD", shell=True).decode().strip()
    if version == "":
        # get branch
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
        result = subprocess.run(f"conan create -pr:b default -pr:h {profile} -b missing -o:a platform={platform} . --version {version}", shell=True)
        if result.returncode != 0:
            exit()
