# Color in ScriptFu

## About

The audience is script authors and developers of ScriptFu.

For more information, see the test script script-fu/test/tests/pdb/color.scm.
The tests demonstrate exactly how ScriptFu behaves.

## Representation in Scheme

Colors are represented by lists of numerics, or string names or notation.

### List representation

A color *from* GIMP is represented in ScriptFu by a list of from one to four numerics,
depending on the image mode.
The numerics are "components" of the color.

You can also represent colors by lists of numeric in calls *to* the PDB.

Examples:

  - RGBA (0 0 0 0)
  - RGB (0 0 0)
  - Grayscale with alpha (0 0)
  - Grayscale (0)


### String name representation

Color names can be used in PDB calls *to* GIMP.  Color names must be CSS names.

You can also represent default colors by name
when declaring script arguments.

#### Name "transparent"

The color name "transparent" is a CSS color name.

You can use color named by "transparent" in calls to the PDB.
When setting the context foreground color,
it sets to opaque black.
When setting pixel of an RGBA image, the pixel
is set to black transparent.

### Other CSS color notation

You can also use other string notation to denote a color.

Examples:

  - "#FFFFFF"  hexadecimal notation for RGB
  - "hsl(0, 0, 0)" converter function call from HSL values

Note that ScriptFu still converts to RGB format.

### CSS names/notation and grayscale images

You can use CSS names in procedures on grayscale images.
GIMP converts RGB colors to grayscale.

## Colorspace and Precision

While the GIMP app supports many colorspaces and precisions,
ScriptFu does not.

Generally speaking, ScriptFu has limited support for dealing with
colorspace and precision.
Scripts may call any procedures in the PDB that deal with colorspace
and precision.
But ScriptFu does not handle pixels/colors in high precision
or in colorspaces other than RGB and grayscale.
ScriptFu is not designed for intense and complicated pixel-level operations.

### Colorspace

ScriptFu keeps colors in the sRGB colorspace,
or grayscale.

GIMP converts colorspace when a ScriptFu script operates on images in other colorspaces.

### Precision

ScriptFu keeps colors with 8-bit integer precision.
This means a script may suffer loss of precision
when processing images of higher precision.

ScriptFu clamps numerics passed to the PDB as color components
to the range [0, 255].
Numeric color components received from GIMP are also integers in the range [0,255].

Float numerics may be passed as components of a color,
but are converted to integers and clamped.

## Difference from ScriptFu version 2

Version 2 represented colors differently,
converting to and from C type GimpRGB, which is obsolete.

Most scripts using colors will continue to work,
when they use colors opaquely,
receiving and passing colors without looking inside them.
It is only scripts that deal with color components
that might need to be ported.

In version 2, gimp-drawable-get-pixel
returned a count of components and a list of components e.g. (3 (255 255 255)).
Now it just returns a list of components,
and the length of the list correlates with format length.

## The GeglColor type

In the PDB, the type of colors is GeglColor.
When you browse the PDB, you see this type.
Arguments of this type are the "colors"
that ScriptFu represents as lists of numerics.

GeglColor is a class i.e. object in GIMP.
There are no methods in the PDB API for GeglColor.
For example in ScriptFu you can't get the format of a GeglColor.
You can get the length of the list representation to know the format.

## Indexed images

Colors to and from indexed images are colors, not indexes.
That is, in ScriptFu, a color from an indexed image is
a list of four numerics, RGBA.


