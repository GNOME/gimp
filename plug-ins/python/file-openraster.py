#!/usr/bin/env python3

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

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import os, sys, tempfile, zipfile
import xml.etree.ElementTree as ET

NESTED_STACK_END = object()

layermodes_map = {
    "svg:src-over": Gimp.LayerMode.NORMAL,
    "svg:multiply": Gimp.LayerMode.MULTIPLY_LEGACY,
    "svg:screen": Gimp.LayerMode.SCREEN_LEGACY,
    "svg:overlay": Gimp.LayerMode.OVERLAY,
    "svg:darken": Gimp.LayerMode.DARKEN_ONLY_LEGACY,
    "svg:lighten": Gimp.LayerMode.LIGHTEN_ONLY_LEGACY,
    "svg:color-dodge": Gimp.LayerMode.DODGE_LEGACY,
    "svg:color-burn": Gimp.LayerMode.BURN_LEGACY,
    "svg:hard-light": Gimp.LayerMode.HARDLIGHT_LEGACY,
    "svg:soft-light": Gimp.LayerMode.SOFTLIGHT_LEGACY,
    "svg:difference": Gimp.LayerMode.DIFFERENCE_LEGACY,
    "svg:color": Gimp.LayerMode.HSL_COLOR_LEGACY,
    "svg:luminosity": Gimp.LayerMode.HSV_VALUE_LEGACY,
    "svg:hue": Gimp.LayerMode.HSV_HUE_LEGACY,
    "svg:saturation": Gimp.LayerMode.HSV_SATURATION_LEGACY,
    "svg:plus": Gimp.LayerMode.ADDITION_LEGACY,
}

def reverse_map(mapping):
    return dict((v,k) for k, v in mapping.items())

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
    layer_mode = layermodes_map.get(m, Gimp.LayerMode.NORMAL)

    return path, name, x, y, opac, visible, layer_mode

def get_group_layer_attributes(layer):
    a = layer.attrib
    name = a.get('name', '')
    opac = float(a.get('opacity', '1.0'))
    visible = a.get('visibility', 'visible') != 'hidden'
    m = a.get('composite-op', 'svg:src-over')
    layer_mode = layermodes_map.get(m, NORMAL_MODE)

    return name, 0, 0, opac, visible, layer_mode

def thumbnail_ora(procedure, file, thumb_size, args, data):
    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')
    orafile = zipfile.ZipFile(file.peek_path())
    stack, w, h = get_image_attributes(orafile)

    # create temp file
    tmp = os.path.join(tempdir, 'tmp.png')
    with open(tmp, 'wb') as fid:
        fid.write(orafile.read('Thumbnails/thumbnail.png'))

    args = Gimp.ValueArray.new(2)
    args.insert(0, GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE))
    args.insert(1, GObject.Value(GObject.TYPE_STRING, tmp))
    img = Gimp.get_pdb().run_procedure('file-png-load', args)
    img = img.index(1)
    img = Gimp.Image.get_by_id(img)
    # TODO: scaling
    os.remove(tmp)
    os.rmdir(tempdir)

    retval = Gimp.ValueArray.new(2)
    retval.insert(0, GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS))
    retval.insert(1, GObject.Value(Gimp.Image, img))
    retval.insert(2, GObject.Value(GObject.TYPE_INT, w))
    retval.insert(3, GObject.Value(GObject.TYPE_INT, h))
    retval.insert(4, GObject.Value(Gimp.ImageType, Gimp.ImageType.RGB_IMAGE))
    retval.insert(5, GObject.Value(GObject.TYPE_INT, 1))

    return retval

def save_ora(procedure, run_mode, image, drawable, file, args, data):
    def write_file_str(zfile, fname, data):
        # work around a permission bug in the zipfile library:
        # http://bugs.python.org/issue3394
        zi = zipfile.ZipInfo(fname)
        zi.external_attr = int("100644", 8) << 16
        zfile.writestr(zi, data)

    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')

    # use .tmpsave extension, so we don't overwrite a valid file if
    # there is an exception
    orafile = zipfile.ZipFile(file.peek_path() + '.tmpsave', 'w', compression=zipfile.ZIP_STORED)

    write_file_str(orafile, 'mimetype', 'image/openraster') # must be the first file written

    # build image attributes
    xml_image = ET.Element('image')
    stack = ET.SubElement(xml_image, 'stack')
    a = xml_image.attrib
    a['w'] = str(image.width())
    a['h'] = str(image.height())

    def store_layer(image, drawable, path):
        tmp = os.path.join(tempdir, 'tmp.png')
        interlace, compression = 0, 2
        args = Gimp.ValueArray.new(11)
        args.insert(0, GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE))
        args.insert(1, GObject.Value(Gimp.Image, image))
        args.insert(2, GObject.Value(Gimp.Drawable, drawable))
        args.insert(3, GObject.Value(GObject.TYPE_STRING, tmp))
        args.insert(4, GObject.Value(GObject.TYPE_STRING, 'tmp.png'))
        args.insert(5, GObject.Value(GObject.TYPE_BOOLEAN, interlace))
        args.insert(6, GObject.Value(GObject.TYPE_INT, compression))
        # write all PNG chunks except oFFs(ets)
        args.insert(7, GObject.Value(GObject.TYPE_BOOLEAN, True))
        args.insert(8, GObject.Value(GObject.TYPE_BOOLEAN, True))
        args.insert(9, GObject.Value(GObject.TYPE_BOOLEAN, False))
        args.insert(10, GObject.Value(GObject.TYPE_BOOLEAN, True))
        Gimp.get_pdb().run_procedure('file-png-save', args)
        orafile.write(tmp, path)
        os.remove(tmp)

    def add_layer(parent, x, y, opac, gimp_layer, path, visible=True):
        store_layer(image, gimp_layer, path)
        # create layer attributes
        layer = ET.Element('layer')
        parent.append(layer)
        a = layer.attrib
        a['src'] = path
        a['name'] = gimp_layer.get_name()
        a['x'] = str(x)
        a['y'] = str(y)
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = reverse_map(layermodes_map).get(gimp_layer.get_mode(), 'svg:src-over')
        return layer

    def add_group_layer(parent, opac, gimp_layer, visible=True):
        # create layer attributes
        group_layer = ET.Element('stack')
        parent.append(group_layer)
        a = group_layer.attrib
        a['name'] = gimp_layer.name
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = reverse_map(layermodes_map).get(gimp_layer.get_mode(), 'svg:src-over')
        return group_layer


    def enumerate_layers(layers):
        for layer in layers:
            if not layer.is_group():
                yield layer
            else:
                yield layer
                for sublayer in enumerate_layers(layer.get_children()):
                    yield sublayer
                yield NESTED_STACK_END

    # save layers
    parent_groups = []
    i = 0
    for lay in enumerate_layers(image.get_layers()):
        if lay is NESTED_STACK_END:
            parent_groups.pop()
            continue
        _, x, y = lay.offsets()
        opac = lay.get_opacity () / 100.0 # needs to be between 0.0 and 1.0

        if not parent_groups:
            path_name = 'data/{:03d}.png'.format(i)
            i += 1
        else:
            path_name = 'data/{}-{:03d}.png'.format(
                parent_groups[-1][1], parent_groups[-1][2])
            parent_groups[-1][2] += 1

        parent = stack if not parent_groups else parent_groups[-1][0]

        if lay.is_group():
            group = add_group_layer(parent, opac, lay, lay.get_visible())
            group_path = ("{:03d}".format(i) if not parent_groups else
                parent_groups[-1][1] + "-{:03d}".format(parent_groups[-1][2]))
            parent_groups.append([group, group_path , 0])
        else:
            add_layer(parent, x, y, opac, lay, path_name, lay.get_visible())

    # save mergedimage
    args = Gimp.ValueArray.new(1)
    args.insert(0, GObject.Value(Gimp.Image, image))
    thumb = Gimp.get_pdb().run_procedure('gimp-image-duplicate', args)
    thumb = thumb.index(1)
    thumb = Gimp.Image.get_by_id(thumb)
    thumb_layer = thumb.merge_visible_layers (Gimp.MergeType.CLIP_TO_IMAGE)
    store_layer (thumb, thumb_layer, 'mergedimage.png')

    # save thumbnail
    w, h = image.width(), image.height()
    if max (w, h) > 256:
        # should be at most 256x256, without changing aspect ratio
        if w > h:
            w, h = 256, max(h*256/w, 1)
        else:
            w, h = max(w*256/h, 1), 256
        thumb_layer.scale(w, h, False)
    if thumb.get_precision() != Gimp.Precision.U8_GAMMA:
        thumb.convert_precision (Gimp.Precision.U8_GAMMA)
    store_layer(thumb, thumb_layer, 'Thumbnails/thumbnail.png')
    thumb.delete()

    # write stack.xml
    xml = ET.tostring(xml_image, encoding='UTF-8')
    write_file_str(orafile, 'stack.xml', xml)

    # finish up
    orafile.close()
    os.rmdir(tempdir)
    if os.path.exists(file.peek_path()):
        os.remove(file.peek_path()) # win32 needs that
    os.rename(file.peek_path() + '.tmpsave', file.peek_path())

    retval = Gimp.ValueArray.new(1)
    retval.insert(0, GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS))

    return retval

def load_ora(procedure, run_mode, file, args, data):
    tempdir = tempfile.mkdtemp('gimp-plugin-file-openraster')
    orafile = zipfile.ZipFile(file.peek_path())
    stack, w, h = get_image_attributes(orafile)

    img = Gimp.Image.new(w, h, Gimp.ImageBaseType.RGB)
    img.set_filename (file.peek_path())

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
            gimp_layer = img.layer_group_new()

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
            with open(tmp, 'wb') as fid:
                try:
                    data = orafile.read(path)
                except KeyError:
                    # support for bad zip files (saved by old versions of this plugin)
                    data = orafile.read(path.encode('utf-8'))
                    print('WARNING: bad OpenRaster ZIP file. There is an utf-8 encoded filename that does not have the utf-8 flag set:', repr(path))
                fid.write(data)

            # import layer, set attributes and add to image
            args = Gimp.ValueArray.new(3)
            arg0 = GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE)
            args.insert(0, arg0)
            arg1 = GObject.Value(Gimp.Image, img)
            args.insert(1, arg1)
            arg2 = GObject.Value(GObject.TYPE_STRING, tmp)
            args.insert(2, arg2)
            gimp_layer = Gimp.get_pdb().run_procedure('gimp-file-load-layer', args)
            gimp_layer = gimp_layer.index(1)
            gimp_layer = Gimp.Item.get_by_id(gimp_layer)
            os.remove(tmp)
        gimp_layer.set_name(name)
        gimp_layer.set_mode(layer_mode)
        gimp_layer.set_offsets(x, y)  # move to correct position
        gimp_layer.set_opacity(opac * 100)  # a float between 0 and 100
        gimp_layer.set_visible(visible)

        img.insert_layer(gimp_layer,
                         parent_groups[-1][0] if parent_groups else None,
                         parent_groups[-1][1] if parent_groups else layer_no)
        if parent_groups:
            parent_groups[-1][1] += 1
        else:
            layer_no += 1

        if gimp_layer.is_group():
            parent_groups.append([gimp_layer, 0])

    os.rmdir(tempdir)

    retval = Gimp.ValueArray.new(2)
    arg0 = GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS)
    retval.insert(0, arg0)
    arg1 = GObject.Value(Gimp.Image, img)
    retval.insert(1, arg1)

    return retval


class FileOpenRaster (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        self.set_translation_domain("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))

        return [ 'file-openraster-load-thumb',
                 'file-openraster-load',
                 'file-openraster-save' ]

    def do_create_procedure(self, name):
        if name == 'file-openraster-save':
            procedure = Gimp.SaveProcedure.new(self, name,
                                               Gimp.PDBProcType.PLUGIN,
                                               save_ora, None)
            procedure.set_image_types("*");
            procedure.set_documentation ('save an OpenRaster (.ora) file',
                                         'save an OpenRaster (.ora) file',
                                         name)
            procedure.set_menu_label('OpenRaster')
            procedure.set_extensions ("ora");
        elif name == 'file-openraster-load':
            procedure = Gimp.LoadProcedure.new (self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                load_ora, None)
            procedure.set_menu_label('OpenRaster')
            procedure.set_documentation ('load an OpenRaster (.ora) file',
                                         'load an OpenRaster (.ora) file',
                                         name)
            procedure.set_mime_types ("image/openraster");
            procedure.set_extensions ("ora");
            procedure.set_thumbnail_loader ('file-openraster-load-thumb');
        else: # 'file-openraster-load-thumb'
            procedure = Gimp.ThumbnailProcedure.new (self, name,
                                                     Gimp.PDBProcType.PLUGIN,
                                                     thumbnail_ora, None)
            procedure.set_documentation ('loads a thumbnail from an OpenRaster (.ora) file',
                                         'loads a thumbnail from an OpenRaster (.ora) file',
                                         name)
            pass
        procedure.set_attribution('Jon Nordby', #author
                                  'Jon Nordby', #copyright
                                  '2009') #year
        return procedure

Gimp.main(FileOpenRaster.__gtype__, sys.argv)
