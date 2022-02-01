---
title: Developers documentation
---

This manual holds information that you will find useful if you
develop a GIMP plug-in or want to contribute to the GIMP core.

People only interested into plug-ins can probably read just the
[Plug-in development](#plug-in-development) section. If you wish to
contribute to all parts of GIMP, the whole documentation is of interest.

[TOC]

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

### Debugging

GIMP provides an infrastructure to help debugging plug-ins.

You are invited to read the [dedicated
documentation](debug-plug-ins.txt).

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

This section list all types of data usable to enhance GIMP
functionalities. If you are interested to contribute default data to
GIMP, be aware that we are looking for a very good base set, not an
unfinite number of data for all possible usage (even the less common
ones).

Furthermore we only accept data on Libre licenses:

* [Free Art License](https://artlibre.org/licence/lal/en/)
* [CC0](https://creativecommons.org/publicdomain/zero/1.0/)
* [CC BY](https://creativecommons.org/licenses/by/4.0/)
* [CC BY-SA](https://creativecommons.org/licenses/by-sa/4.0/)

Of course you are free to share data usable by GIMP on any license you
want on your own. Providing them as third-party GIMP
[extensions](#gimp-extensions-gex) is probably the best idea.

### Brushes

GIMP currently supports the following brush formats:

* GIMP Brush (GBR): format to store pixmap brushes
* GIMP Brush Pipe (GIH): format to store a series of pixmap brushes
* GIMP Generated Brush (VBR): format of "generated" brushes
* GIMP Brush Pixmap (GPB): *OBSOLETE* format to store pixel brushes
* MyPaint brushes v1 (MYB)
* Photoshop ABR Brush
* Paint Shop Pro JBR Brush

We do fully support the GIMP formats obviously, as well as MyPaint
brushes, since we use the official `libmypaint` library. We are not sure
how well we support other third-party formats, especially if they had
recent versions.

If you are interested in brushes from a developer perspective, you are
welcome to read specifications of GIMP formats:
[GBR](specifications/gbr.txt), [GIH](specifications/gih.txt),
[VBR](specifications/vbr.txt) or the obsolete [GPB](specifications/gpb.txt).

If you want to contribute brushes to the official GIMP, be aware we
would only accept brushes in non-obsolete GIMP formats. All these
formats can be generated by GIMP itself from images.

If you want to contribute MyPaint brushes, we recommend to propose them
to the [MyPaint-brushes](https://github.com/mypaint/mypaint-brushes/)
data project, which is also used by GIMP for its default MyPaint brush
set.

Otherwise, you are welcome to provide brush set in any format as
third-party [extensions](#gimp-extensions-gex).

### Dynamics

GIMP supports the GIMP Paint Dynamics format which can be generated from
within GIMP.

### Patterns

GIMP supports the GIMP Pattern format (PAT, whose
[specification](specifications/pat.txt) is available for developers).

This format can be exported by GIMP itself.

Alternatively GIMP supports patterns from `GdkPixbuf` (TODO: get more
information?).

### Palettes

GIMP supports the GIMP Palette format which can be generated from within
GIMP.

### Gradients

GIMP supports the GIMP Gradient format (GGR, whose
[specification](specifications/ggr.txt) is available for developers)
which can be generated from within GIMP.

Alternatively GIMP supports the SVG Gradient format.

### Themes
### Icon themes

Icon sets (a.k.a. "icon themes") have been separated from themes since
GIMP 2.10 so you can have any icon theme with any theme.

We currently only support 2 such icon themes — Symbolic and Color — and
we keep around the Legacy icons.

We don't want too many alternative designs as official icon themes
(people are welcome to publish their favorite designs as third-party
icons) though we would welcome special-purpose icon themes (e.g. high
contrast).

We also welcome design updates as a whole (anyone willing to work on
this should discuss with us and propose something) and obviously fixes
on existing icons or adding missing icons while keeping consistent
styling.

See the dedicated [icons documentation](icons.md) for more technical
information.

### Tool presets


## GIMP extensions (*.gex*)

## Continuous Integration

For most of its continuous integration (macOS excepted), GIMP project
uses Gitlab CI. We recommend looking the file
[.gitlab-ci.yml](/.gitlab-ci.yml) which is the startup script.

The main URL for our CI system is
[build.gimp.org](https://build.gimp.org) which redirects to Gitlab
pipelines page.

Note that it is important to keep working CI jobs for a healthy code
source. Therefore when you push some code which breaks the CI (you
should receive a notification email when you do so), you are expected to
look at the failed jobs' logs, try and understand the issue(s) and fix
them (or ask for help). Don't just shrug this because it works locally
(the point of the CI is to build in more conditions than developers
usually do locally).

Of course, sometimes CI failures are out of our control, for instance
when downloaded dependencies have issues, or because of runner issues.
You should still check that these were reported and that
packagers/maintainers of these parts are aware and working on a fix.

### Automatic pipelines

At each commit pushed to the repository, several pipelines are currently
running, such as:

- Debian testing autotools and meson builds (autotools is still the
  official build system while meson is experimental).
- Windows builds (cross or natively compiled).

Additionally, we test build with alternative tools or options (e.g. with
`Clang` instead of `gcc` compiler) or jobs which may take much longer,
such as package creation as scheduled pipelines (once every few days).

The above listing is not necessarily exhaustive nor is it meant to be.
Only the [.gitlab-ci.yml](/.gitlab-ci.yml) script is meant to be
authoritative. The top comment in this file should stay as exhaustive
as possible.

### Manual pipelines

It is possible to trigger pipelines manually, for instance with specific
jobs, if you have the "*Developer*" Gitlab role:

1. go to the [Pipelines](https://gitlab.gnome.org/GNOME/gimp/-/pipelines)
   page.
2. Hit the "*Run pipeline*" button.
3. Choose the branch or tag you wish to build.
4. Add relevant variables. A list of variables named `GIMP_CI_*` are
   available (just set them to any value) and will trigger specific job
   lists. These variables are listed in the top comment of
   [.gitlab-ci.yml](/.gitlab-ci.yml).

### Merge request pipelines

Special pipelines happen for merge request code. For instance, these
also include a (non-perfect) code style check.

Additionally you can trigger Windows installer or flatpack standalone
packages to be generated with the MR code as explained in
[gitlab-mr.md](gitlab-mr.md).

### Release pipeline

Special pipelines happen when pushing git `tags`. These should be tested
before a release to avoid unexpected release-time issues, as explained
in [release-howto.txt](release-howto.txt).

### Exception: macOS

As an exception, macOS is currently built with the `Circle-CI` service.
The whole CI scripts and documentation can be found in the dedicated
[gimp-macos-build](https://gitlab.gnome.org/Infrastructure/gimp-macos-build)
repository.

Eventually we want to move this pipeline to Gitlab as well.

## Core development

When writing code, any core developer is expected to follow:

- GIMP's [coding style](/CODING_STYLE.md);
- the [directory structure](#directory-structure-of-gimp-source-tree)
- our [header file inclusion policy](includes.txt)

[GIMP's developer wiki](https://wiki.gimp.org/index.php/Main_Page) can
also contain various valuable resources.

### Newcomers

If this is your first time contributing to GIMP, you might be interested
by these [instructions on submitting
patches](https://gimp.org/bugs/howtos/submit-patch.html).

If you are unsure what to work on, this [list of bugs for
newcomers](https://gitlab.gnome.org/GNOME/gimp/-/issues?scope=all&state=opened&label_name[]=4.%20Newcomers)
might be a good start. It doesn't necessarily contain only bugs for
beginner developers. Some of them might be for experienced developers
who just don't know yet enough the codebase.

Nevertheless we often recommend to rather work on topics which you
appreciate, or even better: fixes for bugs you encounter or features you
want. These are the most self-rewarding contributions which will really
make you feel like developing on GIMP means developing for yourself.

### Core Contributors

### Maintainers

GIMP maintainers have a few more responsibilities, in particular
regarding releases and coordination.

Some of these duties include:

- setting the version of GIMP as well as the API version. This is
  explained in [libtool-instructions.txt](libtool-instructions.txt).
- Making a release by followng accurately the process described in
  [release-howto.txt](release-howto.txt).
- Managing dependencies: except for core projects (such as `babl` and
  `GEGL`), we should stay as conservative as possible for the stable
  branch (otherwise distributions might end up getting stuck providing
  very old GIMP versions). On development builds, we should verify any
  mandatory dependency is at the very least available in Debian testing
  and MSYS2; we may be a bit more adventurous for optional dependencies
  yet stay reasonable (a feature is not so useful if nobody can build
  it). In any case, any dependency bump must be carefully weighed within
  reason, especially when getting close to make the development branch
  into the new stable branch. See also [os-support.txt](os-support.txt).
- Maintain [milestones](gitlab-milestones.txt).
- Maintain [NEWS](/NEWS) file. Any developer is actually encouraged to
  update it when they do noteworthy changes, but this is the
  maintainers' role to do the finale checks and make sure we don't miss
  anything. The purpose of this rule is to make it as easy as possible
  to make a GIMP release as looking in this file to write release notes
  is much easier than reviewing hundreds of commits.

### Directory structure of GIMP source tree

GIMP source tree can be divided into the main application, libraries, plug-ins,
data files and some stuff that don't fit into these categories. Here are the
top-level directories:

| Folder          | Description |
| ---             | ---         |
| app/            | Source code of the main GIMP application             |
| app-tools/      | Source code of distributed tools                     |
| build/          | Scripts for creating binary packages                 |
| cursors/        | Bitmaps used to construct cursors                    |
| data/           | Data files: brushes, gradients, patterns, images…    |
| desktop/        | Desktop integration files                            |
| devel-docs/     | Developers documentation                             |
| docs/           | Users documentation                                  |
| etc/            | Configuration files installed with GIMP              |
| extensions/     | Source code of extensions                            |
| icons/          | Official icon themes                                 |
| libgimp/        | Library for plug-ins (core does not link against)    |
| libgimpbase/    | Basic functions shared by core and plug-ins          |
| libgimpcolor/   | Color-related functions shared by core and plug-ins  |
| libgimpconfig/  | Config functions shared by core and plug-ins         |
| libgimpmath/    | Mathematic operations useful for core and plug-ins   |
| libgimpmodule/  | Abstracts dynamic loading of modules (used to implement loadable color selectors and display filters) |
| libgimpthumb/   | Thumbnail functions shared by core and plug-ins      |
| libgimpwidgets/ | User interface elements (widgets) and utility functions shared by core and plug-ins                   |
| m4macros/       | Scripts for autotools configuration                  |
| menus/          | XML/XSL files used to generate menus                 |
| modules/        | Color selectors and display filters loadable at run-time |
| pdb/            | Scripts for PDB source code generation               |
| plug-ins/       | Source code for plug-ins distributed with GIMP       |
| po/             | Translations of strings used in the core application |
| po-libgimp/     | Translations of strings used in libgimp              |
| po-plug-ins/    | Translations of strings used in C plug-ins           |
| po-python/      | Translations of strings used in Python plug-ins      |
| po-script-fu/   | Translations of strings used in Script-Fu scripts    |
| po-tags/        | Translations of strings used in tags                 |
| po-tips/        | Translations of strings used in tips                 |
| po-windows-installer/ | Translations of strings used in the Windows installer |
| themes/         | Official themes                                      |
| tools/          | Source code for non-distributed GIMP-related tools   |
| .gitlab/        | Gitlab-related templates or scripts                  |

The source code of the main GIMP application is found in the `app/` directory:

| Folder          | Description |
| ---             | ---         |
| app/actions/    | Code of actions (`GimpAction*` defined in `app/widgets/`) (depends: GTK)         |
| app/config/     | Config files handling: GimpConfig interface and GimpRc object (depends: GObject) |
| app/core/       | Core of GIMP **core** (depends: GObject)                                         |
| app/dialogs/    | Dialog widgets (depends: GTK)                                                    |
| app/display/    | Handles displays (e.g. image windows) (depends: GTK)                             |
| app/file/       | File handling routines in **core** (depends: GIO)                                |
| app/file-data/  | GIMP file formats (gbr, gex, gih, pat) support (depends: GIO)                    |
| app/gegl/       | Wrapper code for babl and GEGL API (depends: babl, GEGL)                         |
| app/gui/        | Code that puts the user interface together (depends: GTK)                        |
| app/menus/      | Code for menus (depends: GTK)                                                    |
| app/operations/ | Custom GEGL operations (depends: GEGL)                                           |
| app/paint/      | Paint core that provides different ways to paint strokes (depends: GEGL)         |
| app/pdb/        | Core side of the Procedural Database, exposes internal functionality             |
| app/plug-in/    | Plug-in handling in **core**                                                     |
| app/propgui/    | Property widgets generated from config properties (depends: GTK)                 |
| app/tests/      | Core unit testing framework                                                      |
| app/text/       | Text handling in **core**                                                        |
| app/tools/      | User interface part of the tools. Actual tool functionality is in core           |
| app/vectors/    | Vectors framework in **core**                                                    |
| app/widgets/    | Collection of widgets used in the application GUI                                |
| app/xcf/        | XCF file handling in **core**                                                    |

You should also check out [gimp-module-dependencies.svg](gimp-module-dependencies.svg).
**TODO**: this SVG file is interesting yet very outdated. It should not
be considered as some kind dependency rule and should be updated.

### Advanced concepts

#### XCF

The `XCF` format is the core image format of GIMP, which mirrors
features made available in GIMP. More than an image format, you may
consider it as a work or project format, as it is not made for finale
presentation of an artwork but for the work-in-progress processus.

Developers are welcome to read the [specifications of XCF](specifications/xcf.txt).

#### UI Framework

GIMP has an evolved GUI framework, with a toolbox, dockables, menus…

This [document describing how the GIMP UI framework functions and how it
is implemented](ui-framework.txt) might be of interest.

#### Contexts

GIMP uses a lot a concept of "contexts". We recommend reading more about
[how GimpContexts are used in GIMP](contexts.txt).

#### Undo

GIMP undo system can be challenging at times. This [quick overview of
the undo system](undo.txt) can be of interest as a first introduction.

#### Parasites

GIMP has a concept of "parasite" data which basically correspond to
persistent or semi-persistent data which can be attached to images or
items (layers, channels, paths) within an image. These parasites are
saved in the XCF format.

Parasites can also be attached globally to the GIMP session.

Parasite contents is format-free and you can use any parasite name,
nevertheless GIMP itself uses parasite so you should read the
[descriptions of known parasites](parasites.txt).

#### Metadata

GIMP supports Exif, IPTC and XMP metadata as well as various image
format-specific metadata. The topic is quite huge and complex, if not
overwhelming.

This [old document](exif-handling.txt) might be of interest (or maybe
not, it has not been recently reviewed and might be widely outdated; in
any case, it is not a complete document at all as we definitely do a lot
more nowadays). **TODO**: review this document and delete or update it
depending of whether it still makes sense.

#### Tagging

Various data in GIMP can be tagged across sessions.

This document on [how resource tagging in GIMP works](tagging.txt) may
be of interest.
