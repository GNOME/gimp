#!/usr/bin/env python

import os

import gimp
from gimpfu import *

def pyslice(image, drawable, save_path, html_filename,
            image_basename, image_extension, separate,
            image_path, capitalize, cellspacing):

    vert, horz = get_guides(image)

    if len(vert) == 0 and len(horz) == 0:
        return

    gimp.progress_init("Py-Slice")
    progress_increment = 1 / ((len(horz) + 1) * (len(vert) + 1))
    progress = 0.0

    def check_path(path):
        path = os.path.abspath(path)

        if not os.path.exists(path):
            os.mkdir(path)

        return path
    
    save_path = check_path(save_path)

    if not os.path.isdir(save_path):
        save_path = os.path.dirname(save_path)

    if separate:
        image_relative_path = image_path
        image_path = check_path(os.path.join(save_path, image_path))
    else:
        image_relative_path = ''
        image_path = save_path

    tw = TableWriter(os.path.join(save_path, html_filename),
                     cellspacing=cellspacing, capitalize=capitalize)

    top = 0

    for i in range(0, len(horz) + 1):
        if i == len(horz):
            bottom = image.height
        else:
            bottom = image.get_guide_position(horz[i])

        tw.row_start()

        left = 0

        for j in range(0, len(vert) + 1):
            if j == len(vert):
                right = image.width
            else:
                right = image.get_guide_position(vert[j])

            src = slice(image, image_path, image_basename, image_extension,
                        left, right, top, bottom, i, j)

            if image_relative_path:
                if image_relative_path.endswith('/'):
                    src_path = image_relative_path + src
                else:
                    src_path = image_relative_path + '/' + src
            else:
                src_path = src

            tw.cell(src_path, right - left, bottom - top)

            left = right + cellspacing

            progress += progress_increment
            gimp.progress_update(progress)

        tw.row_end()

        top = bottom + cellspacing

    tw.close()

def slice(image, image_path, image_basename, image_extension,
          left, right, top, bottom, i, j):
    src = "%s-%d-%d.%s" % (image_basename, i, j, image_extension)
    filename = os.path.join(image_path, src)

    temp_image = image.duplicate()
    temp_image.disable_undo()

    temp_image.crop(right - left, bottom - top, left, top)

    pdb.gimp_file_save(temp_image, temp_image.active_layer, filename, filename)

    gimp.delete(temp_image)

    return src

class GuideIter:
    def __init__(self, image):
        self.image = image
        self.guide = 0

    def __iter__(self):
        return iter(self.next_guide, 0)

    def next_guide(self):
        self.guide = self.image.find_next_guide(self.guide)
        return self.guide

def get_guides(image):
    vguides = []
    hguides = []

    for guide in GuideIter(image):
        orientation = image.get_guide_orientation(guide)

        guide_position = image.get_guide_position(guide)

        if guide_position > 0:
            if orientation == ORIENTATION_VERTICAL:
                if guide_position < image.width:
                    vguides.append((guide_position, guide))
            elif orientation == ORIENTATION_HORIZONTAL:
                if guide_position < image.height:
                    hguides.append((guide_position, guide))

    def position_sort(x, y):
        return cmp(x[0], y[0])

    vguides.sort(position_sort)
    hguides.sort(position_sort)

    vguides = [g[1] for g in vguides]
    hguides = [g[1] for g in hguides]

    return vguides, hguides

class TableWriter:
    def __init__(self, filename, cellpadding=0, cellspacing=0, border=0,
                 capitalize=FALSE):
        self.table_attrs = {}

        self.table_attrs['cellpadding'] = cellpadding
        self.table_attrs['cellspacing'] = cellspacing
        self.table_attrs['border'] = border

        self.capitalize = capitalize

        self.html = file(filename, 'w')

        self.open()

    def write(self, s, vals=None):
        if self.capitalize:
            s = s.upper()
            s = s.replace('%S', '%s')
        else:
            s = s.lower()

        if vals:
            s = s % vals

        self.html.write(s + '\n')

    def open(self):
        out = '<table'

        for attr, value in self.table_attrs.iteritems():
            out += ' %s=%s' % (attr, value)

        out += '>'

        self.write(out)

    def close(self):
        self.write('</table>')
        self.html.close()

    def row_start(self):
        self.write('\t<tr>')

    def row_end(self):
        self.write('\t</tr>')

    def cell(self, src, width, height):
        out = ('\t\t<td><img alt=" " src="%%s" width="%d" height="%d"></td>' %
               (width, height))
        self.write(out, src)

register(
    "py_slice",
    "Guillotine implemented ala python, with html output (based on perlotine by Seth Burgess)",
    "Add guides to an image. Then run this. It will cut along the guides, and give you the html to reassemble the resulting images.",
    "Manish Singh",
    "Manish Singh",
    "2003",
    "<Image>/Filters/Web/Py-Slice...",
    "*",
    [
        (PF_STRING, "save_path", "The path to export the HTML to", os.getcwd()),
        (PF_STRING, "html_filename", "Filename to export", "py-slice.html"),
        (PF_STRING, "image_basename", "What to call the images", "pyslice"),
        (PF_RADIO, "image_extension", "The format of the images: {gif, jpg, png}", "gif", (("gif", "gif"), ("jpg", "jpg"), ("png", "png"))),
        (PF_TOGGLE, "separate_image_dir", "Use a separate directory for images?", FALSE),
        (PF_STRING, "relative_image_path", "The path to export the images to, relative to the Save Path", "images/"),
        (PF_TOGGLE, "capitalize_tags", "Capitalize HTML tags?", FALSE),
        (PF_SPINNER, "cellspacing", "Add space between the table elements", 0, (0,15,1))
    ],
    [],
    pyslice)

main()
