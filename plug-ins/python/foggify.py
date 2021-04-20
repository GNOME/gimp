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
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import time
import sys

import gettext
_ = gettext.gettext
def N_(message): return message

def foggify(procedure, run_mode, image, n_drawables, drawables, args, data):
    config = procedure.create_config()
    config.begin_run(image, run_mode, args)

    if run_mode == Gimp.RunMode.INTERACTIVE:
        GimpUi.init('python-fu-foggify')
        dialog = GimpUi.ProcedureDialog.new(procedure, config)
        # Even though gimp_procedure_dialog_get_color_widget() is
        # transfer none, somehow if I don't set it to a variable PyGObject
        # frees it, then the plug-in fails at dialog.fill().
        b = dialog.get_color_widget('color', True, GimpUi.ColorAreaType.FLAT)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            config.end_run(Gimp.PDBStatusType.CANCEL)
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())
        else:
            dialog.destroy()

    color      = config.get_property('color')
    name       = config.get_property('name')
    turbulence = config.get_property('turbulence')
    opacity    = config.get_property('opacity')

    Gimp.context_push()
    image.undo_group_start()

    if image.get_base_type() is Gimp.ImageBaseType.RGB:
        type = Gimp.ImageType.RGBA_IMAGE
    else:
        type = Gimp.ImageType.GRAYA_IMAGE
    for drawable in drawables:
        fog = Gimp.Layer.new(image, name,
                             drawable.get_width(), drawable.get_height(),
                             type, opacity,
                             Gimp.LayerMode.NORMAL)
        fog.fill(Gimp.FillType.TRANSPARENT)
        image.insert_layer(fog, drawable.get_parent(),
                           image.get_item_position(drawable))

        Gimp.context_set_background(color)
        fog.edit_fill(Gimp.FillType.BACKGROUND)

        # create a layer mask for the new layer
        mask = fog.create_mask(0)
        fog.add_mask(mask)

        # add some clouds to the layer
        Gimp.get_pdb().run_procedure('plug-in-plasma', [
            GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE),
            GObject.Value(Gimp.Image, image),
            GObject.Value(Gimp.Drawable, mask),
            GObject.Value(GObject.TYPE_INT, int(time.time())),
            GObject.Value(GObject.TYPE_DOUBLE, turbulence),
        ])

        # apply the clouds to the layer
        fog.remove_mask(Gimp.MaskApplyMode.APPLY)
        fog.set_visible(True)

    Gimp.displays_flush()

    image.undo_group_end()
    Gimp.context_pop()

    config.end_run(Gimp.PDBStatusType.SUCCESS)

    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

_color = Gimp.RGB()
_color.set(240.0, 0, 0)

class Foggify (Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "name": (str,
                 _("Layer _name"),
                 _("Layer name"),
                 _("Clouds"),
                 GObject.ParamFlags.READWRITE),
        "turbulence": (float,
                       _("_Turbulence"),
                       _("Turbulence"),
                       0.0, 10.0, 1.0,
                       GObject.ParamFlags.READWRITE),
        "opacity": (float,
                    _("O_pacity"),
                    _("Opacity"),
                    0.0, 100.0, 100.0,
                    GObject.ParamFlags.READWRITE),
    }
    # I use a different syntax for this property because I think it is
    # supposed to allow setting a default, except it doesn't seem to
    # work. I still leave it this way for now until we figure this out
    # as it should be the better syntax.
    color = GObject.Property(type =Gimp.RGB, default=_color,
                             nick =_("Fog _color"),
                             blurb=_("Fog color"))

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
        procedure.set_sensitivity_mask (Gimp.ProcedureSensitivityMask.DRAWABLE |
                                        Gimp.ProcedureSensitivityMask.DRAWABLES)
        procedure.set_documentation (N_("Add a layer of fog"),
                                     "Adds a layer of fog to the image.",
                                     name)
        procedure.set_menu_label(N_("_Fog..."))
        procedure.set_attribution("James Henstridge",
                                  "James Henstridge",
                                  "1999,2007")
        procedure.add_menu_path ("<Image>/Filters/Decor")

        procedure.add_argument_from_property(self, "name")
        procedure.add_argument_from_property(self, "color")
        procedure.add_argument_from_property(self, "turbulence")
        procedure.add_argument_from_property(self, "opacity")
        return procedure

Gimp.main(Foggify.__gtype__, sys.argv)
