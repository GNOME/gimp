====================================
SPECIFICATION OF THE XCF FILE FORMAT
====================================

Copyright Henning Makholm <henning@makholm.net>, 2006-07-11

This is free documentation; you can modify and/or redistribute
it according to the terms of the GNU General Public License
as published by the Free Software Foundation, either version
2 of the license, or (at your option) any later version.

     ---------------------------------------------
     T H I S    I S    A    D R A F T    O N L Y !
     ---------------------------------------------

This is the native image file format of GIMP. Beware that
CinePaint's native file format is called XCF too. While the latter is
derived from the format described here, the two formats differ in many
details and are not mutually compatible.

The XCF format is designed to store the entire part of the state of
GIMP that is specific to one image (i.e., not the cut buffer, tool
options, key bindings, etc.) and is not undo data.  This makes the
full collection of data stored in an XCF file rather heterogeneous and
tied to the internals of GIMP. Use of the format by third-party
software is recommended only as a way to get data into and out of the
Gimp for which it would be impossible or inconvenient to use a more
standard interchange format.

Authors of third-party XCF-creating software in particular should take
care to write files that are as indistinguishable as possible from
ones saved by GIMP. The GIMP developers take care to make each
version of GIMP able to read XCF files produced by older versions,
but they make no special efforts to allow reading of files created by
other software that attempt to extrapolate from the GIMP source.

The name
--------

The name XCF honors GIMP's origin at the eXperimental Computing
Facility of the University of California at Berkeley.

Status of this document
-----------------------

This specification is an unofficial condensation and extrapolation of
the XCF-writing and -reading code in version 2.2.11 of GIMP. As of
this writing, it has not been approved or proofread by any Gimp
developer, though it has been written with the intention of
contributing it to the GIMP project for use as official documentation.

Some of the normative statements made below are enforced by the XCF
reader in GIMP; others are just the author's informed guess about
"best practices" that would be likely to maximize interoperability
with future versions of GIMP.


1. BASIC DATA MODEL
===================

It is recommended that a software developer who wants to take full
advantage of the XCF format be deeply familiar with GIMP at least
as a user. The following high-level overview is meant to help those
non-users who just need to extract pixel data from an XCF file get up
to speed.

In general an XCF file describes a stack of _layers_ and _channels_ on
a _canvas_, which is just an abstract rectangular viewport for the
layers and channels.

Layers
------

A layer is a named rectangular area of pixels which has a definite
position with respect to the canvas. A layer may extend beyond the
canvas or (more commonly) only cover some of it. Each pixel of the
layer has a color which is specified in one of three ways:

  RGB: Three intensity values for red, green, and blue additive color
     components, each on a scale from 0 to 255. The exact color space
     is not specified. GIMP displays image data directly on PC
     display hardware without any software correction, so in most
     cases the intensity values should be considered nonlinear samples
     that map to physical light intensities using a power function
     with an exponent ("gamma") of about 2.5. (This is how PC hardware
     commonly treat bit values in the video buffer, which incidentally
     also has the property of making each 1/255th step about equally
     perceptible to the human eye when the monitor is correctly
     adjusted).
     Beware, however, that GIMP's compositing algorithms (as
     described in Section 8 below) implicitly treat the intensities
     as _linear_ samples. The XCF file format currently has no support
     for storing the intended gamma of the samples.

  Grayscale: One intensity value on a scale from 0 (black) to 255
     (white). Gamma considerations as for RIB.

  Indexed: An 8-bit index into a colormap that is shared between all
     layers. The colormap maps each index to an RGB triple which is
     interpreted as in the RGB model.

All layers in an image must use the same color model. Exception: if
the "floating selection" belongs to a channel or layer mask, it will
be represented as grayscale pixels independently of the image's
overall color model.

Each pixel of a layer also has an alpha component which specifies the
opacity of the pixel on a linear scale from 0 (denoting an alpha of
0.0, or completely transparent) to 255 (denoting an alpha of 1.0, or
completely opaque). The color values do not use "premultiplied alpha"
storage. The color information for pixels with alpha 0 _may_ be
meaningful; GIMP preserves it when parts of a layer are erased and
provides (obscure) ways of recovering it in its user interface.

The bottommost layer _only_ in an image may not contain alpha
information; in this case all pixels in the layer have an alpha value
of 255. (Even if the bottommost layer does not cover the entire
canvas, it is the only layer that can be without an explicit alpha
channel).

In images that use the indexed color model, GIMP does not support
partial transparency and interprets alpha values from 0 to 127 as
fully transparent and values from 128 to 255 as fully opaque. This
behavior _may_ change in future versions of GIMP.

Layers have certain other properties such as a visibility flag, a
global opacity (which is multiplied with individual pixel alphas) and
various editing state flags.

Channels
--------

A channel is a named object that contains a single byte of information
for each pixel in the canvas area. Channels have a variety of use as
intermediate objects during editing; they are not meant to be rendered
directly when the final image is displayed or exported to layer-less
formats.

A channel can be edited as if it was a grayscale layer with the same
dimensions as the canvas. When it is shown in the GIMP editor UI
together with other layers, it is used as if it was the _inverse_
alpha channel of a layer with the same color information in all
pixels; this color can be stored in the XCF file as a property of the
channel. This "mask" representation is generally thought of as an UI
feature rather than an intrinsic semantics of a channel.

The current _selection_ in the editor is stored as a channel in the
XCF file if it is nonempty. Pixels with a value of 255 belong to the
selection; pixels with a value of 0 don't, an pixels with intermediate
values are partially selected. A major use of channels is as a store
for saved selections.

Though the channel data structure in the XCF file contains a height
and width field, these must always be the same as the canvas width and
height.

Related to channels is the _layer mask_ that can be attached to a
layer. The layer mask is in fact represented as a channel structure in
the XCF file, but it is not listed in the master list of channels.
Its dimensions and placement coincide with those of its parent layer.

Unless disabled by the PROP_APPLY_MASK property, the layer mask
functions as an extra alpha channel for the layer, in that for each
pixel the layer's alpha byte and the layer mask byte are multiplied to
find the extent to which the layer obscures the background. Thus a
layer mask can make parts of the layer more transparent, but never
more opaque.


2. FORMAT CONCEPTS AND DATATYPES
================================

An XCF file is a sequence of bytes. It contains a series of data
structures, the order of which is in general not significant.  The
exception to this is that the main image structure must come at the
very beginning of the files, and that the tile data blocks for each
drawable must follow each other directly.

References _between_ structures in the XCF file take the form of
32-bit "pointers" that count the number of bytes between the beginning
of the XCF file and the beginning of the pointed-to structure.

Each structure is designed to be written and read sequentially; many
contain items of variable length and the concept of an offset _within_
a data structure is not often relevant.

Basic data types
----------------

A WORD is a 32-bit integer stored as 4 bytes in network byte order,
i.e. with the most significant byte first. The word is not necessarily
aligned to an offset within the XCF file that is a multiple of
4. Depending on the context the word can be unsigned or (2's
complement) signed. In this specification unsigned words are denoted
"uint32" and signed words are denoted "int32".

A FLOAT is stored as a 32-bit IEEE 754 single-precision floating-point
number in network byte order.

A STRING is stored as follows:

  uint32   n+1  The number of bytes that follow, including the zero byte
  byte[n]  ...  The string data in Unicode, encoded using UTF-8
  byte     0    A terminating zero byte

Exception: the empty string is stored simply as an uint32 with the
value 0.

Properties
----------

As an extension mechanism, most kinds of structures in an XCF file
include a variable-length series of variable-length PROPERTY records
which have the following general format

  uint32   t    A magic number that identifies the type of property
  uint32   n    They payload length (but BEWARE! see below)
  byte[n]  ...  Payload - interpretation depends on the type

The authoritative source for property type numbers is the file
app/xcf/xcf-private.h in the GIMP sources.

The number of properties in a property list is not stored explicitly;
the last property in the list is identified by having type 0; it must
have length 0.

XCF readers must skip and ignore property records of unrecognized
type, and the length word is there to support such skipping. However,
GIMP's own XCF reader will _ignore_ the length word of most
properties that it _does_ recognize, and instead reads the amount of
payload it knows this property to have. This means that a property
record is not itself extensible: one cannot piggyback extra data onto
an existing property record by increasing its length. Also, some
historical versions of GIMP actually stored the wrong length for
some properties, so there are XCF files with misleading property
length information in circulation. For maximal compatibility, an XCF
reader should endeavor to know the native lengths of as many
properties as possible and fall back to the length word only for truly
unknown property types.

There is not supposed to be more than one instance of each property in
a property list, but some versions of GIMP will erroneously emit
duplicate properties. An XCF reader that meets a duplicated property
should let the contents of the later instance take precedence, except
for properties that contain lists of subitems, in which the lists
should generally be concatenated. An XCF writer should never
deliberately duplicate properties within a single property list.

Parasites
---------

A second level of extensibility is provided by the "parasite"
concept. A parasite is analogous to a property (and is usually stored
in a special property in the XCF file) but is identified by a string
rather than a number. This makes a larger namespace available for
parasites.  Gimp plug-ins can access the parasites of an image
component through a generic API and can define their own parasite
names which will be ignored by other plug-ins. In contrast, only the
Gimp itself should define new property types.

A list of known parasites and their data formats can be found in the
file devel-doc/parasites.txt of the GIMP source tree.


3. THE MASTER IMAGE STRUCTURE
=============================

The image structure always starts at offset 0 in the XCF file.

  byte[9] "gimp xcf "  File type magic
  byte[4] version      XCF version
                         "file" - version 0
                         "v001" - version 1
                         "v002" - version 2
  byte    0            Zero-terminator for version tag
  uint32  width        With of canvas
  uint32  height       Height of canvas
  uint32  base_type    Color mode of the image; one of
                          0: RGB color
                          1: Grayscale
                          2: Indexed color
                       (enum GimpImageBaseType in libgimpbase/gimpbaseenums.h)
  property-list        Image properties (details below)
  ,------------------- Repeat once for each layer, topmost layer first:
  | uint32 layer       Pointer to the layer structure
  `--
  uint32   0           Zero marks the end of the array of layer pointers
  ,------------------- Repeat once for each channel, in no particular order:
  | uint32 channel1    Pointer to the channel structure
  `--
  uint32   0           Zero marks the end of the array of channel pointers

The last four characters of the initial 13-character magic string are
a version indicator. The version will be higher than 2 if the correct
reconstruction of pixel data from the file requires that the reader
understands features not described in this specification. On the other
hand, optional extra information that can be safely ignored will not
cause the version to increase.

GIMP's XCF writer dynamically selects the lowest version that will
allow the image to be represented. Third-party XCF writers should do
likewise.

Version numbers from v100 upwards have been used by CinePaint, which
originated as a 16-bit fork of GIMP. That format is not described
by this specification.

Image properties
----------------

The following properties are found only in the property list of the
image structure. In addition to these, the list can also contain the
properties PROP_TATTOO, PROP_PARASITES and PROP_END, described in
section 8 below.

PROP_COLORMAP (essential)
  uint32  1     The type number for PROP_COLORMAP is 1
  uint32  3n+4  The payload length
  uint32  n     The number of colors in the colormap (should be <256)
  ,------------ Repeat n times:
  | byte  r     The red component of a colormap color
  | byte  g     The green component of a colormap color
  | byte  b     The blue component of a colormap color
  `--

  Appears in all indexed images and stores the colormap. The property
  will be ignored if it is encountered in an RGB or grayscale
  image. The current GIMP will not write a colormap with RGB or
  grayscale images, but some older ones occasionally did, and readers
  should be prepared to gracefully ignore it in those cases.

  Note that in contrast to the palette data model of, for example, the
  PNG format, an XCF colormap does not contain alpha components, and
  there is no colormap entry for "transparent"; the alpha channel of
  layers that have one is always represented separately.

  The structure here is that of XCF version >= 1.  Comments in the
  GIMP source code indicate that XCF version 0 could not store indexed
  images in a sane way; contemporary Gimps will complain and
  reinterpret the pixel data as a grayscale image if they meet a
  version-0 indexed image.
  
  Beware that the payload length of the PROP_COLORMAP in particular
  cannot be trusted: some historic releases of GIMP erroneously
  wrote n+4 instead of 3n+4 into the length word (but still actually
  followed it by 3n+4 bytes of payload).

PROP_COMPRESSION (essential)
  uint32  17  The type number for PROP_COMPRESSION is 17
  uint32  1   One byte of payload
  byte    c   Compression indicator; one of
                0: No compression
                1: RLE encoding
                2: (Never used, but reserved for zlib compression)
                3: (Never used, but reserved for some fractal compression)

  Defines the encoding of pixels in tile data blocks in the entire XCF
  file. See section 6 below for details.

  Note that unlike most other properties whose payload is always a
  small integer, PROP_COMPRESSION does _not_ pad the value to a full
  32-bit integer.

  Contemporary Gimps always write files with c=1. It is unknown to the
  author of this document whether versions that wrote completely
  uncompressed (c=0) files ever existed.
  
PROP_GUIDES (editing state)
  uint32  18  The type number for PROP_GUIDES is 18
  uint32  5*n Five bytes of payload per guide
  ,---------- Repeat n times:
  | int32 c   Guide coordinate
  | byte  o   Guide orientation; one of
  |            1: The guide is horizontal, and c is a y coordinate
  |            2: The guide is vertical, and c is an x coordinate
  `--

  Appears if any guides have been defined.

  Some old XCF files define guides with negative coordinates; those
  should be ignored by readers.
   
PROP_RESOLUTION (not editing state, but not _really_ essential either)
  uint32  19  The type number for PROP_RESOLUTION is 19
  uint32  8   Eight bytes of payload
  float   x   Horizontal resolution in pixels per 25.4 mm
  float   y   Vertical resolution in pixels per 25.4 mm

  Gives the intended physical size of the image's pixels.

  Note that for many images, such as graphics created for the web, the
  creator does not really have an intended resolution in mind but
  intends the image to be shown at whatever the natural resolution of
  the viewer's monitor is. Similarly, photographs commonly do not have
  a well-defined target size and are intended to be scaled to fit the
  available space instead. Therefore readers should not interpret the
  information in this property too rigidly; GIMP writes it to XCF
  files unconditionally, even if the user has not explicitly chosen a
  resolution.
  
PROP_UNIT (editing state)
  uint32  22  The type number for PROP_UNIT is 22
  uint32  4   Four bytes of payload
  uint32  c   Magic number specifying the unit; one of
                1: Inches (25.4 mm)
                2: Millimeters (1 mm)
                3: Points (127/360 mm)
                4: Picas (127/30 mm)

  Specifies the units used to specify resolution in the Scale Image
  and Print Size dialogs.  Note that this is used only in the user
  interface; the PROP_RESOLUTION property is always stored in pixels
  per 25.4 mm.

  Instead of this property, PROP_USER_UNIT can be used to specify a
  unit not on the list of standard units.

PROP_PATHS
  uint32    23   The type number for PROP_PATHS is 23
  uint32    len  The total length of the following payload
  uint32    a    The index of the active path
  uint32    n    The number of paths that follow
    path_1
    path_2
    ...
    path_n

  Each path has one of three formats

    Format 1:   Format 2:   Format 3:
     string      string      string     name
     uint32      uint32      uint32     locked
     byte        byte        byte       state    4 if closed, 2 otherwise
     uint32      uint32      uint32     closed
     uint32      uint32      uint32     n        The number of points
     uint32=1    uint32=2    uint32=3   v        A version indicator
                 uint32      uint32     dummy    Ignored; always set to 1
                             uint32     tattoo   0 if none, or see PROP_TATTOO
     ,---------- ,---------- ,------------------ Repeat for n points:
     | int32     | int32     | int32    typ      Type of point; one of
     |           |           |                    0: an anchor
     |           |           |                    1: a Bezier control point
     | int32     | float     | float    x        X coordinate
     | int32     | float     | float    y        Y coordinate
     `--         `--         `--

  This format is used to save path data if all paths in the image are
  continuous sequences of Bezier strokes. Otherwise PROP_VECTORS is
  used instead.

  (Hmmm... PROP_PATHS cannot represent parasites for paths, but the
  XCF writer does not check whether all paths are parasite-less when
  choosing which property to use, so path parasites may be lost upon
  saving).
  
  There may be paths that declare a length of 0 points; these should
  be ignored.
  
PROP_USER_UNIT (editing state)
  uint32  24      The type number for PROP_USER_UNIT is 24
  uint32  length  The total length of the following payload
  float   factor  25.4 mm divided by the length of the unit
  uint32  digits  The number of decimal digits used with the unit
  string  id      An identifier for the unit
  string  sym     The short symbol for the unit
  string  abbr    The abbreviation for the unit
  string  sing    The name of the unit in singular
  string  plur    The name of the unit in plural

  An alternative to PROP_UNIT which allows the use of units not on the
  standard list.
  
PROP_VECTORS
  uint32       25         The type number for PROP_VECTORS is 25
  uint32       length     The total length of the following payload
  uint32       1          A version tag; so far always 1
  uint32       a          The index of the active path
  uint32       n          The number of paths that follow
  ,---------------------- Repeat n times:
  | string     name       Name of the path
  | uint32     tattoo     Tattoo of the path (see PROP_TATTOO), or 0
  | uint32     visible
  | uint32     linked
  | uint32     m          The number of parasites for the path
  | uint32     k          The number of strokes in the first path
  | ,-------------------- Repeat m times:
  | | parasite ...        In same format as in PROP_PARASITES (q.v.)
  | `--
  | ,-------------------- Repeat k times:
  | | uint32   1          The stroke is a Bezier stroke (p.t., all are)
  | | uint32   closed     Closed flag
  | | uint32   na         Number of floats given for each point;
  | |                     must be >= 2 and <= 6.
  | | uint32   np         Number of control points for this stroke
  | | ,------------------ Repeat np times:
  | | | uint32 type       Type of the first point; one of
  | | |                     0: an anchor
  | | |                     1: a Bezier control point
  | | | float  x          X coordinate
  | | | float  y          Y coordinate
  | | | float  pressure   Only if na >= 3; otherwise defaults to 1.0
  | | | float  xtilt      Only if na >= 4; otherwise defaults to 0.5
  | | | float  ytilt      Only if na >= 5; otherwise defaults to 0.5
  | | | float  wheel      Only if na == 6; otherwise defaults to 0.5
  | | `--
  | `--
  `--
  
  Appears unless all paths can be stored in the PROP_PATHS format.

  GIMP's XCF reader checks that the total size of all path
  specifications in the property precisely equals the length word, so
  it is safe for a reader to use the length word to skip the property
  without parsing the individual parasites. (Note that this is _not_
  the case for PROP_PATHS).


4. THE LAYER STRUCTURE
======================

Layer structures are pointed to from a list of layer pointers in the
master image structure.

  uint32  width  The width of the layer
  uint32  height The height of the layer
  uint32  type   Color mode of the layer: one of
                   0: RGB color without alpha; 3 bytes per pixel
                   1: RGB color with alpha;    4 bytes per pixel
                   2: Grayscale without alpha; 1 byte per pixel
                   3: Grayscale with alpha;    2 bytes per pixel
                   4: Indexed without alpha;   1 byte per pixel
                   5: Indexed with alpha;      2 bytes per pixel
                 (enum GimpImageType in libgimpbase/gimpbseenums.h)
  string  name   The name of the layer
  property-list  Layer properties (details below)
  uint32  hptr   Pointer to the hierarchy structure containing the pixels
  uint32  mptr   Pointer to the layer mask (a channel structure), or 0

The color mode of a layer must match that of the entire image.  All
layers except the bottommost one _must_ have an alpha channel.

Exception: If the layer is a floating selection (see
PROP_FLOATING_SELECTION) and is attached to a channel or layer mask,
its color mode must be 3 (grayscale with alpha).

Layer properties
----------------

The following properties are found only in the property list of layer
structures. In addition to these, the list can also contain the
properties PROP_OPACITY, PROP_VISIBLE, PROP_LINKED, PROP_TATTOO,
PROP_PARASITES, and PROP_END, defined in section 8 below.

PROP_ACTIVE_LAYER (editing state)
  uint32  2    The type number for PROP_ACTIVE_LAYER is 2
  uint32  0    PROP_ACTIVE_LAYER has no payload

  Appears in the property list for the currently active layer. Only
  one layer must have this property.

PROP_FLOATING_SELECTION (essential)
  uint32  5    The type number for PROP_FLOATING_SELECTION is 5
  uint32  4    Four bytes of payload
  uint32  ptr  Pointer to the layer or channel that the floating
               selection is attached to.

  Appears in the property list for the layer that is the floating
  selection. If a floating selection exists, it must always be the
  first layer in the layer list, but it is not rendered at that
  position in the layer stack. Instead it is logically attached to
  another layer, or a channel or layer mask, and the contents of the
  floating selection is combined with ("anchored to") that drawable
  before it is used to render the visible image.

  The floating selection must not have a layer mask of its own, but if
  an ordinary (not floating) selection also exists, it will be used as
  a layer mask for the floating selection.

  If a floating selection exists, it must also be the active layer.

  Because floating selections are modal and ephemeral, users rarely
  save XCF files containing a floating selection. It may be acceptable
  for third-party XCF consumers to ignore floating selections or
  explicitly refuse to process them.

PROP_MODE (essential)
  uint32  7   The type number for PROP_MODE is 7
  uint32  4   Four bytes of payload
  unit32  m   The layer mode; one of
                 0: Normal
                 1: Dissolve (random dithering to discrete alpha)
                 2: (Behind: not selectable in the GIMP UI)
                 3: Multiply
                 4: Screen
                 5: Overlay
                 6: Difference
                 7: Addition
                 8: Subtract
                 9: Darken Only
                 10: Lighten Only
                 11: Hue (H of HSV)
                 12: Saturation (S of HSV)
                 13: Color (H and S of HSL)
                 14: Value (V of HSV)
                 15: Divide
                 16: Dodge
                 17: Burn
                 18: Hard Light
                 19: Soft Light (XCF version >= 2 only)
                 20: Grain Extract (XCF version >= 2 only)
                 21: Grain Merge (XCF version >= 2 only)
                 
  When reading old XCF files that lack this property, assume m=0.
  The effects of the various layer modes is defined in Section 8, below.

  Beware that GIMP ignores all other layer modes than Normal and
  Dissolve for the bottommost visible layer of the image; if m>=3 has
  been specified for this layer it will interpreted as m==0 for
  display and flattening purposes. This effect happens for one layer
  only: even if the bottommost visible layer covers only some (or
  none) of the canvas, it will be the only layer to have its mode
  forced to Normal.

PROP_PRESERVE_TRANSPARENCY (editing state)
(called PROP_LOCK_ALPHA in Gimp 3.3+)
  uint32  10  The type number for PROP_PRESERVE_TRANSPARENCY is 10
  uint32  4   Four bytes of payload
  uint32  b   1 if the Preserve Transparency flag is set; 0 if not

  The Preserve Transparency flag prevents all drawing tools in the
  Gimp from increasing the alpha of any pixel in the layer.

PROP_APPLY_MASK (essential)
  uint32  11  The type number for PROP_APPLY_MASK is 11
  uint32  4   Four bytes of payload
  uint32  b   1 if the layer mask should be applied, 0 if not

  If the property does not appear for a layer which has a layer mask,
  it defaults to true.

  Robust readers should force this to false if the layer has no layer
  mask. Writers should never save this as true unless the layer has a
  layer mask.

PROP_EDIT_MASK (editing state)
  uint32  12  The type number for PROP_EDIT_MASK is 12
  uint32  4   Four bytes of payload
  uint32  b   1 if the layer mask is currently being edited, 0 if not

  If the property does not appear for a layer which has a layer mask,
  it defaults to false.

  Robust readers should force this to false if the layer has no layer
  mask. Writers should never save this as true unless the layer has a
  layer mask.

PROP_SHOW_MASK (editing state)
  uint32  13  The type number for PROP_APPLY_MASK is 13
  uint32  4   Four bytes of payload
  uint32  b   1 if the layer mask is visible, 0 if not

  If the property does not appear for a layer which has a layer mask,
  it defaults to false.

  Robust readers should force this to false if the layer has no layer
  mask. Writers should never save this as true unless the layer has a
  layer mask.

PROP_OFFSETS (essential)
  uint32  15  The type number for PROP_OFFSETS is 15
  uint32  8   Eight bytes of payload
  int32   dx  Horizontal offset
  int32   dy  Vertical offset

  Gives the coordinates of the topmost leftmost corner of the layer
  relative to the topmost leftmost corner of the canvas. The
  coordinates can be negative; this corresponds to a layer that
  extends to the left of or above the canvas boundary.

  When reading old XCF files that lack this property, assume (0,0).
    
PROP_TEXT_LAYER_FLAGS
  uint32  26  The type number for PROP_TEXT_LAYER_FLAGS is 26
  uint32  4   Four bytes of payload
  uint32  f   Flags, or'ed together from the following set:
               0x00000001  Do _not_ change the layer name if the text
                           contents is changed
               0x00000002  The pixel data has been painted to or
                           otherwise modified since the text was rendered
  
  Appears in property lists for text layers. The actual text (and
  other parameters such as font and color) appears as a parasite
  rather than a property.

                   
5. THE CHANNEL STRUCTURE
========================

Channel structures are pointed to from layer structures (in case of
layer masks) or from the master image structure (for all other
channels).

  uint32  width  The width of the channel
  uint32  height The height of the channel
  string  name   The name of the channel
  property-list  Layer properties (see below)
  uint32  hptr   Pointer to the hierarchy structure containing the pixels  

The with and height of the channel must be the same as those of its
parent structure (the layer in the case of layer masks; the canvas for
all other channels).

Channel properties
------------------

The following properties are found only in the property list of
channel structures. In addition to these, the list can also contain
the properties PROP_OPACITY, PROP_VISIBLE, PROP_LINKED, PROP_TATTOO,
PROP_PARASITES, and PROP_END, defined in section 8 below.

PROP_ACTIVE_CHANNEL (editing state)
  uint32  3    The type number for PROP_ACTIVE_CHANNEL is 3
  uint32  0    PROP_ACTIVE_CHANNEL has no payload

  Appears in the property list for the currently active channel.

PROP_SELECTION (editing state?)
  uint32  4    The type number for PROP_SELECTION is 4
  uint32  0    PROP_SELECTION has no payload

  Appears in the property list for the channel structure that
  represents the selection mask.
  
PROP_SHOW_MASKED (editing state)
  uint32  14  The type number for PROP_SHOW_MASKED is 14
  uint32  4   Four bytes of payload
  uint32  b   1 if the channel is shown as a mask, 0 if not

  Appears in the property list for channels.

PROP_COLOR
  uint32  16  The type number for PROP_COLOR is 16
  uint32  3   Three bytes of payload
  byte    r   Red component of color
  byte    g   Green component of color
  byte    b   Blue component of color

  Appears in the property list for channels and gives the color of the
  screen that is used to represent the channel when it is visible in
  the UI. (The alpha of the screen is given as the channel's
  PROP_OPACITY).


6. THE HIERARCHY STRUCTURE
==========================

A hierarchy contains data for a rectangular array of pixels. It is
pointed to from a layer or channel structure.

  uint32   width   The with of the pixel array
  uint32   height  The height of the pixel array
  uint32   bpp     The number of bytes per pixel given
  uint32   lptr    Pointer to the "level" structure
  ,--------------- Repeat zero or more times
  | uint32 dlevel  Pointer to an unused level structure
  `--
  uint32   0       A zero ends the list of level pointers

The width, height, and bpp values are for consistency checking; their
correct values can always be inferred from the context, and are
checked when GIMP reads the XCF file.

Levels
------

For some unknown historical reason, the hierarchy structure contains
an extra indirection to a series of "level" structures, described
below. Only the first level structure is used by GIMP's XCF
reader, except that the reader checks that a terminating zero for the
level-pointer list can be found. GIMP's XCF writer creates a
series of dummy level structures (with no tile pointers), each
declaring a height and width half of the previous one (rounded down),
until the height and with are both less than 64. Thus, for a layer of
3 x 266 pixels, this series of levels will be saved:

   A level of 3 x 266 pixels, with 5 tiles: the actually used one
   A level of 1 x 133 pixels with no tiles
   A level of 0 x 66 pixels with no tiles
   A level of 0 x 33 pixels with no tiles

Third-party XCF writers should probably mimic this entire structure;
robust XCF readers should have no reason to even read past the pointer
to the first level structure.

The level structure is laid out as follows:

  uint32   width  The width of the pixel array
  uint32   height The height of the pixel array
  ,-------------- Repeat for each of the ceil(width/64)*ceil(height/64) tiles
  | uint32 tptr   Pointer to tile data
  `--
  uint32   0      A zero marks the end of the array of tile pointers

The width and height must be the same as the ones recorded in the
hierarchy structure (except for the aforementioned dummy levels).

Tiles
-----

The pixels in the hierarchy are organized in a grid of "tiles", each
with a width and height of up to 64 pixels. The only tiles that have a
width less than 64 are those in the rightmost column, and the only
tiles that have a height less than 64 are those in the bottommost row.
Thus, a layer measuring 200 x 150 pixels will be divided into 12
tiles:

 +-----------------+-----------------+------------------+-----------------+
 | Tile 0: 64 x 64 | Tile 1: 64 x 64 | Tile 2: 64 x 64  | Tile 3: 8 x 64  |
 +-----------------+-----------------+------------------+-----------------+
 | Tile 4: 64 x 64 | Tile 5: 64 x 64 | Tile 6: 64 x 64  | Tile 7: 8 x 64  |
 +-----------------+-----------------+------------------+-----------------+
 | Tile 8: 64 x 22 | Tile 9: 64 x 22 | Tile 10: 64 x 22 | Tile 11: 8 x 22 |
 +-----------------+-----------------+------------------+-----------------+

As can be seen from this example, the tiles appear in the XCF file in
row-major, top-to-bottom, left-to-right order. The dimensions of the
individual tiles are not stored explicitly in the XCF file, but must
be computed by the reader.

The tiles that are pointed to by a single level structure must be
contiguous in the XCF file, because GIMP's XCF reader uses the
difference between two subsequent tile pointers to judge the amount of
memory it needs to allocate for internal data structures.


7. TILE DATA ORGANIZATION
=========================

The format of the data blocks pointed to by the tile pointers in the
level structure of the previous section differs according to the value
of the PROP_COMPRESSION property of the main image structure. Current
Gimps always use RLE compression, but readers should nevertheless be
prepared to meet the older uncompressed format.

Both formats assume the width, height and byte depth of the tile are
known from the context (namely, they are stored explicitly in the
hierarchy structure). Both encodings store a linear sequence of
with*height pixels, extracted from the tile in row-major,
top-to-bottom, left-to-right order (the same as the reading direction
of multi-line English text).

In color modes with alpha information, the alpha value is the last of
the 2 or 4 bytes for each pixel. In RGB color modes, the 3 (first)
bytes for each pixel is the red intensity, the green intensity, and
the blue intensity, in that order.

Uncompressed tile data
----------------------

In the uncompressed format, the file first contains all the bytes for
the first pixel, then all the bytes for the second pixel, and so on.

RLE compressed tile data
------------------------

In the Run-Length Encoded format, each tile consists of a run-length
encoded stream of the first byte of each pixel, then a stream of the
second byte of each pixel, and so forth. In each of the streams,
multiple occurrences of the same byte value are represented in
compressed form. The representation of a stream is a series of
operations; the first byte of each operation determines the format and
meaning of the operation:

  byte          n     For 0 <= n <= 126: a short run of identical bytes
  byte          v     Repeat this value n+1 times
or
  byte          127   A long run of identical bytes
  byte          p
  byte          q
  byte          v     Repeat this value p*256 + q times
or
  byte          128   A long run of different bytes
  byte          p
  byte          q
  byte[p*256+q] data  Copy these verbatim to the output stream
or
  byte          n     For 129 <= n <= 255: a short run of different bytes
  byte[256-n]   data  Copy these verbatim to the output stream

The end of the stream for "the first byte of all pixels" (and the
following similar streams) must occur at the end of one of these
operations; it is not permitted to have one operation span the
boundary between streams.

The RLE encoding allows "degenerate" encodings in which the original
data stream may double in size (or grow to arbitrarily large sizes if
(128,0,0) operations are inserted). Such encodings must be avoided, as
GIMP's XCF reader expects that the size of an encoded tile is
never more than 24 KB, which is only 1.5 times the unencoded size of a
64x64 RGBA tile.

A simple way for an XCF creator to avoid overflow is
 a) never using opcode 0 (but instead opcode 255)
 b) using opcodes 127 and 128 only for lengths larger than 127
 c) never emitting two "different bytes" opcodes next to each other
    in the encoding of a single stream.


8. GENERIC PROPERTIES
=====================

This section lists the formats of the defined property records that
can appear in more than one context in an XCF file.

PROP_END
  uint32  0   The type number for PROP_END is 0
  uint32  0   PROP_END has no payload

  This pseudo-property marks the end of a property list in the XCF
  file.
  
PROP_OPACITY (essential)
  uint32  6   The type number for PROP_OPACITY is 6
  uint32  4   Four bytes of payload
  uint32  x   The opacity on a scale from 0 (fully transparent) to 255
              (fully opaque)

  Appears in the property list of layers and channels, and records the
  overall opacity setting for the layer/channel. Note that though the
  Gimp's user interface displays the opacity as a percentage, it is
  actually stored on a 0-255 scale. Also note that this opacity value
  is stored as a 32-bit quantity even though it has been scaled to
  fit exactly in a single byte.

  When reading old XCF files that lack this property, full opacity
  should be assumed.

PROP_VISIBLE (essential)
  uint32  8   The type number for PROP_VISIBLE is 8
  uint32  4   Four bytes of payload
  uint32  b   1 if the layer/channel is visible; 0 if not

  Appears in the property list for layers and channels.
  When reading old XCF files that lack this property, assume that
  layers are visible and channels are not.

PROP_LINKED (editing state)
  uint32  9   The type number for PROP_LINKED is 9
  uint32  4   Four bytes of payload
  uint32  b   1 if the layer is linked; 0 if not

  Appears in the property list for layers and channels.  "Linked" is a
  UI property: if the Move tool is used to move a linked layer, all
  other linked layers will be moved in parallel.

PROP_TATTOO (internal Gimp state)
  uint32  20  The type number for PROP_TATTOO is 20
  uint32  4   Four bytes of payload
  uint32  4   The tattoo, a nonzero unsigned integer

  Appears in the property list of layers, channels, and entire images.

  A tattoo is a unique and permanent identifier attached to a drawable
  (or vector element) that can be used to uniquely identify a drawable
  within an image even between sessions.

  The PROP_TATTOO property of the entire image stores a "high-water
  mark" for the entire image; it is greater than OR EQUAL TO any
  tattoo for an element of the image. It allows efficient generation
  of new unused tattoo values and also prevents old tattoo numbers
  from being reused within a single image, lest plug-ins that use
  the tattoos for bookkeeping get confused.

  An XCF file must either provide tattoo values for all its elements
  or for none of them. GIMP will invent fresh tattoos when it
  reads in tattoo-less elements, but it does not attempt to keep them
  different from ones specified explicitly in the file. 

PROP_PARASITES
  uint32    21      The type number for PROP_PARASITES is 21
  uint32    length  Total length of the following payload data
  ,---------------- Repeat for each parasite:
  | string  name    Name of the parasite
  | uint32  flags   ?????
  | uint32  n       Size of the payload data
  | byte[n] ...     Parasite-specific payload
  `--

  This property can appear in any property list. It can contain
  multiple "parasites" which are named extension records. See "Basic
  concepts and datatypes" above. The number of parasites is not
  directly encoded; the list ends when the total length of the
  parasite data read equals the property payload length.

  GIMP's XCF reader checks that the combined size of all parasites
  in the property precisely equals the length word, so it is safe for
  a reader to use the length word to skip the property without parsing
  the individual parasites.

  The parasite contents may be binary, but often a textual encoding is
  chosen in order to free the creator/consumer code of having to deal
  with byte ordering.

  There can only be one parasite with a given name attached to
  each element of the image. Some versions of GIMP will
  erroneously write some parasites twice in the same property list;
  XCF readers must be prepared to gracefully ignore all but the
  last instance of a parasite name in each property list.


9. COMPOSITING AND LAYER MODES
===============================

This section describes the "flattening" process that GIMP applies
when a multi-layer image is displayed in the editor or exported to
other image file formats. It is present for reference only; an XCF
consumer may well elect to do something different with pixel data from
the layers than flattening them.

Most XCF consumers will need to react to the layer mode property of
each layer; such a reaction must be informed by knowledge of how the
different layer modes affect the flattening process. In some
applications it might be acceptable for an XCF consumer to refuse
processing images with layer modes other than "Normal", but such an
application will probably not be considered fully XCF capable by its
users.

In this section we consider primary color (or grayscale) intensities
and alpha values for pixels to be real numbers ranging from 0.0 to
1.0. This makes many of the formulas easier; the reader is asked to
keep in mind that a (linear) conversion from the integral 0..255 scale
of the actual XCF scale is implied whenever data from the XCF file is
mentioned.

Any practical implementation of the computations specified below may
suffer rounding errors; this specification do not detail how these are
to be handled. GIMP itself rounds values to an integral number of
255ths at many points in the computation. This specification does not
specify exactly which these points are, and authors of XCF renderers
that aim to reproduce the effects of GIMP's flattening down to the
least significant bits are referred to studying its source code.

In the description below, the variable letter "a" is used for alpha
values. The variable letter "r", "g", "b" are used for primary
intensities, "y" is used for grayscale intensities, and "i" is used
for colormap indexed. The letter "c" is used for the complete
color information for a pixel; depending on the color mode of the
image that is either an (r,g,b) triple, a y, or a c.

The flattening process works independently for each pixel in the
canvas area. The description of some layer modes in the GIMP manual
may give the impression that they involve filters that let pixels
influence neighbor pixels, but that is not true.

This description does not attempt to preserve the color information
for completely transparent pixels in a layer. If an application uses
this color information, it should document explicitly how it behaves
when transparent pixels from several different layers cover the same
point of the canvas.

Flattening overview
-------------------

This is how to compute the flattened result for a single pixel
position (in theory, that is - efficient implementations will of
course follow this procedure or an equivalent one for many pixels in
parallel):

1. Initialize a "working pixel" (a1,c1) to be completely transparent
   (that is, a1=0.0 and the value of c1 is immaterial).
   
2. Do the following for each visible layer in the image, starting with
   the one that comes LAST in the master layer list:

   3. Ignore the layer if it is the floating selection, or if it
      does not overlap the pixel position in question.

   4. Let (a2,c2) be the pixel data for the layer at the pixel
      position in question. If the layer does not have an alpha
      channel, then set a1 to 1.0.

   5. If the layer is the one that the floating selection is attached
      to and the floating selection overlaps the pixel position in
      question, then do the following:

      6. Let (a3,c3) be the pixel data for the floating selection
         layer at the pixel position in question.

      7. If there is a selection channel, then let x be its value
         at the pixel position in question, and set a3 to a3*x.

      8. Let m3 be the layer mode of the floating selection.
         
      9. Set (a2,c2) to COMPOSITE(a2,c2, a3,c3,m3).
         The COMPOSITE function is defined below.

   10. If the layer has a layer mask and it is enabled, then let x be
       the value of the layer mask at the pixel position in question,
       and set a2 to a2*x.

   11. Let m2 be the layer mode of the layer.

   12. If the layer is the bottommost visible layer (i.e., if it is
       the last visible layer in the master layer list) and m2 is not
       "Normal" or "Dissolve", then set m2 to "Normal".

   13. Set (a1,c1) to COMPOSITE(a1,c1, a2,c2,m2).
       The COMPOSITE function is defined below.

14. If the flattened image is to be shown against a background of
    color c0, then actually visible pixel is
    COMPOSITE(1.0,c0, a1,c1,Normal).

    Note that unless all layers have mode Normal, it would give the
    wrong result to start by initializing (a1,c1) to (1.0,c0).

Helper functions
----------------

The following auxiliary functions are used in the definition of
COMPOSITE below:

 MIN(x1,...,xn) is the least value of x1...xn
 
 MAX(x1,...,xn) is the largest value of x1..xn

 MID(x1,...,xn) = (MIN(x1,...,xn)+MAX(x1,...,xn))/2

 CLAMP(x) = if x < 0 then 0.0 else if x > 1 then 1.0 else x
 
 BLEND(a1,x1, a2,x2) = (1-k)*x1 + k*x2
                       where k = a2/(1-(1-a1)*(1-a2))

The layer modes
---------------

This and the following sections define the COMPOSITE function used in
the general flattening algorithm. 

"Normal" mode for RGB or grayscale images is the usual mode of
compositeing in computer graphics with alpha channels. In indexed
mode, the alpha value gets rounded to either 1.0 or 0.0 such that
no colors outside the colormap get produced:

  COMPOSITE(a1,y1, a2,y2,Normal)
     = ( 1-(1-a1)*(1-a2), BLEND(a1,y1, a2,y2) )

  COMPOSITE(a1,r1,g1,b1, a2,r2,g2,b2,Normal)
     = ( 1-(1-a1)*(1-a2), BLEND(a1,r1, a2,r2),
                          BLEND(a1,g1, a2,g2),
                          BLEND(a1,b1, a2,b2) )

  COMPOSITE(a1,i1, a2,i2,Normal) = if a2 > 0.5 then (1.0,i2) else (a1,i1)

"Dissolve" mode corresponds to randomly dithering the alpha channel to
the set {0.0, 1.0}:

  COMPOSITE(a1,c1, a2,c2,Dissolve) = chose pseudo-randomly between
                                     (1.0,c2) with probability a2
                                     (a1,c1)  with probability 1-a2

These two modes are the only ones that make sense for all of the RGB,
grayscale and indexed color models. In the indexed color model, all
layer modes except Dissolve are treated as Normal.

Most layer modes belong to the following group, which makes sense for
RGB and grayscale images, but not for indexed ones:

  COMPOSITE(a1,y2, a2,y2,m)
     = ( a1, BLEND(a1,y1, MIN(a1,a2),f(y1,y2, m)) )

  COMPOSITE(a1,r1,g1,b1, a2,r2,g2,b2,m)
     = ( a1, BLEND(a1,r2, MIN(a1,a2),f(r1,r2, m)),
             BLEND(a1,g1, MIN(a1,a2),f(g1,g2, m)),
             BLEND(a1,b1, MIN(a1,a2),f(b1,g2, m)) )

when 3 <= m <= 10 or 15 <= m <= 21.

The  following table defines f(x1,x2,m):

  Multiply:      f(x1,x2,  3) = x1*x2
  Screen:        f(x1,x2,  4) = 1-(1-x1)*(1-x2)
  Overlay:       f(x1,x2,  5) = (1-x2)*x1^2 + x2*(1-(1-x2)^2)
  Difference:    f(x1,x2,  6) = if x1 > x2 then x1-x2 else x2-x1
  Addition:      f(x1,x2,  7) = CLAMP(x1+x2)
  Subtract:      f(x1,x2,  8) = CLAMP(x1-x2)
  Darken Only:   f(x1,x2,  9) = MIN(x1,x2)
  Lighten Only:  f(x1,x2, 10) = MAX(x1,x2)
  Divide:        f(x1,x2, 15) = CLAMP(x1/x2)
  Dodge:         f(x1,x2, 16) = CLAMP(x1/(1-x2))
  Burn           f(x1,x2, 17) = CLAMP(1-(1-x1)/x2)
  Hard Light:    f(x1,x2, 18) = if x2 < 0.5 then 2*x1*x2 else 1-2*(1-x1)(1-x2)
  Soft Light:    f(x1,x2, 19) = (1-x2)*x1^2 + x2*(1-(1-x2)^2)
  Grain Extract: f(x1,x2, 20) = CLAMP(x1-x2+0.5)
  Grain Merge:   f(x1,x2, 21) = CLAMP(x1+x2-0.5)

Note that the "Overlay" and "Soft Light" modes have identical effects.
In the "Divide", "Dodge", and "Burn" modes, division by zero should
be considered to produce a number so large that CLAMP(x/0) = 1 unless
x=0, in which case CLAMP(0/0) = 0.

The remaining four layer modes only make sense in the RGB color model;
if the color mode of the image is grayscale or indexed they will be
interpreted as Normal.

  COMPOSITE(a1,r1,g1,b1, a2,r2,g2,b2,m)
     = ( a1, BLEND(a1,r2, MIN(a1,a2),r0),
             BLEND(a1,g1, MIN(a1,a2),g0),
             BLEND(a1,b1, MIN(a1,a2),b0) )
       where (r0,g0,b0) = h(r1,g1,b1, r2,g2,b2, m)

when 11 <= m <= 14.

For defining these modes, we say that

(r,g,b) has the _hue_ of (r',g',b')
  if r' = g' = b' and r >= g = b
  or there exist p and q such that p>=0 and r=p*r'+q and b=p*b'+q and g=p*g'+q

(r,g,b) has the _value_ of (r',g',b')
  if MAX(r,g,b) = MAX(r',g',b')

(r,g,b) has the _HSV-saturation_ of (r',g',b')
  if r' = g' = b' = 0 and r = g = b
  or MIN(r,g,b) = MAX(r,g,b)*MIN(r',g',b')/MAX(r',g',b')

(r,g,b) has the _luminosity_ of (r',g',b')
  if MID(r,g,b) = MID(r',g',b')

(r,g,b) has the _HSL-saturation_ of (r',g',b')
  if r' = g' = b' and r = g = b
  or MAX(r,g,b)-MIN(r,g,b) = MIN(MID(r,g,b),1-MID(r,g,b)) *
            (MAX(r',g',b')-MIN(r',g',b'))/MIN(MID(r',g',b'),1-MID(r',g',b'))
 
Mode 11: Hue (H of HSV)

  h(r1,g1,b1, r2,g2,b2, 11) is
   if r2=g2=b2 then (r1,g1,b1) unchanged
   otherwise: the color that has
                the hue of (r1,g2,b2)
                the value of (r1,g1,b1)
                the HSV-saturation of (r1,g1,b1)

Mode 12: Saturation (S of HSV)

  h(r1,g1,b1, r2,g2,b2, 12) is the color that has
    the hue of (r1,g1,b1)
    the value of (r1,g1,b1)
    the HSV-saturation of (r2,g2,b2)

Mode 13: Color (H and S of HSL)

  h(r1,g1,b1, r2,g2,b2, 13) is the color that has
    the hue of (r2,g2,b2)
    the luminosity of (r1,g1,b1)
    the HSL-saturation of (r2,g2,b2)

Mode 14: Value (V of HSV)

  h(r1,g1,b1, r2,g2,b2, 14) is the color that has
    the hue of (r1,g1,b1)
    the value of (r2,g2,b2)
    the HSV-saturation of (r1,g1,b1)
