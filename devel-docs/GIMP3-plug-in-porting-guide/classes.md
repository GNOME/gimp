# Useful Modules/Classes in GIMP 3.0+

Here's a guide to the modules you're likely to need.
It's a work in progress: feel free to add to it.

Eventually we'll have online documentation for these classes.
In the meantime, you can generate your own:
```
HTMLDOCDIR=/path/to/doc/dir
g-ir-doc-tool -I /path/to/share/gir-1.0/ --language=Python -o $HTMLDOCDIR Gimp-3.0.gir
```
Then browse $HTMLDOCDIR with yelp, or generate HTML from it:
```
cd $HTMLDOCDIR
yelp-build cache *.page
yelp-build html .

```

You can also get some information in GIMP's Python console with
*help(module)* or *help(object)*, and you can get a list of functions
with *dir(object)*.

## Gimp

The base module: almost everything is under Gimp.

## Gimp.Image

The image object.

Some operations that used to be PDB calls, like
```
pdb.gimp_selection_layer_alpha(layer)
```
are now in the Image object, e.g.
```
img.select_item(Gimp.ChannelOps.REPLACE, layer)
```

## Gimp.Layer

The layer object.

```
fog = Gimp.Layer.new(image, name,
                     drawable.width(), drawable.height(), type, opacity,
                     Gimp.LayerMode.NORMAL)
```

## Gimp.Selection

Selection operations that used to be in the PDB, e.g.
```
pdb.gimp_selection_none(img)
```
are now in the Gimp.Selection module, e.g.
```
Gimp.Selection.none(img)
```

## Gimp.ImageType

A home for image types like RGBA, GRAY, etc:
```
Gimp.ImageType.RGBA_IMAGE
```

## Gimp.FillType

e.g. Gimp.FillType.TRANSPARENT, Gimp.FillType.BACKGROUND

## Gimp.ChannelOps

The old channel op definitions in the gimpfu module, like
```
CHANNEL_OP_REPLACE
```
are now in their own module:

```
Gimp.ChannelOps.REPLACE
```

## Gegl.Color

In legacy plug-ins you could pass a simple list of integers, like (0, 0, 0).
In 3.0+, create a Gegl.Color object:

```
    c = Gegl.Color.new("black")
    c.set_rgba(0.94, 0.71, 0.27, 1.0)
```
