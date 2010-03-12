#!/usr/bin/env python

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

from gimpfu import *

import tempfile, zipfile, os
import xml.etree.ElementTree as ET


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

    return path, name, x, y, opac


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
        zi.external_attr = 0100644 << 16
        zfile.writestr(zi, data)

    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')

    # use .tmpsave extension, so we don't overwrite a valid file if
    # there is an exception
    orafile = zipfile.ZipFile(filename + '.tmpsave', 'w', compression=zipfile.ZIP_STORED)

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
        png_chunks = (1, 1, 1, 1, 1) # write all PNG chunks
        pdb['file-png-save'](img, drawable, tmp, 'tmp.png',
                             interlace, compression, *png_chunks)
        orafile.write(tmp, path)
        os.remove(tmp)

    def add_layer(x, y, opac, gimp_layer, path):
        store_layer(img, gimp_layer, path)
        # create layer attributes
        layer = ET.Element('layer')
        stack.append(layer)
        a = layer.attrib
        a['src'] = path
        a['name'] = gimp_layer.name
        a['x'] = str(x)
        a['y'] = str(y)
        a['opacity'] = str(opac)
        return layer

    # save layers
    for lay in img.layers:
        x, y = lay.offsets
        opac = lay.opacity / 100.0 # needs to be between 0.0 and 1.0
        add_layer(x, y, opac, lay, 'data/%s.png' % lay.name)

    # save thumbnail
    w, h = img.width, img.height
    # should be at most 256x256, without changing aspect ratio
    if w > h:
        w, h = 256, max(h*256/w, 1)
    else:
        w, h = max(w*256/h, 1), 256
    thumb = pdb['gimp-image-duplicate'](img)
    thumb_layer = thumb.flatten()
    thumb_layer.scale(w, h)
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
    os.rename(filename + '.tmpsave', filename)

def load_ora(filename, raw_filename):
    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')
    orafile = zipfile.ZipFile(filename)
    stack, w, h = get_image_attributes(orafile)

    img = gimp.Image(w, h, RGB)
    img.filename = filename

    def get_layers(root):
        """returns a flattened list of all layers under root"""
        res = []
        for item in root:
            if item.tag == 'layer':
                res.append(item)
            elif item.tag == 'stack':
                res += get_layers(item)
        return res

    for layer_no, layer in enumerate(get_layers(stack)):
        path, name, x, y, opac = get_layer_attributes(layer)

        if not path.lower().endswith('.png'):
            continue
        if not name:
            # use the filename without extention as name
            n = os.path.basename(path)
            name = os.path.splitext(n)[0]

        # create temp file. Needed because gimp cannot load files from inside a zip file
        tmp = os.path.join(tempdir, 'tmp.png')
        f = open(tmp, 'wb')
        f.write(orafile.read(path))
        f.close()

        # import layer, set attributes and add to image
        gimp_layer = pdb['gimp-file-load-layer'](img, tmp)
        gimp_layer.name = name
        gimp_layer.translate(x, y)  # move to correct position
        gimp_layer.opacity = opac * 100  # a float between 0 and 100
        img.add_layer(gimp_layer, layer_no)

        os.remove(tmp)
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
