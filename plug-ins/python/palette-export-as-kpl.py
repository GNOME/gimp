#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#   Allows exporting Krita's .kpl palettes
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
import os, sys, tempfile, zipfile
import xml.etree.ElementTree as ET

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

# Taken from file-openraster.py
def write_file_str(zfile, fname, data):
    # work around a permission bug in the zipfile library:
    # http://bugs.python.org/issue3394
    zi = zipfile.ZipInfo(fname)
    zi.external_attr = int("100644", 8) << 16
    zfile.writestr(zi, data)

def palette_kpl_export(procedure, config, data):
    success = True
    runmode = config.get_property("run-mode")

    if runmode == Gimp.RunMode.INTERACTIVE:
        gi.require_version('GimpUi', '3.0')
        from gi.repository import GimpUi
        gi.require_version('Gtk', '3.0')
        from gi.repository import Gtk

        GimpUi.init('python-fu-palette-export-as-kpl')
        dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)

        # Fill the dialog.
        config.set_property("palette", None)
        config.set_property("file", None)

        dialog.get_label ("options-label", _("Options"), False, False)
        hbox = dialog.fill_box ("options-box", ["comment", "read-only"])
        dialog.fill_frame("options-frame", "options-label", False, "options-box")

        dialog.fill (["palette", "file", "options-frame"])
        dialog.set_ok_label (_("_Export"))

        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())

    palette   = config.get_property("palette")
    file      = config.get_property("file")
    comment   = config.get_property("comment")
    read_only = config.get_property("read-only")

    if file is None or file.peek_path() is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    tempdir = tempfile.mkdtemp('python-fu-palette-export-as-kpl')

    kplfile = zipfile.ZipFile(file.peek_path() + '.tmpsave', 'w', compression=zipfile.ZIP_STORED)

    write_file_str(kplfile, 'mimetype', 'application/x-krita-palette') # must be the first file written

    # Filling in palette
    xml_palette = ET.Element('ColorSet')
    a = xml_palette.attrib
    if comment != "":
        a['comment'] = comment
    a['name'] = palette.get_name()
    a['version'] = "1.0"
    a['readonly'] = "true" if read_only == True else "false"
    columns = palette.get_columns()
    if columns == 0:
        columns = 16
    a['columns'] = str(columns)

    count = palette.get_color_count()

    row = 0
    col = 0
    for i in range(count):
        entry = ET.Element('ColorSetEntry')
        xml_palette.append(entry)

        success, name = palette.get_entry_name(i)
        if success:
            a = entry.attrib
            a['name'] = name
            a['id'] = str(i + 1)
            # TODO: Add options for different bit-depths
            a['bitdepth'] = "F32"
            a['spot'] = "false"

            color = palette.get_entry_color(i)
            # TODO: Allow for other color spaces
            pal = ET.Element('RGB')
            entry.append(pal)
            a = pal.attrib

            r, g, b, alpha = color.get_rgba()
            a['r'] = str(r)
            a['g'] = str(g)
            a['b'] = str(b)

            pos = ET.Element('Position')
            entry.append(pos)
            a = pos.attrib
            a['row'] = str(row)
            a['column'] = str(col)

            col += 1
            if col >= columns:
                col = 0
                row += 1
        else:
            break;

    if success:
        # Write everything to file
        xml = ET.tostring(xml_palette, encoding='UTF-8')
        write_file_str(kplfile, 'colorset.xml', xml)

        kplfile.close()
        os.rmdir(tempdir)
        if os.path.exists(file.peek_path()):
            os.remove(file.peek_path()) # win32 needs that
        os.rename(file.peek_path() + '.tmpsave', file.peek_path())

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
    else:
        kplfile.close()
        os.rmdir(tempdir)
        return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                           GLib.Error('File exporting failed: {}'.format(file.get_path())))

class PaletteExportAsKPL (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [ 'python-fu-palette-export-as-kpl' ]

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       palette_kpl_export, None)
        if name == 'python-fu-palette-export-as-kpl':
            procedure.set_documentation (_("Export palette as Krita .kpl"),
                                         _("Export palette as Krita .kpl"),
                                         name)
            procedure.set_menu_label(_("_Krita palette..."))
            procedure.set_attribution("Alx Sa",
                                      "(c) GPL V3.0 or later",
                                      "2025")
            procedure.add_menu_path('<Palettes>/Palettes Menu/Export as')

            procedure.add_enum_argument ("run-mode", _("Run mode"),
                                         _("The run mode"), Gimp.RunMode,
                                         Gimp.RunMode.NONINTERACTIVE,
                                         GObject.ParamFlags.READWRITE)
            procedure.add_palette_argument ("palette", _("_Palette to export"),
                                             "", True,
                                             None, True, # Default to context.
                                             GObject.ParamFlags.READWRITE)
            procedure.add_file_argument ("file", _("_File (.kpl)"), "",
                                         Gimp.FileChooserAction.SAVE,
                                         False, None, GObject.ParamFlags.READWRITE)
            procedure.add_string_argument ("comment", _("Commen_t"),
                                           _("Optional comment"), "",
                                           GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("read-only",
                                            _("Re_ad only"),
                                            _("The palette will be locked on import"),
                                            False, GObject.ParamFlags.READWRITE)
        return procedure

Gimp.main(PaletteExportAsKPL.__gtype__, sys.argv)
