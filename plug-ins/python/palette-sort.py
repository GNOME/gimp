#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


# For a detailed explanation of the parameters of this plugin, see :
# https://gitlab.gnome.org/GNOME/gimp/-/issues/4368#note_763460

# little known, colorsys is part of Python's stdlib
from colorsys import rgb_to_yiq
from textwrap import dedent
from random import randint
import struct

import gi
gi.require_version('Babl', '0.1')
from gi.repository import Babl
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


AVAILABLE_CHANNELS = (_("Red"), _("Green"), _("Blue"),
                      _("Luma (Y)"),
                      _("Hue"), _("Saturation"), _("Value"),
                      _("Saturation (HSL)"), _("Lightness (HSL)"),
                      _("Index"),
                      _("Random"))

channel_getters = [
    (lambda v, i: v.get_rgba()[0]),
    (lambda v, i: v.get_rgba()[1]),
    (lambda v, i: v.get_rgba()[2]),

    (lambda v, i: rgb_to_yiq(v.get_rgba()[0], v.get_rgba()[1], v.get_rgba()[1])[0]),

    # TODO: Replace with gegl_color_get_hsv () when available in Gegl
    (lambda v, i: gegl_color_bytes_convert (v, "HSV float", 'fff', 0)),
    (lambda v, i: gegl_color_bytes_convert (v, "HSV float", 'fff', 1)),
    (lambda v, i: gegl_color_bytes_convert (v, "HSV float", 'fff', 2)),

    (lambda v, i: gegl_color_bytes_convert (v, "HSL float", 'fff', 1)),
    (lambda v, i: gegl_color_bytes_convert (v, "HSL float", 'fff', 2)),

    (lambda v, i: i),
    (lambda v, i: randint(0, 0x7fffffff)),

    (lambda v, i: gegl_color_bytes_convert (v, "CIE Lab float", 'fff', 0)),
    (lambda v, i: gegl_color_bytes_convert (v, "CIE Lab float", 'fff', 1)),
    (lambda v, i: gegl_color_bytes_convert (v, "CIE Lab float", 'fff', 2)),

    (lambda v, i: gegl_color_bytes_convert (v, "CIE LCH(ab) float", 'fff', 1)),
    (lambda v, i: gegl_color_bytes_convert (v, "CIE LCH(ab) float", 'fff', 2))
]

GRAIN_SCALE = (1.0, 1.0 , 1.0,
              1.0,
              360., 100., 100.,
              100., 100.,
              16384.,
              float(0x7ffffff),
              100., 256., 256.,
              256., 360.,)

def gegl_color_bytes_convert (color, format, precision, index):
    color_bytes = color.get_bytes(Babl.format(format))
    data = color_bytes.get_data()
    result = struct.unpack (precision, data)

    return result[index]

slice_expr_doc = N_("""
    Format is 'start:nrows,length' . All items are optional.

    The empty string selects all items, as does ':'
    ':4,' makes a 4-row selection out of all colors (length auto-determined)
    ':4' also.
    ':1,4' selects the first 4 colors
    ':,4' selects rows of 4 colors (nrows auto-determined)
    ':3,4' selects 3 rows of 4 colors
    '4:' selects a single row of all colors after 4, inclusive.
    '3:,4' selects rows of 4 colors, starting at 3 (nrows auto-determined)
    '2:3,4' selects 3 rows of 4 colors (12 colors total), beginning at index 2.
    '4' is illegal (ambiguous)
""")


def parse_slice(s, numcolors):
    """Parse a slice spec and return (start, nrows, length)
    All items are optional. Omitting them makes the largest possible selection that
    exactly fits the other items.

    In general, slices are comparable to a numpy sub-array.
    'start at element START, with shape (NROWS, LENGTH)'

    """
    s = s.strip()

    def notunderstood():
        raise ValueError('Slice %r not understood. Should be in format'
                         ' START?:NROWS?,ROWLENGTH? eg. "0:4,16".' % s)
    def _int(v):
        try:
            return int(v)
        except ValueError:
            notunderstood()
    if s in ('', ':', ':,'):
        return 0, 1, numcolors  # entire palette, one row
    if s.count(':') != 1:
        notunderstood()
    rowpos = s.find(':')
    start = 0
    if rowpos > 0:
        start = _int(s[:rowpos])
    numcolors -= start
    nrows = 1
    if ',' in s:
        commapos = s.find(',')
        nrows = s[rowpos+1:commapos]
        length = s[commapos+1:]
        if not nrows:
            if not length:
                notunderstood()
            else:
                length = _int(length)
                if length == 0:
                    notunderstood()
                nrows = numcolors // length
                if numcolors % length:
                    nrows = -nrows
        elif not length:
            nrows = _int(nrows)
            if nrows == 0:
                notunderstood()
            length = numcolors // nrows
            if numcolors % nrows:
                length = -length
        else:
            nrows = _int(nrows)
            if nrows == 0:
                notunderstood()
            length = _int(length)
            if length == 0:
                notunderstood()
    else:
        nrows = _int(s[rowpos+1:])
        if nrows == 0:
            notunderstood()
        length = numcolors // nrows
        if numcolors % nrows:
            length = -length
    return start, nrows, length


def quantization_grain(channel, g):
    "Given a channel and a quantization, return the size of a quantization grain"
    g = max(1.0, g)
    if g <= 1.0:
        g = 0.00001
    else:
        g = max(0.00001, GRAIN_SCALE[channel] / g)
    return g


def palette_sort(palette, selection, slice_expr, channel1, ascending1,
                 channel2, ascending2, quantize, pchannel, pquantize):

    grain1 = quantization_grain(channel1, quantize)
    grain2 = quantization_grain(channel2, quantize)
    pgrain = quantization_grain(pchannel, pquantize)

    # If palette is read only, work on a copy:
    editable = palette.is_editable()
    if not editable:
        palette = palette.duplicate()

    num_colors = palette.get_color_count()

    start, nrows, length = None, None, None
    if selection == "auto-slice":
        def find_index(color, startindex=0):
            for i in range(startindex, num_colors):
                rgba = palette.get_entry_color(i)

                if hexcolor(rgba) == hexcolor(color):
                    return i
            return None
        def hexcolor(c):
            rgba = c.get_rgba()
            return "#%02x%02x%02x" % (int(255 * rgba[0]), int(255 * rgba[1]), int(255 * rgba[2]))
        fg = Gimp.context_get_foreground()
        bg = Gimp.context_get_background()
        start = find_index(fg)
        end = find_index(bg)

        if start is None:
            raise ValueError("Couldn't find foreground color %s in palette" % hexcolor(fg))
        if end is None:
            raise ValueError("Couldn't find background color %s in palette" % hexcolor(bg))
        if find_index(fg, start + 1):
            raise ValueError('Autoslice cannot be used when more than one'
                             ' instance of an endpoint'
                             ' (%s) is present' % hexcolor(fg))
        if find_index(bg, end + 1):
            raise ValueError('Autoslice cannot be used when more than one'
                             ' instance of an endpoint'
                             ' (%s) is present' % hexcolor(bg))
        if start > end:
            end, start = start, end
        length = (end - start) + 1
        try:
            _, nrows, _ = parse_slice(slice_expr, length)
            nrows = abs(nrows)
            if length % nrows:
                raise ValueError('Total length %d not evenly divisible'
                                 ' by number of rows %d' % (length, nrows))
            length //= nrows
        except ValueError:
            # bad expression is okay here, just assume one row
            nrows = 1
        # remaining behavior is implemented by "slice-array" 'inheritance'.
        selection = "slice-array"
    elif selection in ("slice-array", "partitioned"):
        start, nrows, length = parse_slice(slice_expr, num_colors)

    channels_getter_1 = channel_getters[channel1]
    channels_getter_2 = channel_getters[channel2]

    def get_colors(start, end):
        result = []
        for i in range(start, end):
            entry =  (palette.get_entry_name(i)[1],
                      palette.get_entry_color(i))
            index1 = channels_getter_1(entry[1], i)
            index2 = channels_getter_2(entry[1], i)
            index = ((index1 - (index1 % grain1)) * (1 if ascending1 else -1),
                     (index2 - (index2 % grain2)) * (1 if ascending2 else -1)
                    )
            result.append((index, entry))
        return result

    if selection == "all":
        entry_list = get_colors(0, num_colors)
        entry_list.sort(key=lambda v: v[0])
        for i in range(num_colors):
            palette.set_entry_name(i, entry_list[i][1][0])
            palette.set_entry_color(i, entry_list[i][1][1])

    elif selection == "partitioned":
        if num_colors < (start + length * nrows) - 1:
            raise ValueError('Not enough entries in palette to '
                             'sort complete rows! Got %d, expected >=%d' %
                             (num_colors, start + length * nrows))
        pchannels_getter = channel_getters[pchannel]
        for row in range(nrows):
            partition_spans = [1]
            rowstart = start + (row * length)
            old_color = palette.get_entry_color(rowstart)
            old_partition = pchannels_getter(old_color, rowstart)
            old_partition = old_partition - (old_partition % pgrain)
            for i in range(rowstart + 1, rowstart + length):
                this_color = palette.get_entry_color(i)
                this_partition = pchannels_getter(this_color, i)
                this_partition = this_partition - (this_partition % pgrain)
                if this_partition == old_partition:
                    partition_spans[-1] += 1
                else:
                    partition_spans.append(1)
                    old_partition = this_partition
            base = rowstart
            for size in partition_spans:
                palette_sort(palette, "slice-array", '%d:1,%d' % (base, size),
                             channel1, ascending1,
                             channel2, ascending2,
                             quantize, 0, 1.0)
                base += size
    else:
        # "slice-array" and "auto-slice"
        stride = length
        if num_colors < (start + stride * nrows) - 1:
            raise ValueError('Not enough entries in palette to sort '
                             'complete rows! Got %d, expected >=%d' %
                             (num_colors, start + stride * nrows))

        for row_start in range(start, start + stride * nrows, stride):
            sublist = get_colors(row_start, row_start + stride)
            sublist.sort(key=lambda v: v[0])
            for i, entry in zip(range(row_start, row_start + stride), sublist):
                palette.set_entry_name(i, entry[1][0])
                palette.set_entry_color(i, entry[1][1])

    return palette

# FIXME: Write humanly readable help -
# See for reference: https://gitlab.gnome.org/GNOME/gimp/-/issues/4368#note_763460

# Important to describe the general effect on palettes rather than details of the sort.
help_doc = r"""
Sorts a palette, or part of a palette.
Sorts the given palette when it is editable, otherwise creates a new sorted palette.
The default is a 1D sort, but you can also sort over two color channels
or create a 2D sorted palette with sorted rows.
You can optionally install colormath (https://pypi.python.org/pypi/colormath/1.0.8)
to GIMP's Python to get even more channels to choose from.
"""

class PaletteSort (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return ["python-fu-palette-sort"]

    def do_create_procedure(self, name):
        procedure = None
        if name == "python-fu-palette-sort":
            procedure = Gimp.Procedure.new(self, name,
                                           Gimp.PDBProcType.PLUGIN,
                                           self.run, None)
            procedure.set_menu_label(_("_Sort Palette..."))
            procedure.set_documentation(
                _("Sort the colors in a palette"),
                help_doc,
                ""
            )
            procedure.set_attribution("João S. O. Bueno, Carol Spears, David Gowers",
                                      "João S. O. Bueno, Carol Spears, David Gowers",
                                      "2006-2014")
            procedure.add_menu_path ('<Palettes>/Palettes Menu')

            procedure.add_enum_argument ("run-mode", _("Run mode"),
                                         _("The run mode"), Gimp.RunMode,
                                         Gimp.RunMode.INTERACTIVE,
                                         GObject.ParamFlags.READWRITE)
            procedure.add_palette_argument ("palette", _("_Palette"),
                                            _("Palette"), True,
                                            None, True, # Default to context.
                                            GObject.ParamFlags.READWRITE)

            selection_choice = Gimp.Choice.new()
            selection_choice.add("all", 0, _("All"), "")
            selection_choice.add("slice-array", 1, _("Slice / Array"), "")
            selection_choice.add("auto-slice", 2, _("Autoslice (fg->bg)"), "")
            selection_choice.add("partitioned", 3, _("Partitioned"), "")
            procedure.add_choice_argument ("selections", _("Select_ions"), _("Selections"),
                                           selection_choice, "all", GObject.ParamFlags.READWRITE)
            # TODO: It would be much simpler to replace the slice expression with three
            # separate parameters: start-index, number-of-rows, row_length
            procedure.add_string_argument ("slice-expr", _("Slice _expression"),
                                           _(slice_expr_doc), "",
                                           GObject.ParamFlags.READWRITE)
            channel_choice = Gimp.Choice.new()
            self.add_choices (channel_choice)
            procedure.add_choice_argument ("channel1", _("Channel _to sort"), _("Channel to sort"),
                                           channel_choice, "luma", GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("ascending1", _("_Ascending"), _("Ascending"),
                                            True, GObject.ParamFlags.READWRITE)
            channel_choice2 = Gimp.Choice.new()
            self.add_choices (channel_choice2)
            procedure.add_choice_argument ("channel2", _("Secondary C_hannel to sort"),
                                           _("Secondary Channel to sort"), channel_choice2,
                                           "saturation", GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("ascending2", _("Ascen_ding"), _("Ascending"),
                                            True, GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("quantize", _("_Quantization"),
                                           _("Quantization"), 0.0, 1.0, 0.0,
                                           GObject.ParamFlags.READWRITE)
            pchannel_choice = Gimp.Choice.new()
            self.add_choices (pchannel_choice)
            procedure.add_choice_argument ("pchannel", _("Partitionin_g channel"),
                                           _("Partitioning channel"), pchannel_choice,
                                           "luma", GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("pquantize", _("Partition q_uantization"),
                                           _("Partition quantization"), 0.0, 1.0, 0.0,
                                           GObject.ParamFlags.READWRITE)

            procedure.add_palette_return_value ("new-palette", _("Palette"),
                                                _("Palette"), GObject.ParamFlags.READWRITE)

        return procedure

    def run(self, procedure, config, data):
        run_mode = config.get_property("run-mode")
        palette  = config.get_property("palette")

        if palette is None or not palette.is_valid():
            if palette is not None:
              sys.stderr.write(f'Invalid palette id: {palette.get_id()}\n')
              sys.stderr.write('This should not happen. Please report to GIMP project.\n')
              sys.stderr.write('Falling back to context palette instead.\n')
            palette = Gimp.context_get_palette()
            config.set_property("palette", palette)

        if not palette.is_valid():
            palette_name = palette.get_id()
            error = f'Invalid palette id: {palette_name}'
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                               GLib.Error(error))

        if run_mode == Gimp.RunMode.INTERACTIVE:
            GimpUi.init('python-fu-palette-sort')
            dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)

            config.set_property("palette", None)
            dialog.fill (["palette"])
            dialog.fill (["selections","slice-expr"])
            dialog.fill (["channel1", "ascending1"])
            dialog.fill (["channel2","ascending2", "quantize"])
            dialog.fill (["pchannel","pquantize"])

            if not dialog.run():
                dialog.destroy()
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())

            dialog.destroy()

        palette    = config.get_property("palette")
        selection  = config.get_property("selections")
        slice_expr = config.get_property("slice-expr")
        channel1   = config.get_choice_id("channel1")
        ascending1 = config.get_property ("ascending1")
        channel2   = config.get_choice_id("channel2")
        ascending2 = config.get_property ("ascending2")
        quantize   = config.get_property ("quantize")
        pchannel   = config.get_choice_id("pchannel")
        pquantize  = config.get_property ("pquantize")
        try:
            new_palette = palette_sort(palette, selection, slice_expr, channel1, ascending1,
                                       channel2, ascending2, quantize, pchannel, pquantize)
        except ValueError as err:
            return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                               GLib.Error(str(err)))

        return_val = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
        value = GObject.Value(Gimp.Palette, new_palette)
        return_val.remove(1)
        return_val.insert(1, value)
        return return_val

    def add_choices (self, choice):
       #TODO: Re-incorporate LAB and LCHab options with GeglColor
       choice.add("red", 0, _("Red"), "")
       choice.add("green", 1, _("Green"), "")
       choice.add("blue", 2, _("Blue"), "")
       choice.add("luma", 3, _("Luma (Y)"), "")
       choice.add("hue", 4, _("Hue"), "")
       choice.add("saturation", 5, _("Saturation"), "")
       choice.add("value", 6, _("Value"), "")
       choice.add("saturation-hsl", 7, _("Saturation (HSL)"), "")
       choice.add("lightness-hsl", 8, _("Lightness (HSL)"), "")
       choice.add("index", 9, _("Index"), "")
       choice.add("random", 10, _("Random"), "")
       choice.add("lightness-lab", 11, _("Lightness (LAB)"), "")
       choice.add("a-color", 12, _("A-color"), "")
       choice.add("b-color", 13, _("B-color"), "")
       choice.add("chroma-lchab", 14, _("Chroma (LCHab)"), "")
       choice.add("hue-lchab", 15, _("Hue (LCHab)"), "")

Gimp.main(PaletteSort.__gtype__, sys.argv)
