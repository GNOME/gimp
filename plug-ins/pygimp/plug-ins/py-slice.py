#!/usr/bin/env python
# -*- coding: utf-8 -*-

#Copyright (c) Manish Singh
#javascript animation support by Joao S. O. Bueno Calligaris (2004)

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 2003, 2005  Manish Singh <yosh@gimp.org>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# (c) 2003 Manish Singh.
#"Guillotine implemented ala python, with html output
# (based on perlotine by Seth Burgess)",
# Modified by João S. O. Bueno Calligaris to allow  dhtml animations (2005)

import os

from gimpfu import *
import os.path

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def pyslice(image, drawable, save_path, html_filename,
            image_basename, image_extension, separate,
            image_path, cellspacing, animate, skip_caps):

    cellspacing = int (cellspacing)

    if animate:
        count = 0
        drw = []
        #image.layers is a reversed list of the layers on the image
        #so, count indexes from number of layers to 0.
        for i in xrange (len (image.layers) -1, -1, -1):
            if image.layers[i].visible:
                drw.append(image.layers[i])
                count += 1
                if count == 3:
                    break


    vert, horz = get_guides(image)

    if len(vert) == 0 and len(horz) == 0:
        return

    gimp.progress_init(_("Slice"))
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
        if not image_relative_path.endswith("/"):
            image_relative_path += "/"
        image_path = check_path(os.path.join(save_path, image_path))
    else:
        image_relative_path = ''
        image_path = save_path

    tw = TableWriter(os.path.join(save_path, html_filename),
                     cellspacing=cellspacing, animate=animate)

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
            if (skip_caps   and
                 (
                   (len(horz) >= 2 and (i == 0 or i == len(horz) )) or
                   (len(vert) >= 2 and (j == 0 or j == len(vert) ))
                 )
               ):
                skip_stub = True
            else:
                skip_stub = False

            if (not animate or skip_stub):
                src = (image_relative_path +
                       slice (image, None, image_path,
                              image_basename, image_extension,
                              left, right, top, bottom, i, j, ""))
            else:
                src = []
                for layer, postfix in zip (drw, ("", "hover", "clicked")):
                    src.append (image_relative_path +
                                slice(image, layer, image_path,
                                      image_basename, image_extension,
                                      left, right, top, bottom, i, j, postfix))

            tw.cell(src, right - left, bottom - top, i, j, skip_stub)

            left = right + cellspacing

            progress += progress_increment
            gimp.progress_update(progress)

        tw.row_end()

        top = bottom + cellspacing

    tw.close()

def slice(image, drawable, image_path, image_basename, image_extension,
          left, right, top, bottom, i, j, postfix):
    if postfix:
        postfix = "_" + postfix
    src = "%s_%d_%d%s.%s" % (image_basename, i, j, postfix, image_extension)
    filename = os.path.join(image_path, src)

    if not drawable:
        temp_image = image.duplicate()
        temp_drawable = temp_image.active_layer
    else:
        if image.base_type == INDEXED:
            #gimp_layer_new_from_drawable doesn't work for indexed images.
            #(no colormap on new images)
            original_active = image.active_layer
            image.active_layer = drawable
            temp_image = image.duplicate()
            temp_drawable = temp_image.active_layer
            image.active_layer = original_active
            temp_image.disable_undo()
            #remove all layers but the intended one
            while len (temp_image.layers) > 1:
                if temp_image.layers[0] != temp_drawable:
                    pdb.gimp_image_remove_layer (temp_image, temp_image.layers[0])
                else:
                    pdb.gimp_image_remove_layer (temp_image, temp_image.layers[1])
        else:
            temp_image = pdb.gimp_image_new (drawable.width, drawable.height,
                                         image.base_type)
            temp_drawable = pdb.gimp_layer_new_from_drawable (drawable, temp_image)
            temp_image.insert_layer (temp_drawable)

    temp_image.disable_undo()
    temp_image.crop(right - left, bottom - top, left, top)
    if image_extension == "gif" and image.base_type == RGB:
        pdb.gimp_image_convert_indexed (temp_image,NO_DITHER, MAKE_PALETTE, 255,
                                        True, False, False)
    if image_extension == "jpg" and image.base_type == INDEXED:
        pdb.gimp_image_convert_rgb (temp_image)

    pdb.gimp_file_save(temp_image, temp_drawable, filename, filename)

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
                 animate=False):

        self.filename = filename
        self.table_attrs = {}

        #Hellraisen IE 6 doesn't support CSS for table control.
        self.table_attrs['cellpadding'] = cellpadding
        self.table_attrs['cellspacing'] = cellspacing
        self.table_attrs['border'] = border

        self.image_prefix = os.path.basename (filename)
        self.image_prefix = self.image_prefix.split(".")[0]
        self.image_prefix = self.image_prefix.replace ("-", "_")
        self.image_prefix = self.image_prefix.replace (" ", "_")


        if animate:
            self.animate = True
            self.images = []
        else:
            self.animate = False

        if os.path.exists (filename):
            #The plug-in is running to overwrite a previous
            #version of the file. This will parse the href targets already
            #in the file to preserve them.
            self.urls = self.parse_urls ()
        else:
            self.urls = []

        self.url_index = 0

        self.html = open(filename, 'wt')
        self.open()

    def next_url (self):
        if self.url_index < len (self.urls):
            self.url_index += 1
            return self.urls [self.url_index - 1]
        else:
            #Default url to use in the anchor tags:
            return ("#")

    def write(self, s, vals=None):
        if vals:
            s = s % vals

        self.html.write(s + '\n')

    def open(self):
        out = '''<!--HTML SNIPPET GENERATED BY GIMP

WARNING!! This is NOT a fully valid HTML document, it is rather a piece of
HTML generated by GIMP's py-slice plugin that should be embedded in an HTML
or XHTML document to be valid.

Replace the href targets in the anchor (<a >) for your URLS to have it working
as a menu.
 -->\n'''
        out += '<table'

        for attr, value in self.table_attrs.iteritems():
            out += ' %s="%s"' % (attr, value)

        out += '>'

        self.write(out)

    def close(self):
        self.write('</table>\n')
        prefix = self.image_prefix
        if self.animate:
            out = """
<script language="javascript" type="text/javascript">
/* Made with GIMP */

/* Preload images: */
    images_%s = new Array();
                   \n"""    % prefix
            for image in self.images:
                for type_ in ("plain", "hover", "clicked"):
                    if image.has_key(type_):
                        image_index = ("%d_%d_%s" %
                                       (image["index"][0],
                                        image["index"][1], type_))
                        out += ("    images_%s[\"%s\"] = new  Image();\n" %
                                (prefix, image_index))
                        out += ("    images_%s[\"%s\"].src = \"%s\";\n" %
                            (prefix, image_index, image[type_]))

            out+= """
function exchange (image, images_array_name, event)
  {
    name = image.name;
    images = eval (images_array_name);

    switch (event)
      {
        case 0:
          image.src = images[name + "_plain"].src;
          break;
        case 1:
          image.src = images[name + "_hover"].src;
          break;
        case 2:
          image.src = images[name + "_clicked"].src;
          break;
        case 3:
          image.src = images[name + "_hover"].src;
          break;
      }

  }
</script>
<!--
End of the part generated by GIMP
-->
"""
            self.write (out)


    def row_start(self):
        self.write('  <tr>')

    def row_end(self):
        self.write('</tr>\n')

    def cell(self, src, width, height, row=0, col=0, skip_stub = False):
        if isinstance (src, list):
            prefix = "images_%s" % self.image_prefix
            self.images.append ({"index" : (row, col), "plain" : src[0]})

            out = ('    <td><a href="%s"><img alt="" src="%s" ' +
                  'style="width: %dpx; height: %dpx; border-width: 0px" \n') %\
                  (self.next_url(), src[0], width, height)
            out += 'name="%d_%d" \n' % (row, col)
            if len(src) >= 2:
                self.images[-1]["hover"] = src [1]
                out += """      onmouseout="exchange(this, '%s', 0);"\n""" % \
                       prefix
                out += """      onmouseover="exchange(this, '%s', 1);"\n""" % \
                       prefix
            if len(src) >= 3:
                self.images[-1]["clicked"] = src [2]
                out += """      onmousedown="exchange(this, '%s', 2);"\n""" % \
                       prefix
                out += """      onmouseup="exchange(this, '%s', 3);"\n""" % \
                       prefix



            out += "/></a></td>\n"

        else:
            if skip_stub:
                out =  ('    <td><img alt=" " src="%s" style="width: %dpx; ' +
                        ' height: %dpx; border-width: 0px;"></td>') % \
                        (src, width, height)
            else:
                out = ('    <td><a href="#"><img alt=" " src="%s" ' +
                      ' style="width: %dpx; height: %dpx; border-width: 0px;">' +
                      '</a></td>') %  (src, width, height)
        self.write(out)
    def parse_urls (self):
        """
           This will parse any url targets in the href="XX" fields
           of the given file and return then as a list
        """
        import re
        url_list = []
        try:
            html_file = open (self.filename)

            # Regular expression to pick everything up to the next
            # doublequote character after finding the sequence 'href="'.
            # The found sequences will be returned as a list by the
            # "findall" method.
            expr = re.compile (r"""href\=\"([^\"]*?)\"""")
            url_list = expr.findall (html_file.read (2 ** 18))
            html_file.close()

        except:
            # silently ignore any errors parsing this. The file being
            # overwritten may not be a file created by py-slice.
            pass

        return url_list


register(
    "python-fu-slice",
    # table snippet means a small piece of HTML code here
    N_("Cuts an image along its guides, creates images and a HTML table snippet"),
    """Add guides to an image. Then run this. It will cut along the guides,
    and give you the html to reassemble the resulting images. If you
    choose to generate javascript for onmouseover and clicked events, it
    will use the lower three visible layers on the image for normal,
    onmouseover and clicked states, in that order. If skip caps is
    enabled, table cells on the edge of the table won't become animated,
    and its images will be taken from the active layer.""",
    "Manish Singh",
    "Manish Singh",
    "2003",
    _("_Slice..."),
    "*",
    [
        (PF_IMAGE, "image", "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
        (PF_DIRNAME, "save-path",     _("Path for HTML export"), os.getcwd()),
        (PF_STRING, "html-filename",  _("Filename for export"),  "slice.html"),
        (PF_STRING, "image-basename", _("Image name prefix"),    "slice"),
        (PF_RADIO, "image-extension", _("Image format"),         "gif", (("gif", "gif"), ("jpg", "jpg"), ("png", "png"))),
        (PF_TOGGLE, "separate-image-dir",  _("Separate image folder"),
         False),
        (PF_STRING, "relative-image-path", _("Folder for image export"), "images"),
        (PF_SPINNER, "cellspacing", _("Space between table elements"), 0,
        (0,15,1)),
        (PF_TOGGLE, "animate",      _("Javascript for onmouseover and clicked"),
         False),
        # table caps are table cells on the edge of the table
        (PF_TOGGLE, "skip-caps",    _("Skip animation for table caps"), True)
    ],
    [],
    pyslice,
    menu="<Image>/Filters/Web",
    domain=("gimp20-python", gimp.locale_directory)
    )

main()
