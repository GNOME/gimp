#!/usr/bin/env -S make -f
default: build

# This file is executable so you can do
# ./meson.make
# ./meson.make install


prefix = /usr
prefix = $(HOME)/.local


_build:
	meson _build --prefix=$(prefix)


.PHONY: build
build: | _build
	ninja -C _build


.PHONY: install
install:
	ninja -C _build install


.PHONY: clean
clean:
	rm -rf _build
