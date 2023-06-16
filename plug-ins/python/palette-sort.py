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

import gi
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
    (lambda v, i: v.r),
    (lambda v, i: v.g),
    (lambda v, i: v.r),

    (lambda v, i: rgb_to_yiq(v.r, v.g, v.b)[0]),

    (lambda v, i: v.to_hsv().h),
    (lambda v, i: v.to_hsv().s),
    (lambda v, i: v.to_hsv().v),

    (lambda v, i: v.to_hsl().s),
    (lambda v, i: v.to_hsl().l),

    (lambda v, i: i),
    (lambda v, i: randint(0, 0x7fffffff))
]


GRAIN_SCALE = (1.0, 1.0 , 1.0,
              1.0,
              360., 100., 100.,
              100., 100.,
              16384.,
              float(0x7ffffff),
              100., 256., 256.,
              256., 360.,)

SELECT_ALL = 0
SELECT_SLICE = 1
SELECT_AUTOSLICE = 2
SELECT_PARTITIONED = 3
SELECTIONS = (SELECT_ALL, SELECT_SLICE, SELECT_AUTOSLICE, SELECT_PARTITIONED)

try:
    from colormath.color_objects import RGBColor, LabColor, LCHabColor

    def to_lab(v):
        return RGBColor(v.r, v.g, v.b).convert_to('LAB').get_value_tuple()

    def to_lchab(v):
        return RGBColor(v.r, v.g, v.b).convert_to('LCHab').get_value_tuple()

    AVAILABLE_CHANNELS = AVAILABLE_CHANNELS + (_("Lightness (LAB)"),
                                               _("A-color"), _("B-color"),
                                               _("Chroma (LCHab)"),
                                               _("Hue (LCHab)"))
    channel_getters.extend([
        (lambda v, i: to_lab(v)[0]),
        (lambda v, i: to_lab(v)[1]),
        (lambda v, i: to_lab(v)[2]),

        (lambda v, i: to_lchab(v)[1]),
        (lambda v, i: to_lchab(v)[2])
    ])
except ImportError:
    pass


slice_expr_doc = """
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
"""


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
    if selection == SELECT_AUTOSLICE:
        def find_index(color, startindex=0):
            for i in range(startindex, num_colors):
                c = palette.entry_get_color(i)
                if c[1].r == color[1].r and c[1].g == color[1].g and c[1].b == color[1].b:
                    return i
            return None
        def hexcolor(c):
            return "#%02x%02x%02x" % (int(255 * c[1].r), int(255 * c[1].b), int(255 * c[1].g))
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
        # remaining behavior is implemented by SELECT_SLICE 'inheritance'.
        selection = SELECT_SLICE
    elif selection in (SELECT_SLICE, SELECT_PARTITIONED):
        start, nrows, length = parse_slice(slice_expr, num_colors)

    channels_getter_1 = channel_getters[channel1]
    channels_getter_2 = channel_getters[channel2]

    def get_colors(start, end):
        result = []
        for i in range(start, end):
            entry =  (palette.entry_get_name(i)[1],
                      palette.entry_get_color(i)[1])
            index1 = channels_getter_1(entry[1], i)
            index2 = channels_getter_2(entry[1], i)
            index = ((index1 - (index1 % grain1)) * (1 if ascending1 else -1),
                     (index2 - (index2 % grain2)) * (1 if ascending2 else -1)
                    )
            result.append((index, entry))
        return result

    if selection == SELECT_ALL:
        entry_list = get_colors(0, num_colors)
        entry_list.sort(key=lambda v: v[0])
        for i in range(num_colors):
            palette.entry_set_name(i, entry_list[i][1][0])
            palette.entry_set_color(i, entry_list[i][1][1])

    elif selection == SELECT_PARTITIONED:
        if num_colors < (start + length * nrows) - 1:
            raise ValueError('Not enough entries in palette to '
                             'sort complete rows! Got %d, expected >=%d' %
                             (num_colors, start + length * nrows))
        pchannels_getter = channel_getters[pchannel]
        for row in range(nrows):
            partition_spans = [1]
            rowstart = start + (row * length)
            old_color = palette.entry_get_color(rowstart)[1]
            old_partition = pchannels_getter(old_color, rowstart)
            old_partition = old_partition - (old_partition % pgrain)
            for i in range(rowstart + 1, rowstart + length):
                this_color = palette.entry_get_color(i)[1]
                this_partition = pchannels_getter(this_color, i)
                this_partition = this_partition - (this_partition % pgrain)
                if this_partition == old_partition:
                    partition_spans[-1] += 1
                else:
                    partition_spans.append(1)
                    old_partition = this_partition
            base = rowstart
            for size in partition_spans:
                palette_sort(SELECT_SLICE, '%d:1,%d' % (base, size),
                             channel1, ascending1,
                             channel2, ascending2,
                             quantize, 0, 1.0)
                base += size
    else:
        # SELECT_SLICE and SELECT_AUTOSLICE
        stride = length
        if num_colors < (start + stride * nrows) - 1:
            raise ValueError('Not enough entries in palette to sort '
                             'complete rows! Got %d, expected >=%d' %
                             (num_colors, start + stride * nrows))

        for row_start in range(start, start + stride * nrows, stride):
            sublist = get_colors(row_start, row_start + stride)
            sublist.sort(key=lambda v: v[0])
            for i, entry in zip(range(row_start, row_start + stride), sublist):
                palette.entry_set_name(i, entry[1][0])
                palette.entry_set_color(i, entry[1][1])

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

selections_option = [ _("All"), _("Slice / Array"), _("Autoslice (fg->bg)"), _("Partitioned") ]
class PaletteSort (Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "run-mode": (Gimp.RunMode,
                     _("Run mode"),
                     "The run mode",
                     Gimp.RunMode.INTERACTIVE,
                     GObject.ParamFlags.READWRITE),
        "palette": (Gimp.Palette,
                    _("_Palette"),
                    _("Palette"),
                    GObject.ParamFlags.READWRITE),
        "selections": (int,
                       _("Select_ions"),
                       str(selections_option),
                       0, 3, 0,
                       GObject.ParamFlags.READWRITE),
        # TODO: It would be much simpler to replace the slice expression with three
        # separate parameters: start-index, number-of-rows, row_length
        "slice_expr": (str,
                       _("Slice _expression"),
                       slice_expr_doc,
                       "",
                       GObject.ParamFlags.READWRITE),
        "channel1": (int,
                     _("Channel _to sort"),
                     "Channel to sort: " + str(AVAILABLE_CHANNELS),
                     0, len(AVAILABLE_CHANNELS), 3,
                     GObject.ParamFlags.READWRITE),
        "ascending1": (bool,
                       _("_Ascending"),
                       _("Ascending"),
                       True,
                       GObject.ParamFlags.READWRITE),
        "channel2": (int,
                     _("Secondary C_hannel to sort"),
                     "Secondary Channel to sort: " + str(AVAILABLE_CHANNELS),
                     0, len(AVAILABLE_CHANNELS), 5,
                     GObject.ParamFlags.READWRITE),
        "ascending2": (bool,
                       _("Ascen_ding"),
                       _("Ascending"),
                       True,
                       GObject.ParamFlags.READWRITE),
        "quantize": (float,
                     _("_Quantization"),
                     _("Quantization"),
                     0.0, 1.0, 0.0,
                     GObject.ParamFlags.READWRITE),
        "pchannel": (int,
                     _("_Partitioning channel"),
                     "Partitioning channel: " + str(AVAILABLE_CHANNELS),
                     0, len(AVAILABLE_CHANNELS), 3,
                     GObject.ParamFlags.READWRITE),
        "pquantize": (float,
                     _("Partition q_uantization"),
                     _("Partition quantization"),
                     0.0, 1.0, 0.0,
                     GObject.ParamFlags.READWRITE),

        # Returned value
        "new_palette": (Gimp.Palette,
                        _("Palette"),
                        _("Palette"),
                        GObject.ParamFlags.READWRITE),
    }

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
            procedure.add_menu_path ('<Palettes>')

            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "palette")
            procedure.add_argument_from_property(self, "selections")
            procedure.add_argument_from_property(self, "slice_expr")
            procedure.add_argument_from_property(self, "channel1")
            procedure.add_argument_from_property(self, "ascending1")
            procedure.add_argument_from_property(self, "channel2")
            procedure.add_argument_from_property(self, "ascending2")
            procedure.add_argument_from_property(self, "quantize")
            procedure.add_argument_from_property(self, "pchannel")
            procedure.add_argument_from_property(self, "pquantize")
            procedure.add_return_value_from_property(self, "new_palette")

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

            dialog.fill (["palette"])
            dialog.get_int_combo("selections", GimpUi.IntStore.new (selections_option))
            dialog.fill (["selections","slice-expr"])
            dialog.get_int_combo("channel1", GimpUi.IntStore.new (AVAILABLE_CHANNELS))
            dialog.fill (["channel1", "ascending1"])
            dialog.get_int_combo("channel2", GimpUi.IntStore.new (AVAILABLE_CHANNELS))
            dialog.fill (["channel2","ascending2", "quantize"])
            dialog.get_int_combo("pchannel", GimpUi.IntStore.new (AVAILABLE_CHANNELS))
            dialog.fill (["pchannel","pquantize"])

            if not dialog.run():
                dialog.destroy()
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())

            dialog.destroy()

        palette    = config.get_property("palette")
        selection  = config.get_property("selections")
        slice_expr = config.get_property ("slice_expr")
        channel1   = config.get_property("channel1")
        ascending1 = config.get_property ("ascending1")
        channel2   = config.get_property("channel2")
        ascending2 = config.get_property ("ascending2")
        quantize   = config.get_property ("quantize")
        pchannel   = config.get_property("pchannel")
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

Gimp.main(PaletteSort.__gtype__, sys.argv)
