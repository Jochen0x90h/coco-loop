.PHONY: release debug install upload

# name (from conan package)
NAME := $(shell python3 -c 'from conanfile import Project; print(Project.name)')

# version (from git branch or tag)
left := (
right := )
BRANCH := $(shell git diff-index --quiet HEAD && git tag --points-at HEAD)
ifeq ($(BRANCH),)
	BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
endif
VERSION := $(subst /,-,$(subst $(left),_,$(subst $(right),_,$(BRANCH))))

# name/version@user/channel
REFERENCE := $(NAME)/$(VERSION)@

# options
export CONAN_RUN_TESTS=1
export CONAN_INSTALL_PREFIX=${HOME}/.local
RELEASE := -pr:b default -pr:h default -b missing -b $(REFERENCE)
DEBUG := -pr:b debug -pr:h debug -b missing -b $(REFERENCE)
ARMV6 := -pr:b default -pr:h armv6 -b missing -b $(REFERENCE)
ARMV7 := -pr:b default -pr:h armv7 -b missing -b $(REFERENCE)


# default target
all: native emu nrf52 stm32f0

#debug:
#	conan create $(DEBUG) . $(REFERENCE)

#default:
#	conan create $(RELEASE) . $(REFERENCE)

native:
	conan create $(DEBUG) -o platform=native . $(REFERENCE)
#	conan create $(RELEASE) -o platform=native . $(REFERENCE)

emu:
	conan create $(DEBUG) -o platform=emu . $(REFERENCE)
#	conan create $(RELEASE) -o platform=emu . $(REFERENCE)

nrf52:
	conan create $(ARMV7) -o platform=nrf52840 . $(REFERENCE)

stm32f0:
#	conan create $(ARMV6) -o platform=stm32f042x6 . $(REFERENCE)
	conan create $(ARMV6) -o platform=stm32f051x8 . $(REFERENCE)


# install (e.g. to ~/.local)
install:
	conan install $(RELEASE) $(REFERENCE)

# upload package to conan repository
upload:
	conan upload $(REFERENCE) --all --force
