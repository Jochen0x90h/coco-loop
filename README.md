# CoCo Loop

Event Loop for CoCo that waits for events (e.g. WFE instruction on ARM)

## Import
Add coco-loop/\<version> to your conanfile where version corresponds to the git tags

## Features
* Lets the CPU sleep until an event occurs. Note the wake-up time
* Also runs on Windows/MacOS/Linux using poll()

## Supported Platforms
See README.md of coco base library
