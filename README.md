# CoCo Loop

Event Loop for CoCo that waits for events

## Import
Add coco-loop/\<version> to your conanfile where version corresponds to the git tags

## Features
* Event loop, can be instantiated multiple times in separate threads on Windows/MacOS/Linux
* Uses IO completion ports on Windows
* Time with millisecond resolution
* Sleep and yield methods for passing control to other coroutines (cooperative multitasking)
* Lets the CPU sleep until an event occurs
* Simple user interface for emulating hardware (leds, displays, buttons) on desktop OS

Can use WFE instruction on ARM. Note the wake-up time of microcontrollers of about 10μs

## Supported Platforms
See README.md of coco base library
