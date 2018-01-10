#!/usr/bin/env python2

# GIMP Plug-in for the OpenRaster file format
# http://create.freedesktop.org/wiki/OpenRaster

# Copyright (C) 2009 by Jon Nordby <jononor@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Based on MyPaint source code by Martin Renold
# http://gitorious.org/mypaint/mypaint/blobs/edd84bcc1e091d0d56aa6d26637aa8a925987b6a/lib/document.py

import os, sys, tempfile, zipfile
import xml.etree.ElementTree as ET


from gimpfu import *

NESTED_STACK_END = object()


layermodes_map = {
    "svg:src-over": LAYER_MODE_NORMAL,
    "svg:multiply": LAYER_MODE_MULTIPLY_LEGACY,
    "svg:screen": LAYER_MODE_SCREEN_LEGACY,
    "svg:overlay": LAYER_MODE_OVERLAY,
    "svg:darken": LAYER_MODE_DARKEN_ONLY_LEGACY,
    "svg:lighten": LAYER_MODE_LIGHTEN_ONLY_LEGACY,
    "svg:color-dodge": LAYER_MODE_DODGE_LEGACY,
    "svg:color-burn": LAYER_MODE_BURN_LEGACY,
    "svg:hard-light": LAYER_MODE_HARDLIGHT_LEGACY,
    "svg:soft-light": LAYER_MODE_SOFTLIGHT_LEGACY,
    "svg:difference": LAYER_MODE_DIFFERENCE_LEGACY,
    "svg:color": LAYER_MODE_HSL_COLOR_LEGACY,
    "svg:luminosity": LAYER_MODE_HSV_VALUE_LEGACY,
    "svg:hue": LAYER_MODE_HSV_HUE_LEGACY,
    "svg:saturation": LAYER_MODE_HSV_SATURATION_LEGACY,
    "svg:plus": LAYER_MODE_ADDITION_LEGACY,
}

def reverse_map(mapping):
    return dict((v,k) for k, v in mapping.iteritems())

def get_image_attributes(orafile):
    xml = orafile.read('stack.xml')
    image = ET.fromstring(xml)
    stack = image.find('stack')
    w = int(image.attrib.get('w', ''))
    h = int(image.attrib.get('h', ''))

    return stack, w, h

def get_layer_attributes(layer):
    a = layer.attrib
    path = a.get('src', '')
    name = a.get('name', '')
    x = int(a.get('x', '0'))
    y = int(a.get('y', '0'))
    opac = float(a.get('opacity', '1.0'))
    visible = a.get('visibility', 'visible') != 'hidden'
    m = a.get('composite-op', 'svg:src-over')
    layer_mode = layermodes_map.get(m, LAYER_MODE_NORMAL)

    return path, name, x, y, opac, visible, layer_mode

def get_group_layer_attributes(layer):
    a = layer.attrib
    name = a.get('name', '')
    opac = float(a.get('opacity', '1.0'))
    visible = a.get('visibility', 'visible') != 'hidden'
    m = a.get('composite-op', 'svg:src-over')
    layer_mode = layermodes_map.get(m, NORMAL_MODE)

    return name, 0, 0, opac, visible, layer_mode

def thumbnail_ora(filename, thumb_size):
    # FIXME: Untested. Does not seem to be used at all? should be run
    # when registered and there is no thumbnail in cache

    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')
    orafile = zipfile.ZipFile(filename)
    stack, w, h = get_image_attributes(orafile)

    # create temp file
    tmp = os.path.join(tempdir, 'tmp.png')
    f = open(tmp, 'wb')
    f.write(orafile.read('Thumbnails/thumbnail.png'))
    f.close()

    img = pdb['file-png-load'](tmp)
    # TODO: scaling
    os.remove(tmp)
    os.rmdir(tempdir)

    return (img, w, h)

def save_ora(img, drawable, filename, raw_filename):
    def write_file_str(zfile, fname, data):
        # work around a permission bug in the zipfile library:
        # http://bugs.python.org/issue3394
        zi = zipfile.ZipInfo(fname)
        zi.external_attr = int("100644", 8) << 16
        zfile.writestr(zi, data)

    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')

    if isinstance(filename, str):
        try:
            filename = filename.decode("utf-8")
        except UnicodeDecodeError:
            # 1 - 1 correspondence between raw_bytes and UCS-2 used by Python
            # Unicode characters
            filename = filename.decode("latin1")
    encoding = sys.getfilesystemencoding() or "utf-8"
    filename = filename.encode(encoding)
    tmp_sufix = ".tmpsave".encode(encoding)
    # use .tmpsave extension, so we don't overwrite a valid file if
    # there is an exception
    orafile = zipfile.ZipFile(filename + tmp_sufix, 'w', compression=zipfile.ZIP_STORED)

    write_file_str(orafile, 'mimetype', 'image/openraster') # must be the first file written

    # build image attributes
    image = ET.Element('image')
    stack = ET.SubElement(image, 'stack')
    a = image.attrib
    a['w'] = str(img.width)
    a['h'] = str(img.height)

    def store_layer(img, drawable, path):
        tmp = os.path.join(tempdir, 'tmp.png')
        interlace, compression = 0, 2
        png_chunks = (1, 1, 0, 1, 1) # write all PNG chunks except oFFs(ets)
        pdb['file-png-save'](img, drawable, tmp, 'tmp.png',
                             interlace, compression, *png_chunks)
        orafile.write(tmp, path)
        os.remove(tmp)

    def add_layer(parent, x, y, opac, gimp_layer, path, visible=True):
        store_layer(img, gimp_layer, path)
        # create layer attributes
        layer = ET.Element('layer')
        parent.append(layer)
        a = layer.attrib
        a['src'] = path
        a['name'] = gimp_layer.name
        a['x'] = str(x)
        a['y'] = str(y)
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = reverse_map(layermodes_map).get(gimp_layer.mode, 'svg:src-over')
        return layer

    def add_group_layer(parent, opac, gimp_layer, visible=True):
        # create layer attributes
        group_layer = ET.Element('stack')
        parent.append(group_layer)
        a = group_layer.attrib
        a['name'] = gimp_layer.name
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = reverse_map(layermodes_map).get(gimp_layer.mode, 'svg:src-over')
        return group_layer


    def enumerate_layers(group):
        for layer in group.layers:
            if not isinstance(layer, gimp.GroupLayer):
                yield layer
            else:
                yield layer
                for sublayer in enumerate_layers(layer):
                    yield sublayer
                yield NESTED_STACK_END

    # save layers
    parent_groups = []
    i = 0
    for lay in enumerate_layers(img):
        if lay is NESTED_STACK_END:
            parent_groups.pop()
            continue
        x, y = lay.offsets
        opac = lay.opacity / 100.0 # needs to be between 0.0 and 1.0

        if not parent_groups:
            path_name = 'data/{:03d}.png'.format(i)
            i += 1
        else:
            path_name = 'data/{}-{:03d}.png'.format(
                parent_groups[-1][1], parent_groups[-1][2])
            parent_groups[-1][2] += 1

        parent = stack if not parent_groups else parent_groups[-1][0]

        if isinstance(lay, gimp.GroupLayer):
            group = add_group_layer(parent, opac, lay, lay.visible)
            group_path = ("{:03d}".format(i) if not parent_groups else
                parent_groups[-1][1] + "-{:03d}".format(parent_groups[-1][2]))
            parent_groups.append([group, group_path , 0])
        else:
            add_layer(parent, x, y, opac, lay, path_name, lay.visible)

    # save mergedimage
    thumb = pdb['gimp-image-duplicate'](img)
    thumb_layer = thumb.merge_visible_layers (CLIP_TO_IMAGE)
    store_layer (thumb, thumb_layer, 'mergedimage.png')

    # save thumbnail
    w, h = img.width, img.height
    if max (w, h) > 256:
        # should be at most 256x256, without changing aspect ratio
        if w > h:
            w, h = 256, max(h*256/w, 1)
        else:
            w, h = max(w*256/h, 1), 256
        thumb_layer.scale(w, h)
    if thumb.precision != PRECISION_U8_GAMMA:
        pdb.gimp_image_convert_precision (thumb, PRECISION_U8_GAMMA)
    store_layer(thumb, thumb_layer, 'Thumbnails/thumbnail.png')
    gimp.delete(thumb)

    # write stack.xml
    xml = ET.tostring(image, encoding='UTF-8')
    write_file_str(orafile, 'stack.xml', xml)

    # finish up
    orafile.close()
    os.rmdir(tempdir)
    if os.path.exists(filename):
        os.remove(filename) # win32 needs that
    os.rename(filename + tmp_sufix, filename)


def load_ora(filename, raw_filename):
    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')
    original_name = filename
    try:
        if not isinstance(filename, str):
            filename = filename.decode("utf-8")
        orafile = zipfile.ZipFile(filename.encode(sys.getfilesystemencoding() or "utf-8"))
    except (UnicodeDecodeError, IOError):
        # Someone may try to open an actually garbled name, and pass a raw
        # non-utf 8 filename:
        orafile = zipfile.ZipFile(original_name)
    stack, w, h = get_image_attributes(orafile)

    img = gimp.Image(w, h, RGB)
    img.filename = filename

    def get_layers(root):
        """iterates over layers and nested stacks"""
        for item in root:
            if item.tag == 'layer':
                yield item
            elif item.tag == 'stack':
                yield item
                for subitem in get_layers(item):
                    yield subitem
                yield NESTED_STACK_END

    parent_groups = []

    layer_no = 0
    for item in get_layers(stack):

        if item is NESTED_STACK_END:
            parent_groups.pop()
            continue

        if item.tag == 'stack':
            name, x, y, opac, visible, layer_mode = get_group_layer_attributes(item)
            gimp_layer = gimp.GroupLayer(img)

        else:
            path, name, x, y, opac, visible, layer_mode = get_layer_attributes(item)

            if not path.lower().endswith('.png'):
                continue
            if not name:
                # use the filename without extension as name
                n = os.path.basename(path)
                name = os.path.splitext(n)[0]

            # create temp file. Needed because gimp cannot load files from inside a zip file
            tmp = os.path.join(tempdir, 'tmp.png')
            f = open(tmp, 'wb')
            try:
                data = orafile.read(path)
            except KeyError:
                # support for bad zip files (saved by old versions of this plugin)
                data = orafile.read(path.encode('utf-8'))
                print 'WARNING: bad OpenRaster ZIP file. There is an utf-8 encoded filename that does not have the utf-8 flag set:', repr(path)
            f.write(data)
            f.close()

            # import layer, set attributes and add to image
            gimp_layer = pdb['gimp-file-load-layer'](img, tmp)
            os.remove(tmp)
        gimp_layer.name = name
        gimp_layer.mode = layer_mode
        gimp_layer.set_offsets(x, y)  # move to correct position
        gimp_layer.opacity = opac * 100  # a float between 0 and 100
        gimp_layer.visible = visible

        pdb.gimp_image_insert_layer(img, gimp_layer,
                                    parent_groups[-1][0] if parent_groups else None,
                                    parent_groups[-1][1] if parent_groups else layer_no)
        if parent_groups:
            parent_groups[-1][1] += 1
        else:
            layer_no += 1

        if isinstance(gimp_layer, gimp.GroupLayer):
            parent_groups.append([gimp_layer, 0])

    os.rmdir(tempdir)

    return img


def register_load_handlers():
    gimp.register_load_handler('file-openraster-load', 'ora', '')
    pdb['gimp-register-file-handler-mime']('file-openraster-load', 'image/openraster')
    pdb['gimp-register-thumbnail-loader']('file-openraster-load', 'file-openraster-load-thumb')

def register_save_handlers():
    gimp.register_save_handler('file-openraster-save', 'ora', '')

register(
    'file-openraster-load-thumb', #name
    'loads a thumbnail from an OpenRaster (.ora) file', #description
    'loads a thumbnail from an OpenRaster (.ora) file',
    'Jon Nordby', #author
    'Jon Nordby', #copyright
    '2009', #year
    None,
    None, #image type
    [   #input args. Format (type, name, description, default [, extra])
        (PF_STRING, 'filename', 'The name of the file to load', None),
        (PF_INT, 'thumb-size', 'Preferred thumbnail size', None),
    ],
    [   #results. Format (type, name, description)
        (PF_IMAGE, 'image', 'Thumbnail image'),
        (PF_INT, 'image-width', 'Width of full-sized image'),
        (PF_INT, 'image-height', 'Height of full-sized image')
    ],
    thumbnail_ora, #callback
)

register(
    'file-openraster-save', #name
    'save an OpenRaster (.ora) file', #description
    'save an OpenRaster (.ora) file',
    'Jon Nordby', #author
    'Jon Nordby', #copyright
    '2009', #year
    'OpenRaster',
    '*',
    [   #input args. Format (type, name, description, default [, extra])
        (PF_IMAGE, "image", "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
        (PF_STRING, "filename", "The name of the file", None),
        (PF_STRING, "raw-filename", "The name of the file", None),
    ],
    [], #results. Format (type, name, description)
    save_ora, #callback
    on_query = register_save_handlers,
    menu = '<Save>'
)

register(
    'file-openraster-load', #name
    'load an OpenRaster (.ora) file', #description
    'load an OpenRaster (.ora) file',
    'Jon Nordby', #author
    'Jon Nordby', #copyright
    '2009', #year
    'OpenRaster',
    None, #image type
    [   #input args. Format (type, name, description, default [, extra])
        (PF_STRING, 'filename', 'The name of the file to load', None),
        (PF_STRING, 'raw-filename', 'The name entered', None),
    ],
    [(PF_IMAGE, 'image', 'Output image')], #results. Format (type, name, description)
    load_ora, #callback
    on_query = register_load_handlers,
    menu = "<Load>",
)


main()
