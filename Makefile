.PHONY: release debug install upload

# name (from conan package)
NAME := $(shell python3 -c 'from conanfile import Project; print(Project.name)')

# version (from git branch or tag)
left := (
right := )
BRANCH := $(shell git tag -l --points-at HEAD)
ifeq ($(BRANCH),)
	BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
endif
VERSION := $(subst /,-,$(subst $(left),_,$(subst $(right),_,$(BRANCH))))

# name/version@user/channel
REFERENCE := $(NAME)/$(VERSION)@

# options
export CONAN_RUN_TESTS=1
export CONAN_INSTALL_PREFIX=${HOME}/.local
RELEASE := -pr default -b missing
DEBUG := -pr debug -b missing
ARMV7 := -pr:b default -pr:h armv7
ARMV6 := -pr:b default -pr:h armv6


# default target
all: release debug armv7 armv6

release:
	conan create $(RELEASE) . $(REFERENCE)
	conan create $(RELEASE) -o platform=native . $(REFERENCE)

debug:
	conan create $(DEBUG) . $(REFERENCE)
	conan create $(DEBUG) -o platform=native . $(REFERENCE)

armv7:
	conan create $(ARMV7) -o platform=nrf52840 . $(REFERENCE)

armv6:
	conan create $(ARMV6) -o platform=stm32f042x6 . $(REFERENCE)
	conan create $(ARMV6) -o platform=stm32f051x8 . $(REFERENCE)

# install (e.g. to ~/.local)
install:
	conan install $(RELEASE) $(REFERENCE)

# upload package to conan repository
upload:
	conan upload $(REFERENCE) --all --force
