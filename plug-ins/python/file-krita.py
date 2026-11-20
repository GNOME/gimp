#!/usr/bin/env python3

# GIMP Plug-in for the Krita file format
# https://docs.krita.org/en/general_concepts/file_formats/file_kra.html
# https://github.com/2shady4u/godot-kra-psd-importer/blob/master/docs/KRA_FORMAT.md

#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

import gi
gi.require_version('Babl', '0.1')
from gi.repository import Babl
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import os, sys, tempfile, zipfile
import xml.etree.ElementTree as ET
from concurrent.futures import ThreadPoolExecutor

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

NESTED_STACK_END = object()

doctype = "{http://www.calligra.org/DTD/krita}"

supported_layer_types = [
    "paintlayer",
    "grouplayer",
    "shapelayer",
    "filelayer"
]

layermodes_map = {
    "normal":       Gimp.LayerMode.NORMAL,
    "multiply":     Gimp.LayerMode.MULTIPLY,
    "screen":       Gimp.LayerMode.SCREEN,
    "overlay":      Gimp.LayerMode.OVERLAY,
    "darken":       Gimp.LayerMode.DARKEN_ONLY,
    "lighten":      Gimp.LayerMode.LIGHTEN_ONLY,
    "hard_light":   Gimp.LayerMode.HARDLIGHT,
    "soft_light":   Gimp.LayerMode.SOFTLIGHT,
    "pin_light":    Gimp.LayerMode.PIN_LIGHT,
    "vivid_light":  Gimp.LayerMode.VIVID_LIGHT,
    "linear_light": Gimp.LayerMode.LINEAR_LIGHT,
    "hard_mix":     Gimp.LayerMode.HARD_MIX,
    "burn":         Gimp.LayerMode.BURN,
    "linear_burn":  Gimp.LayerMode.LINEAR_BURN,
    "difference":   Gimp.LayerMode.DIFFERENCE,
    "dissolve":     Gimp.LayerMode.DISSOLVE,
    "divide":       Gimp.LayerMode.DIVIDE,
    "color":        Gimp.LayerMode.HSL_COLOR,
    "hue":          Gimp.LayerMode.HSV_HUE,
    "saturation":   Gimp.LayerMode.HSV_SATURATION,
    "add":          Gimp.LayerMode.ADDITION,
    "subtract":     Gimp.LayerMode.SUBTRACT,
    "erase":        Gimp.LayerMode.ERASE,
    "luminize":     Gimp.LayerMode.LUMINANCE,
}

def get_babl_format (krita_layer_type, precision, is_linear):
    babl_format = None

    if "RGBA" in krita_layer_type:
        if is_linear:
            babl_format = "RGBA "
        else:
            babl_format = "R'G'B'A "
    elif "GRAYA" in krita_layer_type:
        if is_linear:
            babl_format = "YA "
        else:
            babl_format = "Y'A "
    elif "CMYK" in krita_layer_type:
        babl_format = "CMYKA "
    elif "LABA" in krita_layer_type:
        babl_format = "CIE Lab alpha "
    elif "YCBCR" in krita_layer_type:
        babl_format = "Y'CbCrA "
    elif "XYZ" in krita_layer_type:
        babl_format = "CIE XYZ alpha "
    else:
        return None

    match (precision):
        case Gimp.Precision.U8_NON_LINEAR | Gimp.Precision.U8_LINEAR:
            babl_format += "u8"
        case Gimp.Precision.U16_NON_LINEAR | Gimp.Precision.U16_LINEAR:
            babl_format += "u16"
        case Gimp.Precision.HALF_NON_LINEAR | Gimp.Precision.HALF_LINEAR:
            babl_format += "half"
        case Gimp.Precision.FLOAT_NON_LINEAR | Gimp.Precision.FLOAT_LINEAR:
            babl_format += "float"
        case _:
            babl_format = None

    return babl_format

def get_color_tag (tag):
    try:
        tag = int (tag)
    except:
        return Gimp.ColorTag.NONE

    match (tag):
        case 1:
            return Gimp.ColorTag.BLUE
        case 2:
            return Gimp.ColorTag.GREEN
        case 3:
            return Gimp.ColorTag.YELLOW
        case 4:
            return Gimp.ColorTag.ORANGE
        case 5:
            return Gimp.ColorTag.BROWN
        case 6:
            return Gimp.ColorTag.RED
        case 7:
            return Gimp.ColorTag.VIOLET
        case 8:
            return Gimp.ColorTag.GRAY
        case _:
            return Gimp.ColorTag.NONE

    return Gimp.ColorTag.NONE


def count_layers_rec (layers_xml):
    count = 0

    for layer_xml in layers_xml:
        layer_type = layer_xml.attrib.get ('nodetype', 'NONE')

        if layer_type in supported_layer_types:
            count += 1

        if layer_type == "grouplayer":
            count += count_layers_rec (layer_xml)

    return count


# Helper Methods
def get_xml (krita_file):
    root = None

    try:
        xml  = krita_file.read('maindoc.xml')
        root = ET.fromstring(xml)
    except:
        pass

    return root


def get_xml_attribute (xml, attribute, convert_type, fallback):
    value = fallback

    try:
        xml_value = xml.attrib.get (attribute, str (fallback))
        value = convert_type (xml_value)
    except:
        value = fallback

    return value


def create_image(krita_file, xml):
    # Get the <IMAGE> XML element
    image = None
    try:
        image = xml.find (doctype + 'IMAGE')
    except:
        pass
    if image == None:
        return None

    # Get the values for the width & height attributes of the <IMAGE> XML element
    w = get_xml_attribute (image, 'width', int, 0)
    h = get_xml_attribute (image, 'height', int, 0)

    if w <= 0 or w > Gimp.MAX_IMAGE_SIZE or h <= 0 or h > Gimp.MAX_IMAGE_SIZE:
        return None

    xres = get_xml_attribute (image, 'x-res', float, 72)
    yres = get_xml_attribute (image, 'y-res', float, 72)

    type      = image.attrib.get ('colorspacename', '')
    precision = Gimp.Precision.U8_NON_LINEAR

    #Get colorspace
    image_type = Gimp.ImageBaseType.RGB
    if "GRAYA" in type:
        image_type = Gimp.ImageBaseType.GRAY

    is_linear     = False
    color_profile = None

    folder_name = image.attrib.get ("name", '')
    folder_name += "/annotations/"
    try:
        icc_profile = krita_file.read (folder_name + "icc")
        if icc_profile:
            color_profile = Gimp.ColorProfile.new_from_icc_profile (icc_profile)
    except:
        pass

    if color_profile:
        is_linear = color_profile.is_linear ()
        if is_linear:
            precision = Gimp.Precision.U8_LINEAR

    #Get image precision
    if "U16" in type:
        if is_linear:
            precision = Gimp.Precision.U16_LINEAR
        else:
            precision = Gimp.Precision.U16_NON_LINEAR
    elif "F16" in type:
        if is_linear:
            precision = Gimp.Precision.HALF_LINEAR
        else:
            precision = Gimp.Precision.HALF_NON_LINEAR
    elif "F32" in type:
        if is_linear:
            precision = Gimp.Precision.FLOAT_LINEAR
        else:
            precision = Gimp.Precision.FLOAT_NON_LINEAR
    elif "16" in type:
        if is_linear:
            precision = Gimp.Precision.U16_LINEAR
        else:
            precision = Gimp.Precision.U16_NON_LINEAR

    # Create image, then set its resolution
    gimp_image = Gimp.Image.new_with_precision (w, h, image_type, precision)
    gimp_image.set_resolution (xres, yres)

    # Load color profiles and simulation profile, if any
    if color_profile:
        if image_type == Gimp.ImageBaseType.RGB and color_profile.is_rgb ():
            gimp_image.set_color_profile (color_profile)
        elif image_type == Gimp.ImageBaseType.GRAY and color_profile.is_gray ():
            gimp_image.set_color_profile (color_profile)
        elif color_profile.is_cmyk ():
            gimp_image.set_simulation_profile (color_profile)

    try:
        simulation_profile = krita_file.read (folder_name + "proofing/" + "icc")
        if simulation_profile:
            color_profile = Gimp.ColorProfile.new_from_icc_profile (simulation_profile)
            if color_profile:
                gimp_image.set_simulation_profile (color_profile)
    except:
        pass

    # GIMP assumes guide positions are always at 72dpi. If the Krita file
    # has a higher (or lower) resolution, we need to scale the guide positions
    factor_x = 72 / xres
    factor_y = 72 / yres

    # Load guides (if any)
    guides = image.find (doctype + 'guides')
    if guides != None:
        vertical_guides = guides.find (doctype + 'verticalGuides')
        for item in vertical_guides:
            point = get_xml_attribute (item, 'value', float, 0)
            gimp_image.add_vguide (point / factor_x)

        horizontal_guides = guides.find (doctype + 'horizontalGuides')
        for item in horizontal_guides:
            point = get_xml_attribute (item, 'value', float, 0)
            gimp_image.add_hguide (point / factor_y)

    # Load grid (if any)
    grid_xml = image.find (doctype + "grid")
    if grid_xml != None:
        grid_offset = grid_xml.find (doctype + "offset")
        offset_x = get_xml_attribute (grid_offset, 'x', float, 0)
        offset_y = get_xml_attribute (grid_offset, 'y', float, 0)
        gimp_image.grid_set_offset (offset_x, offset_y)

        grid_spacing = grid_xml.find (doctype + "spacing")
        spacing_x = get_xml_attribute (grid_spacing, 'x', float, 0)
        spacing_y = get_xml_attribute (grid_spacing, 'y', float, 0)
        gimp_image.grid_set_spacing (spacing_x, spacing_y)

        color_xml = grid_xml.find (doctype + "colorMain")
        color_hex = color_xml.attrib.get ("value", None)
        color = None
        if color_hex != None:
            color = Gegl.Color.new (color_hex)
        if color != None:
            gimp_image.grid_set_foreground_color (color)

    return gimp_image


def create_layers (krita_file, directory, xml, image, selected):
    # Get the <layers> XML element (by first getting the image XML, then getting the layers tag as its child)
    image_xml  = None
    layers_xml = None

    try:
        image_xml  = xml.find(doctype + 'IMAGE')
        layers_xml = image_xml.find (doctype + 'layers')
    except:
        pass

    if image_xml == None or layers_xml == None:
        return None

    folder_name = image_xml.attrib.get ("name", '')
    folder_name += "/layers/"

    total_layers = count_layers_rec (layers_xml)

    create_layer_rec (krita_file, directory, image, layers_xml, folder_name, None, selected, 0, total_layers)


def create_layer_rec (krita_file, directory, image, layers_xml, folder_name, group_layer, selected, count, total_layers):

    if group_layer == None:
        for layer_xml in layers_xml:
            layer = create_layer (krita_file, directory, image, layer_xml, folder_name, None, selected)

            count += 1
            Gimp.progress_update (count / total_layers)

            if layer_xml.attrib.get ('nodetype') == 'grouplayer':
                xml_group   = layer_xml.find (doctype + 'layers')
                group_layer = layer

                create_layer_rec (krita_file, directory, image, xml_group, folder_name, group_layer, selected, count, total_layers)
    else:
        for layer_xml in layers_xml:
            layer = create_layer (krita_file, directory, image, layer_xml, folder_name, group_layer, selected)

            count += 1
            Gimp.progress_update (count / total_layers)

            if layer_xml.attrib.get ('nodetype') == 'grouplayer':
                xml_group   = layer_xml.find (doctype + 'layers')
                group_layer = layer

                create_layer_rec (krita_file, directory, image, xml_group, folder_name, group_layer, selected, count, total_layers)


def create_layer (krita_file, directory, image, layer_xml, folder_name, group_layer, selected):
    layer         = None
    nodetype      = layer_xml.attrib.get ("nodetype", '')
    color_profile = image.get_effective_color_profile ()

    # Create a raster layer that's the size of the image. We'll fill in the pixel tiles layer
    width  = image.get_width()
    height = image.get_height()

    # Read layer properties from XML
    name    = layer_xml.attrib.get ("name", 'Layer')
    visible = get_xml_attribute (layer_xml, 'visible', int, 1)

    opacity = get_xml_attribute (layer_xml, 'opacity', float, 1.0)
    opacity = (opacity / 255) * 100.0

    offset_x = get_xml_attribute (layer_xml, 'x', int, 0)
    offset_y = get_xml_attribute (layer_xml, 'y', int, 0)

    compositeop  = layer_xml.attrib.get("compositeop", 'normal')
    blend_mode   = layermodes_map.get (compositeop, Gimp.LayerMode.NORMAL)
    color_tag    = get_xml_attribute (layer_xml, 'colorlabel', int, 0)
    color_tag    = get_color_tag (color_tag)
    passthrough  = 0
    locked       = get_xml_attribute (layer_xml, 'locked', int, 0)
    channelflags = layer_xml.attrib.get ("channellockflags", "1111")
    alpha_locked = 0
    if len (channelflags) > 3 and channelflags[3] == '0':
        alpha_locked = 1

    layer_type = Gimp.ImageType.RGBA_IMAGE
    if image.get_base_type () == Gimp.ImageBaseType.GRAY:
        layer_type = Gimp.ImageType.GRAYA_IMAGE

    file_name = layer_xml.attrib.get("filename", "")

    # Raster layer
    if nodetype == 'paintlayer':
        # Krita can have negative offsets that affect the drawable regions
        width  += abs (offset_x)
        height += abs (offset_y)

        if width <= Gimp.MAX_IMAGE_SIZE and height <= Gimp.MAX_IMAGE_SIZE:
            layer = Gimp.Layer.new (image, name, width, height, layer_type, opacity, blend_mode)

            krita_layer_type = layer_xml.attrib.get ("colorspacename", "RGBA")
            babl_format      = get_babl_format (krita_layer_type, image.get_precision (), color_profile.is_linear ())

            import_tile_data (krita_file, image, layer, babl_format, folder_name, file_name)
    # Text/Vector Layer
    elif nodetype == 'shapelayer':
        file_name += ".shapelayer/content.svg"

        tempdir = tempfile.mkdtemp ('gimp-plugin-file-krita')
        tmp = os.path.join (tempdir, 'tmp.svg')
        with open (tmp, 'wb') as fid:
            fid.write (krita_file.read (folder_name + file_name))

        #Use GIMP's built-in layer loading plug-in to load the layer
        svg_file   = Gio.file_new_for_path(tmp)
        pdb_proc   = Gimp.get_pdb().lookup_procedure('gimp-file-load-layer')
        pdb_config = pdb_proc.create_config()
        pdb_config.set_property ('run-mode', Gimp.RunMode.NONINTERACTIVE)
        pdb_config.set_property ('image', image)
        pdb_config.set_property ('file', svg_file)
        result = pdb_proc.run (pdb_config)
        #Clean up after ourselves - delete the temporary file and directory
        os.remove (tmp)
        os.rmdir (tempdir)

        if result.index (0) == Gimp.PDBStatusType.SUCCESS:
            layer = result.index(1)
    # Link Layer
    elif nodetype == 'filelayer':
        source = layer_xml.attrib.get ('source', '')

        # Krita stores relative paths
        if source:
            parent       = directory
            source_parts = source.split('/')

            while source_parts and source_parts[0] == '..':
                parent = parent.get_parent ()
                source_parts.pop (0)

            path = parent.get_path () + '/'
            path += '/'.join (source_parts)

            link_file = Gio.File.new_for_path (path)

            if link_file.query_exists (None):
                layer = Gimp.LinkLayer.new (image, link_file)
    # Group Layer
    elif nodetype == 'grouplayer':
        layer = Gimp.GroupLayer.new (image, name)

        passthrough = layer_xml.attrib.get('passthrough', '0')
        if passthrough == '1':
            blend_mode = Gimp.LayerMode.PASS_THROUGH

    # Set individual layer properties from the XML, add to image
    if layer != None:
        layer.set_name (name)
        layer.set_opacity (opacity)
        layer.set_visible (visible)
        layer.set_offsets (offset_x, offset_y)
        layer.set_mode (blend_mode)
        layer.set_lock_content (locked)
        layer.set_lock_alpha (alpha_locked)
        layer.set_color_tag (color_tag)

        # Add any selected layer to the list
        is_selected = layer_xml.attrib.get ("selected", 'false')
        if is_selected != 'false':
            selected.append (layer)

        position = len (image.get_layers())
        image.insert_layer (layer, group_layer, position)

    return layer


def import_tile_data (krita_file, image, layer, babl_format, folder_name, file_name):
    buffer = layer.get_buffer()

    background_color = Gegl.Color.new ("none")
    rect             = Gegl.Rectangle()
    rect.x           = 0
    rect.y           = 0
    rect.width       = image.get_width()
    rect.height      = image.get_height()

    # Fill with default pixel
    try:
        default_pixel = krita_file.read (folder_name + file_name + ".defaultpixel")
        byte_data     = GLib.Bytes.new (default_pixel)
        background_color.set_bytes (Babl.format (babl_format), byte_data)
    except:
        pass
    buffer.set_color (rect, background_color)

    # Next, read the tiles
    tile_data = None
    try:
        tile_data = krita_file.read (folder_name + file_name)
    except:
        return None

    # Newline in hex is 0x0A, which separates sections
    NEWLINE = 10

    # Read header
    start   = 0
    end     = 10
    version = tile_data[start:end]
    start   = end

    while end < len (tile_data) and tile_data[end] != NEWLINE:
        end += 1
    tile_width = tile_data[start:end]
    end += 1
    start = end

    while end < len (tile_data) and tile_data[end] != NEWLINE:
        end += 1
    tile_height = tile_data[start:end]
    end += 1
    start = end

    while end < len (tile_data) and tile_data[end] != NEWLINE:
        end += 1
    pixel_size = tile_data[start:end]
    end += 1
    start = end

    while end < len (tile_data) and tile_data[end] != NEWLINE:
        end += 1
    data = tile_data[start:end]
    end += 1
    start = end

    #Split header info to get relevant info
    tile_width  = tile_width.decode ('utf-8').split (" ")
    tile_height = tile_height.decode ('utf-8').split (" ")
    pixel_size  = pixel_size.decode ('utf-8').split( " ")
    data        = data.decode ('utf-8').split (" ")

    try:
        tile_width  = int (tile_width[1])
        tile_height = int (tile_height[1])
        pixel_size  = int (pixel_size[1])
        num_tiles   = int (data[1])
    except:
        return None

    if tile_width < 1 or tile_width > Gimp.MAX_IMAGE_SIZE:
        return None
    if tile_height < 1 or tile_height > Gimp.MAX_IMAGE_SIZE:
        return None
    if pixel_size < 1:
        return None
    if num_tiles < 1:
        return None

    rect.width  = tile_width
    rect.height = tile_height

    #Get raw tile info
    tile_coords = []
    compressed  = []
    tile_dims   = (rect.width, rect.height)

    for x in range (num_tiles):
        while end < len (tile_data) and tile_data[end] != NEWLINE:
            end += 1
        item  = tile_data[start:end]
        start = end

        item = item.decode ('utf-8').split (",")
        try:
            rect.x  = int (item[0])
            rect.y  = int (item[1])
            to_read = int (item[3])
        except:
            continue

        if rect.x > Gimp.MAX_IMAGE_SIZE:
            return None
        if rect.y > Gimp.MAX_IMAGE_SIZE:
            return None

        compression = tile_data[end + 1]
        end += 1
        start = end
        end += to_read

        start += 1
        lzf_data = tile_data[start:end]
        start = end

        # Add to list
        tile_coords.append ((rect.x, rect.y))
        compressed.append (lzf_data)

    # Use threads to draw multiple tiles at once
    with ThreadPoolExecutor () as executor:
        for x in range (len (compressed)):
            executor.submit (tile_decompress, layer, buffer, tile_coords[x], tile_dims, compressed[x], pixel_size, babl_format)

def tile_decompress (layer, buffer, tile_coords, tile_dims, lzf_data, pixel_size, babl_format):
    tile_size = tile_dims[0] * tile_dims[1]

    pixels = lzf_decompress (lzf_data, (tile_size * pixel_size))

    final_pixels = bytearray (tile_size * pixel_size)

    rect        = Gegl.Rectangle()
    rect.x      = tile_coords[0]
    rect.y      = tile_coords[1]
    rect.width  = tile_dims[0]
    rect.height = tile_dims[1]

    if pixels == None:
        return None

    # Krita stores 8/16 bit integer RGBA as BGRA
    if "R'G'B'A u8" in babl_format or "RGBA u8" in babl_format:
        final_pixels[0::4] = pixels[2 * tile_size:3 * tile_size]  # Red
        final_pixels[1::4] = pixels[1 * tile_size:2 * tile_size]  # Green
        final_pixels[2::4] = pixels[0:tile_size]                  # Blue
        final_pixels[3::4] = pixels[3 * tile_size:4 * tile_size]  # Alpha
    elif "R'G'B'A u16" in babl_format or "RGBA u16" in babl_format:
        final_pixels[0::8] = pixels[4 * tile_size:5 * tile_size]  # Red
        final_pixels[1::8] = pixels[5 * tile_size:6 * tile_size]
        final_pixels[2::8] = pixels[2 * tile_size:3 * tile_size]  # Green
        final_pixels[3::8] = pixels[3 * tile_size:4 * tile_size]
        final_pixels[4::8] = pixels[0:tile_size]                  # Blue
        final_pixels[5::8] = pixels[tile_size:2 * tile_size]
        final_pixels[6::8] = pixels[6 * tile_size:7 * tile_size]  # Alpha
        final_pixels[7::8] = pixels[7 * tile_size:8 * tile_size]
    else:
        for k in range (pixel_size):
            final_pixels[k::pixel_size] = pixels[k * tile_size:(k + 1) * tile_size]

    try:
        if rect.x >= 0 and rect.y >= 0:
            buffer.set (rect, babl_format, final_pixels)
    except Exception as e:
        print (e)

# Referenced from lzfUncompress (), at
# https://github.com/Grum999/BuliCommander/blob/tool_repair_file/bulicommander/bulicommander/bc/bcrepairfiles.py
def lzf_decompress (dataIn, lenDataOut):
    dataOut   = bytearray(lenDataOut)
    lenDataIn = len (dataIn)
    inputPos  = 0
    outputPos = 0

    while True:
        ctrl      = dataIn[inputPos]
        inputPos += 1

        if ctrl < 32:
            ctrl += 1

            if outputPos + ctrl > lenDataOut:
                return None

            inputLen  = inputPos + ctrl
            outputLen = outputPos + ctrl

            dataOut[outputPos:outputLen] = dataIn[inputPos:inputLen]

            outputPos += ctrl
            inputPos  += ctrl
        else:
            dataLen = ctrl >> 5

            reference = outputPos - ((ctrl & 0x1f) << 8) - 1

            if dataLen == 7:
                dataLen  += dataIn[inputPos]
                inputPos += 1

            reference -= dataIn[inputPos]
            inputPos  += 1

            if outputPos + dataLen + 2 > lenDataOut:
                return None

            if reference < 0:
                return None

            for loop in range (dataLen + 2):
                dataOut[outputPos] = dataOut[reference]
                outputPos         += 1
                reference         += 1

        if inputPos >= lenDataIn:
            break

    return dataOut


# ====================
# Image Format methods
# ====================
def load_kra_thumbnail (procedure, file, thumb_size, args, data):
    tempdir    = tempfile.mkdtemp('gimp-plugin-file-krita')
    krita_file = None

    try:
        krita_file = zipfile.ZipFile (file.peek_path())
    except:
        pass

    if krita_file == None:
        return Gimp.ValueArray.new_from_values([
            GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.EXECUTION_ERROR),
        ]), flags

    # Create temporary file for thumbnail PNG, and write the data to it
    tmp = os.path.join(tempdir, 'tmp.png')
    with open(tmp, 'wb') as fid:
        fid.write(krita_file.read('preview.png'))

    #Use GIMP's built-in PNG plug-in to load the image
    thumb_file = Gio.file_new_for_path(tmp)
    pdb_proc   = Gimp.get_pdb().lookup_procedure('file-png-load')
    pdb_config = pdb_proc.create_config()
    pdb_config.set_property('run-mode', Gimp.RunMode.NONINTERACTIVE)
    pdb_config.set_property('file', thumb_file)
    result = pdb_proc.run(pdb_config)

    os.remove(tmp)
    os.rmdir(tempdir)

    #If we were able to load the thumbnail image, then return it to GIMP to display
    if result.index(0) == Gimp.PDBStatusType.SUCCESS:
        img = result.index(1)

        return Gimp.ValueArray.new_from_values([
            GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS),
            GObject.Value(Gimp.Image, img),
            GObject.Value(GObject.TYPE_INT, img.get_width()),
            GObject.Value(GObject.TYPE_INT, img.get_height()),
            GObject.Value(Gimp.ImageType, Gimp.ImageType.RGB_IMAGE),
            GObject.Value(GObject.TYPE_INT, 1)
        ])
    else:
        return procedure.new_return_values(result.index(0), GLib.Error(result.index(1)))


def load_kra_merged (procedure, run_mode, file, metadata, flags, config, data):
    tempdir    = tempfile.mkdtemp ('gimp-plugin-file-krita')
    krita_file = None

    try:
        krita_file = zipfile.ZipFile (file.peek_path())
    except:
        pass

    if krita_file == None:
        return Gimp.ValueArray.new_from_values([
            GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.EXECUTION_ERROR),
        ]), flags

    # create temp file
    tmp = os.path.join (tempdir, 'tmp.png')
    with open(tmp, 'wb') as fid:
        fid.write(krita_file.read('mergedimage.png'))

    thumb_file = Gio.file_new_for_path (tmp)
    pdb_proc   = Gimp.get_pdb().lookup_procedure ('file-png-load')
    pdb_config = pdb_proc.create_config ()
    pdb_config.set_property ('run-mode', Gimp.RunMode.NONINTERACTIVE)
    pdb_config.set_property ('file', thumb_file)
    result = pdb_proc.run (pdb_config)
    os.remove (tmp)
    os.rmdir (tempdir)

    if result.index (0) == Gimp.PDBStatusType.SUCCESS:
        img = result.index (1)

        return Gimp.ValueArray.new_from_values ([
            GObject.Value (Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS),
            GObject.Value (Gimp.Image,         img),
            GObject.Value (GObject.TYPE_INT,   img.get_width()),
            GObject.Value (GObject.TYPE_INT,   img.get_height()),
            GObject.Value (Gimp.ImageType,     img.get_base_type()),
            GObject.Value (GObject.TYPE_INT,   1)
        ])
    else:
        return procedure.new_return_values (result.index (0), GLib.Error (result.index (1)))


def load_kra (procedure, run_mode, file, metadata, flags, config, data):
    status     = Gimp.PDBStatusType.SUCCESS
    directory  = file.get_parent()
    image      = None
    selected   = []
    krita_file = None

    try:
        krita_file = zipfile.ZipFile (file.peek_path())
    except:
        pass

    if krita_file == None:
        return Gimp.ValueArray.new_from_values([
            GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.EXECUTION_ERROR),
        ]), flags

    xml = get_xml (krita_file)

    if xml != None:
        Gimp.progress_init ("Loading Krita image")

        image = create_image (krita_file, xml)
        if image != None:
            create_layers (krita_file, directory, xml, image, selected)
        else:
            status = Gimp.PDBStatusType.EXECUTION_ERROR

        if len (selected) > 0:
            image.set_selected_layers (selected)

        Gimp.progress_end()
    else:
        return Gimp.ValueArray.new_from_values([
            GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.EXECUTION_ERROR),
        ]), flags

    return Gimp.ValueArray.new_from_values([
        GObject.Value(Gimp.PDBStatusType, status),
        GObject.Value(Gimp.Image, image),
    ]), flags


class FileKrita (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n (self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures (self):
        return [ 'file-krita-load-thumb',
                 'file-krita-load',
                 'file-krita-load-merged' ]

    def do_create_procedure (self, name):
        if name == 'file-krita-load':
            procedure = Gimp.LoadProcedure.new (self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                load_kra, None)
            procedure.set_menu_label(_('Krita'))
            procedure.set_documentation (_('Loads a Krita (.kra) file'),
                                         None, name)
            procedure.set_mime_types ("application/x-kra");
            procedure.set_extensions ("kra");
            procedure.set_thumbnail_loader ('file-krita-load-thumb');

        elif name == 'file-krita-load-merged':
            procedure = Gimp.LoadProcedure.new (self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                load_kra_merged, None)
            procedure.set_menu_label(_('Krita (merged)'))
            procedure.set_documentation (_('Loads the merged image from a Krita (.kra) file'),
                                         None, name)
            procedure.set_mime_types ("application/x-kra");
            procedure.set_extensions ("kra");
            procedure.set_priority (100)
            procedure.set_thumbnail_loader ('file-krita-load-thumb');

        else: # 'file-krita-load-thumb'
            procedure = Gimp.ThumbnailProcedure.new (self, name,
                                                     Gimp.PDBProcType.PLUGIN,
                                                     load_kra_thumbnail, None)
            procedure.set_documentation (_('Loads a thumbnail from a Krita (.kra) file'),
                                         None, name)
        procedure.set_attribution('Isa Ragland, Krish Patel, Yazan Abdallah',
                                  'Isa Ragland, Krish Patel, Yazan Abdallah',
                                  '2026')
        return procedure

Gimp.main(FileKrita.__gtype__, sys.argv)
