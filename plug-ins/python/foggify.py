#!/usr/bin/env python3
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
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
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import time
import sys

import gettext
_ = gettext.gettext
def N_(message): return message

def foggify(procedure, run_mode, image, drawable, args, data):
    name = args.index(0)
    turbulence = args.index(1)
    opacity = args.index(2)
    if run_mode == Gimp.RunMode.INTERACTIVE:
        # TODO: add a GUI. This first port works just with default
        # values.
        color = Gimp.RGB()
        color.set(240.0, 180.0, 70.0)

    Gimp.context_push()
    image.undo_group_start()

    if image.base_type() is Gimp.ImageBaseType.RGB:
        type = Gimp.ImageType.RGBA_IMAGE
    else:
        type = Gimp.ImageType.GRAYA_IMAGE
    fog = Gimp.Layer.new(image, name,
                         drawable.width(), drawable.height(), type, opacity,
                         Gimp.LayerMode.NORMAL)
    fog.fill(Gimp.FillType.TRANSPARENT)
    image.insert_layer(fog, None, 0)

    Gimp.context_set_background(color)
    fog.edit_fill(Gimp.FillType.BACKGROUND)

    # create a layer mask for the new layer
    mask = fog.create_mask(0)
    fog.add_mask(mask)

    # add some clouds to the layer
    args = Gimp.ValueArray.new(5)
    args.insert(0, GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE))
    args.insert(1, GObject.Value(Gimp.Image, image))
    args.insert(2, GObject.Value(Gimp.Drawable, mask))
    args.insert(3, GObject.Value(GObject.TYPE_INT, int(time.time())))
    args.insert(4, GObject.Value(GObject.TYPE_DOUBLE, turbulence))
    Gimp.get_pdb().run_procedure('plug-in-plasma', args)

    # apply the clouds to the layer
    fog.remove_mask(Gimp.MaskApplyMode.APPLY)
    fog.set_visible(True)

    image.undo_group_end()
    Gimp.context_pop()

    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

class Foggify (Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "name": (str,
                 _("Layer name"),
                 _("Layer name"),
                 _("Clouds"),
                 GObject.ParamFlags.READWRITE),
        "color": (Gimp.RGB,
                  _("Fog color"),
                  _("Fog color"),
                  GObject.ParamFlags.READWRITE),
        "turbulence": (float,
                       _("Turbulence"),
                       _("Turbulence"),
                       0.0, 10.0, 1.0,
                       GObject.ParamFlags.READWRITE),
        "opacity": (float,
                    _("Opacity"),
                    _("Opacity"),
                    0.0, 100.0, 100.0,
                    GObject.ParamFlags.READWRITE),
    }

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        self.set_translation_domain("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))

        return [ 'python-fu-foggify' ]

    def do_create_procedure(self, name):
        procedure = Gimp.ImageProcedure.new(self, name,
                                            Gimp.PDBProcType.PLUGIN,
                                            foggify, None)
        procedure.set_image_types("RGB*, GRAY*");
        procedure.set_documentation (N_("Add a layer of fog"),
                                     "Adds a layer of fog to the image.",
                                     name)
        procedure.set_menu_label(N_("_Fog..."))
        procedure.set_attribution("James Henstridge",
                                  "James Henstridge",
                                  "1999,2007")
        procedure.add_menu_path ("<Image>/Filters/Decor")

        procedure.add_argument_from_property(self, "name")
        # TODO: add support for GBoxed values.
        #procedure.add_argument_from_property(self, "color")
        procedure.add_argument_from_property(self, "turbulence")
        procedure.add_argument_from_property(self, "opacity")
        return procedure

Gimp.main(Foggify.__gtype__, sys.argv)
