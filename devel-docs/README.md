# Developers documentation

This manual holds information that you will find useful if you
develop a GIMP plug-in or want to contribute to the GIMP core.

People only interested into plug-ins can probably read just the
[Plug-in development](#plug-in-development) section. If you wish to
contribute to all parts of GIMP, the whole documentation is of interest.

[[TOC]]

## Plug-in development
### Concepts
#### Basics

Plug-ins in GIMP are executables which GIMP can call upon certain
conditions. Since they are separate executables, it means that they are
run as their own process, making the plug-in infrastructure very robust.
No plug-in should ever crash GIMP, even with the worst bugs. If such
thing happens, you can consider this a core bug.

On the other hand, a plug-in can mess your opened files, so a badly
developed plug-in could still leave your opened images in an undesirable
state. If this happens, you'd be advised to close and reopen the file
(provided you saved recently).

Another downside of plug-ins is that GIMP currently doesn't have any
sandboxing ability. Since we explained that plug-ins are run by GIMP as
independant processes, it also means they have the same rights as your
GIMP process. Therefore be careful that you trust the source of your
plug-ins. You should never run shady plug-ins from untrusted sources.

GIMP comes itself with a lot of plug-ins. Actually nearly all file
format support is implemented as a plug-in (XCF support being the
exception: the only format implemented as core code). This makes it a
very good base to study plug-in development.

#### Procedural DataBase (PDB)

Obviously since plug-ins are separate processes, they need a way to
communicate with GIMP. This is the Procedural Database role, also known
as **PDB**.

The PDB is our protocol allowing plug-ins to request or send information
from or to the main GIMP process.

Not only this, but every plug-in has the ability to register one or
several procedures itself, which means that any plug-in can call
features brought by other plug-ins through the PDB.

#### libgimp and libgimpui

The GIMP project provides plug-in developers with the `libgimp` library.
This is the main library which any plug-in needs. All the core PDB
procedures have a wrapper in `libgimp` so you actually nearly never need
to call PDB procedures explicitly (exception being when you call
procedures registered by other plug-ins; these won't have a wrapper).

The `libgimpui` library is an optional one which provides various
graphical interface utility functions, based on the GIMP toolkit
(`GTK`). Of course, it means that linking to this library is not
mandatory (unlike `libgimp`). Some cases where you would not do this
are: because you don't need any graphical interface (e.g. a plug-in
doing something directly without dialog, or even a plug-in meant to be
run on non-GUI servers); because you want to use pure GTK directly
without going through `libgimpui` facility; because you want to make
your GUI with another toolkit…

The whole C reference documentation for both these libraries can be
generated in the main GIMP build with the `--enable-gi-docgen` autotools
option or the `-Dgi-docgen=enabled` meson option (you need to have the
`gi-docgen` tools installed).

TODO: add online links when it is up for the new APIs.

### Programming Languages

While C is our main language, and the one `libgimp` and `libgimpui` are
provided in, these 2 libraries are also introspected thanks to the
[GObject-Introspection](https://gi.readthedocs.io/en/latest/) (**GI**)
project. It means you can in fact create plug-ins with absolutely any
[language with a GI binding](https://wiki.gnome.org/Projects/GObjectIntrospection/Users)
though of course it may not always be as easy as the theory goes.

The GIMP project explicitly tests the following languages and even
provides a test plug-in as a case study:

* [C](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/extensions/goat-exercises/goat-exercise-c.c) (not a binding)
* [Python 3](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/extensions/goat-exercises/goat-exercise-py3.py)
  (binding)
* [Lua](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/extensions/goat-exercises/goat-exercise-lua.lua)
  (binding)
* [Vala](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/extensions/goat-exercises/goat-exercise-vala.vala)
  (binding)
* [Javascript](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/extensions/goat-exercises/goat-exercise-gjs.js)
  (binding, not supported on Windows for the time being)

One of the big advantage of these automatic bindings is that they are
full-featured since they don't require manual tweaking. Therefore any
function in the C library should have an equivalent in any of the
bindings.

TODO: binding reference documentation.

**Note**: several GObject-Introspection's Scheme bindings exist though
we haven't tested them. Nevertheless, GIMP also provides historically
the "script-fu" interface, based on an integrated Scheme implementation.
It is different from the other bindings (even from any GI Scheme
binding) and doesn't use `libgimp`. Please see the [Script-fu
development](#script-fu-development) section.

### Tutorials

TODO: at least in C and in one of the officially supported binding
(ideally even in all of them).

### Porting from GIMP 2 plug-ins

## Script-fu development

`Script-fu` is its own thing as it is a way to run Scheme script with
GIMP. It is itself implemented as an always-running plug-in with its own
Scheme mini-interpreter and therefore `Script-fu` scripts do not use
`libgimp` or `libgimpui`. They interface with the PDB through the
`Script-fu` plug-in.

### Tutorials

### Porting from GIMP 2 scripts

## GEGL operation development

## Custom data
### Brushes
### Dynamics
### Patterns
### Themes
### Icon themes

## GIMP extensions (*.gex*)

## Core development
### Newcomers

### Core Contributors

### Directory structure of the GIMP source tree

### Advanced concepts
