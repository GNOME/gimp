#!/usr/bin/env python
# coding: utf-8

# Author: João Sebastião de Oliveira Bueno
# Copyright: João S. O. Bueno (2009), licensed under the GPL v 3.0

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

from gimpfu import *
import os

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def text_brush(font_name, font_size, text):
    pdb.gimp_context_push()
    pdb.gimp_context_set_default_colors()
    
    padding = font_size // 4
    img = gimp.Image(font_size + padding, font_size + padding, GRAY)
    img.undo_freeze()
    
    text = text.decode("utf-8")
    for letter in reversed(text):
        layer = img.new_layer(fill_mode=BACKGROUND_FILL)
        text_floating_sel = \
            pdb.gimp_text_fontname(img, layer, 
                                   padding // 2,
                                   padding // 2,
                                   letter.encode("utf-8"),
                                   0,
                                   True,
                                   font_size,
                                   PIXELS, 
                                   font_name)
        if text_floating_sel: 
            #whitespace don't generate a floating sel.
            pdb.gimp_edit_bucket_fill(text_floating_sel,
                                  FG_BUCKET_FILL,
                                  NORMAL_MODE, 100, 1.0,
                                  False,0 ,0)
            pdb.gimp_floating_sel_anchor(text_floating_sel)

    file_name = text.lower().replace(" ", "_") + ".gih"
    file_path = os.path.join(gimp.directory, 'brushes', file_name)
    
    pdb.file_gih_save(img, img.layers[0], 
                      file_path, file_path, 
                      100, #spacing
                      text, #description,
                      img.width, img.height,
                      1, 1,
                      1, #dimension
                      [len(text)], #rank - number of cells 
                      1, # dimension again - actual size for the
                         # array of the selection mode
                      ["incremental"])

    pdb.gimp_brushes_refresh()
    pdb.gimp_image_delete(img)
    pdb.gimp_context_pop()

register(
         "text_brush",
         N_("New brush with characters from a text sequence"),
         """New dynamic brush where each cell is a character from 
the input text in the chosen font """,
         "Joao S. O. Bueno",
         "Copyright Joao S.O. Bueno 2009. GPL v3.0",
         "2009",
         N_("New _Brush from Text..."),
         "",
         [
            (PF_FONT, "font", "Font","Sans"),
            (PF_INT, "size", "Pixel Size", 50),
            (PF_STRING, "text", "text",
             "The GNU Image Manipulation Program")
         ],
         [],
         text_brush,
         menu="<Image>/File/Create",
         domain=("gimp20-python", gimp.locale_directory)
        )
main()
