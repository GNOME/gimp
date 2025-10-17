---
title: Developers documentation
---

This manual holds information that you will find useful if you
develop a GIMP plug-in or want to contribute to the GIMP core.

People only interested into plug-ins can probably read just the
[Plug-in development](#plug-in-development) section. If you wish to
contribute to all parts of GIMP, the whole documentation is of interest.

[TOC]

## Plug-in and Filters development

All needed information for Plug-in and Filters development is documented on the
[Resource Development](https://developer.gimp.org/resource/)
section of GIMP Developer website, with exception of the following:

### Porting from GIMP 2 plug-ins

Take a look at our [porting guide](GIMP3-plug-in-porting-guide/README.md).

### Themes

GTK3 uses CSS themes. Don't be fooled though. It's not real CSS in that
it doesn't have all the features of real web CSS, and since it's for
desktop applications, some things are necessarily different. What it
means is mostly that it "looks similar" enough that people used to web
styling should not be too disorientated.

You can start by looking at the [official
documentation](https://docs.gtk.org/gtk3/migrating-themes.html) for
theme migration (from GTK+2 to 3), which gives a good overview, though
it's far from being perfect unfortunately.

Another good idea would be to look at existing well maintained GTK3
themes to get inspiration and see how things work.

Finally you can look at our existing themes, like the [System
theme](https://gitlab.gnome.org/GNOME/gimp/-/blob/master/themes/System/gimp.css).
Note though that this `System` theme is pretty bare, and that's its goal
(try to theme as few as possible over whatever is the current real
GTK system theme).

As a last trick for theme makers, we recommend to work with the
GtkInspector tool, which allows you to test CSS rules live in the `CSS`
tab. You can run the `GtkInspector` by going to the `File > Debug` menu
and selecting `Start GtkInspector` menu item.

It also allows you to find the name of a widget to use in your CSS
rules. To do so:

* Start the `GtkInspector`;
* go on the "Objects" tab;
* click the "target" 🞋 icon on the headerbar's top-left, then pick in
  GIMP interface the widget you are interested to style;
* the widget name will be displayed on the top of the information area
  of the dialog.
* Feel free to browse the various sections to see the class hierarchy,
  CSS nodes and so on.
* The second top-left button (just next to the target icon) allows you
  to switch between the details of the selected widget and the widget
  hierarchy (container widgets containing other widgets), which is also
  very useful information.

Additionally you can quickly switch between the light and dark variant
of a same theme by going to "Visual" tab and switching the "Dark
Variant" button ON or OFF.


## Core development

When writing code, any core developer is expected to follow:

- GIMP's [coding style](https://developer.gimp.org/core/coding_style/);
- the [directory structure](#directory-structure-of-gimp-source-tree)
- our [header file inclusion policy](includes.txt)

[GIMP's developer site](https://developer.gimp.org/) contain various valuable resources.

Finally the [debugging-tips](https://developer.gimp.org/core/debug/debugging-tips/) file contain many very
useful tricks to help you debugging in various common cases.

### Newcomers

If this is your first time contributing to GIMP, you might be interested
by [build instructions](https://developer.gimp.org/core/setup/).

You might also like to read these instructions on the process of
[submitting patches](https://developer.gimp.org/core/submit-patch/).

### TODO: Core Contributors

As a core dev, you can trigger .appimage, .flatpak standalone packages,
.exe Windows installer or Microsoft Store/.msixbundle to be generated
with the MR code as explained in [gitlab-mr.md](gitlab-mr.md).

### Directory structure of GIMP source tree

GIMP source tree can be divided into the main application, libraries, plug-ins,
data files and some stuff that don't fit into these categories. Here are the
top-level directories:

| Folder          | Description |
| ---             | ---         |
| app/            | Source code of the main GIMP application             |
| app-tools/      | Source code of distributed tools                     |
| build/          | Scripts for creating binary packages                 |
| data/           | Data files: dynamics, gradients, palettes…           |
| desktop/        | Desktop integration files                            |
| devel-docs/     | Developers documentation                             |
| docs/           | Users documentation                                  |
| etc/            | Configuration files installed with GIMP              |
| extensions/     | Source code of extensions                            |
| gimp-data/      | Raster or image data files                           |
| libgimp/        | Library for plug-ins (core does not link against)    |
| libgimpbase/    | Basic functions shared by core and plug-ins          |
| libgimpcolor/   | Color-related functions shared by core and plug-ins  |
| libgimpconfig/  | Config functions shared by core and plug-ins         |
| libgimpmath/    | Mathematic operations useful for core and plug-ins   |
| libgimpmodule/  | Abstracts dynamic loading of modules (used to implement loadable color selectors and display filters) |
| libgimpthumb/   | Thumbnail functions shared by core and plug-ins      |
| libgimpwidgets/ | User interface elements (widgets) and utility functions shared by core and plug-ins                   |
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

#### Auto-generated Files

Please notice that some files in the source are generated from other
sources. All those files have a short notice about being generated
somewhere at the top. Among them are the files ending in `pdb.[ch]` in
the `libgimp/` directory and the files ending in `cmds.c` in the
`app/pdb/` subdirectory. Those are generated from the respective `.pdb`
files in `pdb/groups`.

Other files are:

* `AUTHORS` from `authors.xml`

You should also check out [gimp-module-dependencies.svg](gimp-module-dependencies.svg).
**TODO**: this SVG file is interesting yet very outdated. It should not
be considered as some kind dependency rule and should be updated.

### Advanced concepts

#### XCF

The `XCF` format is the core image format of GIMP, which mirrors
features made available in GIMP. More than an image format, you may
consider it as a work or project format, as it is not made for finale
presentation of an artwork but for the work-in-progress process.

Developers are welcome to read the [specifications of XCF](https://developer.gimp.org/core/standards/xcf/).

#### Locks

Items in an image can be locked in various ways to prevent different
types of edits.

This is further explained in [the specifications of locks](https://developer.gimp.org/core/specifications/locks/).

#### UI Framework

GIMP has an evolved GUI framework, with a toolbox, dockables, menus…

This document describing how the GIMP UI framework functions and how it
is [implemented](ui-framework.txt) might be of interest.

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

This [old document](https://developer.gimp.org/core/specifications/exif_handling/)
might be of interest (or maybe not, it has not been recently reviewed and might
be widely outdated; in any case, it is not a complete document at all as we
definitely do a lot more nowadays). **TODO**: review this document and delete or
update it depending of whether it still makes sense.

#### Tagging

Various data in GIMP can be tagged across sessions.

This document on [how resource tagging in GIMP works](tagging.txt) may
be of interest.
