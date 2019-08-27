# GIMP/OSX building guide

## Pre-requirements

Build process is using on JHBuild gnome tool to build GIMP and all it
dependencies from scratch.

It is recommended to use VM or, at least, non-default user to avoid any
conflicts with Homebrew or Macports packages. For the official builds OSX 10.11
is in use, however it is not a strict requirement.

## Installing OSX in the VirtualBox

It is easy to automate OSX/VirtualBox setup using [osx-vm-templates](osx-vm-templates)
project. Some hints:

- Configure sharing to enable ssh to the buildhost from your environment
- Recommended storage size >= 30G, complete GIMP build takes ~21Gb
- It is recommended to disable screensaver and power management completely
- SMP now works fine (was a problem in a previous virtualbox versions), tested with 2 vcpu-s

## Setup host dependencies

- Install Command Line Tools for Xcode (`xcode-select --install`)
- Install `rust` using `curl https://sh.rustup.rs -sSf | sh` command, it is
required to build libRSVG.

## Install JHBuild

- Run `gimp-osx-build-setup.sh` FIXME(ADD FILE) from this directory. This should
checkout latest version of the JHBuild and configure it.
- Build bootstrap (FIXME, can we reduce number of commands?):
    - `jhbuild bootstrap`
    - `jhbuild build python python-six`
    - `jhbuild build meta-gtk-osx-bootstrap`


## Build GIMP and all it dependencies

Run `jhbuild build gimp` or `jhbuild build webkit gimp` if you want to build it with webkit (would take a while!) FIXME(check if webkit is optional dep).

Below you could find expected configure output:

```
Extra Binaries:
  gimp-console:            yes

Optional Features:
  Language selection:      yes
  Vector icons:            yes
  Dr. Mingw (Win32):       no
  Bundled MyPaint Brushes: no (at /Users/builder/gtk/inst/share/mypaint-data/1.0/brushes)
  Default ICC directory:

Optional Plug-Ins:
  Ascii Art:               yes
  Ghostscript:             yes
  Help Browser:            yes
  JPEG 2000:               yes
  MNG:                     yes
  OpenEXR:                 yes
  WebP:                    yes
  Heif:                    yes
  PDF (export):            yes
  Print:                   yes
  Python 2:                yes
  TWAIN (Win32):           no
  Webpage:                 yes
  WMF:                     yes
  X11 Mouse Cursor:        no (libXcursor not found)
  XPM:                     no (XPM library not found)
  Email:                   needs runtime dependency: xdg-email

Optional Modules:
  ALSA (MIDI Input):       no (libasound not found or unusable)
  Linux Input:             no (linux input support disabled) (GUdev support: no)
  DirectInput (Win32):     no

Tests:
  Use xvfb-run             no (not found)
  Test appdata             no (appstream-util not found)
  Test desktop file        no (desktop-file-validate not found
```

## Creating OSX bundle

To build package [gtk-mac-bundler](https://github.com/jralls/gtk-mac-bundler) and some custom scripts to re-link libraries using `@rpath`.

To create GIMP dmg do:


1. Install [gtk-mac-bundler](https://github.com/jralls/gtk-mac-bundler).
2. Clone [this repository](https://github.com/samm-git/gimp-osx-package) to the build host
3. Run `jhbuild run ./build.sh` to build the package.

FIXME: get rid of the warnings during this phase

## OSX specific files (FIXME)

The following files and subdirectories are in this folder:
```
custom                      default application data
README                      this file
gimp.icns                   application icon
gimp.svg                    application icon
gimp.modules                configuration for jhbuild
gimp.bundle                 configuration for gtk-mac-bundler
info.plist.in               OS X application metadata (template)
launcher.sh                 GIMP starter in the OS X application package
patches                     Patches as long as they are not accepted upstream
xcf.icns                    application icon
```
