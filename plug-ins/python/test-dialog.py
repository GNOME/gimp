#!/usr/bin/env python3
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


'''
A Python plugin.
Tests GimpProcedureDialog.

Temporarily, just testing widgets for subclasses of GimpResource.
Temporarily, just testing Brush subclass of Resource.
FUTURE: For all the parameter types.
Formerly PF_ constants, now all parameters for which GimpParamSpecs exist.

Not localized, no i18n
'''


def process_args(brush, font, gradient, palette, pattern):
    '''
    Test the args are sane.
    '''
    assert brush is not None
    assert isinstance(brush, Gimp.Brush)
    id = brush.get_id()
    assert id is not None
    print("Brush id is:", id)
    Gimp.message(id)

    assert font is not None
    assert isinstance(font, Gimp.Font)
    id = font.get_id()
    assert id is not None
    print("font id is:", id)
    Gimp.message(id)

    assert gradient is not None
    assert isinstance(gradient, Gimp.Gradient)
    id = gradient.get_id()
    assert id is not None
    print("gradient id is:", id)
    Gimp.message(id)

    assert palette is not None
    assert isinstance(palette, Gimp.Palette)
    id = palette.get_id()
    assert id is not None
    print("palette id is:", id)
    Gimp.message(id)

    assert pattern is not None
    assert isinstance(pattern, Gimp.Pattern)
    id = pattern.get_id()
    assert id is not None
    print("pattern id is:", id)
    Gimp.message(id)


    return



def test_dialog(procedure, run_mode, image, n_drawables, drawables, args, data):
    '''
    Just a standard shell for a plugin.
    '''
    config = procedure.create_config()
    config.begin_run(image, run_mode, args)

    if run_mode == Gimp.RunMode.INTERACTIVE:
        GimpUi.init('python-fu-test-dialog')
        dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            config.end_run(Gimp.PDBStatusType.CANCEL)
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())
        else:
            dialog.destroy()

    brush = config.get_property('brush')
    font = config.get_property('font')
    gradient = config.get_property('gradient')
    palette = config.get_property('palette')
    pattern = config.get_property('pattern')

    Gimp.context_push()
    image.undo_group_start()

    process_args(brush, font, gradient, palette, pattern)

    Gimp.displays_flush()

    image.undo_group_end()
    Gimp.context_pop()

    config.end_run(Gimp.PDBStatusType.SUCCESS)

    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())



class TestDialogPlugin (Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {

    }
    # See comments about this in foggify.py, from which we borrowed

    brush = GObject.Property(type =Gimp.Brush,
                             nick ="Brush",
                             blurb="Brush")
    font = GObject.Property(type =Gimp.Font,
                            nick ="Font",
                            blurb="Font")
    gradient = GObject.Property(type =Gimp.Gradient,
                                nick ="Gradient",
                                blurb="Gradient")
    palette = GObject.Property(type =Gimp.Palette,
                               nick ="Palette",
                               blurb="Palette")
    pattern = GObject.Property(type =Gimp.Pattern,
                               nick ="Pattern",
                               blurb="Pattern")

    # FUTURE all other Gimp classes that have GimpParamSpecs

    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [ 'python-fu-test-dialog' ]

    def do_create_procedure(self, name):
        procedure = Gimp.ImageProcedure.new(self, name,
                                            Gimp.PDBProcType.PLUGIN,
                                            test_dialog, None)
        procedure.set_image_types("*");
        procedure.set_sensitivity_mask (Gimp.ProcedureSensitivityMask.DRAWABLE |
                                        Gimp.ProcedureSensitivityMask.DRAWABLES)
        procedure.set_documentation ("Test dialog",
                                     "Test dialog",
                                     name)
        procedure.set_menu_label("Test dialog...")
        procedure.set_attribution("Lloyd Konneker",
                                  "Lloyd Konneker",
                                  "2022")
        # Top level menu "Test"
        procedure.add_menu_path ("<Image>/Test")

        procedure.add_argument_from_property(self, "brush")
        procedure.add_argument_from_property(self, "font")
        procedure.add_argument_from_property(self, "gradient")
        procedure.add_argument_from_property(self, "palette")
        procedure.add_argument_from_property(self, "pattern")

        return procedure

Gimp.main(TestDialogPlugin.__gtype__, sys.argv)
