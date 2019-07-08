#!/usr/bin/env python2
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

from gimpfu import *
# little known, colorsys is part of Python's stdlib
from colorsys import rgb_to_yiq
from textwrap import dedent
from random import randint

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

AVAILABLE_CHANNELS = (_("Red"), _("Green"), _("Blue"),
                      _("Luma (Y)"),
                      _("Hue"), _("Saturation"), _("Value"),
                      _("Saturation (HSL)"), _("Lightness (HSL)"),
                      _("Index"),
                      _("Random"))

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


def noop(v, i):
    return v


def to_hsv(v, i):
    return v.to_hsv()


def to_hsl(v, i):
    return v.to_hsl()


def to_yiq(v, i):
    return rgb_to_yiq(*v[:-1])


def to_index(v, i):
    return (i,)

def to_random(v, i):
    return (randint(0, 0x7fffffff),)


channel_getters = [ (noop, 0), (noop, 1), (noop, 2),
                    (to_yiq, 0),
                    (to_hsv, 0), (to_hsv, 1), (to_hsv, 2),
                    (to_hsl, 1), (to_hsl, 2),
                    (to_index, 0),
                    (to_random, 0)]


try:
    from colormath.color_objects import RGBColor, LabColor, LCHabColor
    AVAILABLE_CHANNELS = AVAILABLE_CHANNELS + (_("Lightness (LAB)"),
                                               _("A-color"), _("B-color"),
                                               _("Chroma (LCHab)"),
                                               _("Hue (LCHab)"))
    to_lab = lambda v,i: RGBColor(*v[:-1]).convert_to('LAB').get_value_tuple()
    to_lchab = (lambda v,i:
                    RGBColor(*v[:-1]).convert_to('LCHab').get_value_tuple())
    channel_getters.extend([(to_lab, 0), (to_lab, 1), (to_lab, 2),
                            (to_lchab, 1), (to_lchab, 2)])
except ImportError:
    pass


def parse_slice(s, numcolors):
    """Parse a slice spec and return (start, nrows, length)
    All items are optional. Omitting them makes the largest possible selection that
    exactly fits the other items.

    start:nrows,length


    '' selects all items, as does ':'
    ':4,' makes a 4-row selection out of all colors (length auto-determined)
    ':4' also.
    ':1,4' selects the first 4 colors
    ':,4' selects rows of 4 colors (nrows auto-determined)
    ':4,4' selects 4 rows of 4 colors
    '4:' selects a single row of all colors after 4, inclusive.
    '4:,4' selects rows of 4 colors, starting at 4 (nrows auto-determined)
    '4:4,4' selects 4 rows of 4 colors (16 colors total), beginning at index 4.
    '4' is illegal (ambiguous)


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
        return 0, 1, numcolors # entire palette, one row
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

    #If palette is read only, work on a copy:
    editable = pdb.gimp_palette_is_editable(palette)
    if not editable:
        palette = pdb.gimp_palette_duplicate (palette)

    num_colors = pdb.gimp_palette_get_info (palette)

    start, nrows, length = None, None, None
    if selection == SELECT_AUTOSLICE:
        def find_index(color, startindex=0):
            for i in range(startindex, num_colors):
                c = pdb.gimp_palette_entry_get_color (palette, i)
                if c == color:
                    return i
            return None
        def hexcolor(c):
            return "#%02x%02x%02x" % tuple(c[:-1])
        fg = pdb.gimp_context_get_foreground()
        bg = pdb.gimp_context_get_background()
        start = find_index(fg)
        end = find_index(bg)
        if start is None:
            raise ValueError("Couldn't find foreground color %r in palette" % list(fg))
        if end is None:
            raise ValueError("Couldn't find background color %r in palette" % list(bg))
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
            length /= nrows
        except ValueError:
            # bad expression is okay here, just assume one row
            nrows = 1
        # remaining behaviour is implemented by SELECT_SLICE 'inheritance'.
        selection= SELECT_SLICE
    elif selection in (SELECT_SLICE, SELECT_PARTITIONED):
        start, nrows, length = parse_slice(slice_expr, num_colors)

    channels_getter_1, channel_index = channel_getters[channel1]
    channels_getter_2, channel2_index = channel_getters[channel2]

    def get_colors(start, end):
        result = []
        for i in range(start, end):
            entry =  (pdb.gimp_palette_entry_get_name (palette, i),
                      pdb.gimp_palette_entry_get_color (palette, i))
            index1 = channels_getter_1(entry[1], i)[channel_index]
            index2 = channels_getter_2(entry[1], i)[channel2_index]
            index = ((index1 - (index1 % grain1)) * (1 if ascending1 else -1),
                     (index2 - (index2 % grain2)) * (1 if ascending2 else -1)
                    )
            result.append((index, entry))
        return result

    if selection == SELECT_ALL:
        entry_list = get_colors(0, num_colors)
        entry_list.sort(key=lambda v:v[0])
        for i in range(num_colors):
            pdb.gimp_palette_entry_set_name (palette, i, entry_list[i][1][0])
            pdb.gimp_palette_entry_set_color (palette, i, entry_list[i][1][1])

    elif selection == SELECT_PARTITIONED:
        if num_colors < (start + length * nrows) - 1:
            raise ValueError('Not enough entries in palette to '
                             'sort complete rows! Got %d, expected >=%d' %
                             (num_colors, start + length * nrows))
        pchannels_getter, pchannel_index = channel_getters[pchannel]
        for row in range(nrows):
            partition_spans = [1]
            rowstart = start + (row * length)
            old_color = pdb.gimp_palette_entry_get_color (palette,
                                                          rowstart)
            old_partition = pchannels_getter(old_color, rowstart)[pchannel_index]
            old_partition = old_partition - (old_partition % pgrain)
            for i in range(rowstart + 1, rowstart + length):
                this_color = pdb.gimp_palette_entry_get_color (palette, i)
                this_partition = pchannels_getter(this_color, i)[pchannel_index]
                this_partition = this_partition - (this_partition % pgrain)
                if this_partition == old_partition:
                    partition_spans[-1] += 1
                else:
                    partition_spans.append(1)
                    old_partition = this_partition
            base = rowstart
            for size in partition_spans:
                palette_sort(palette, SELECT_SLICE, '%d:1,%d' % (base, size),
                             channel, quantize, ascending, 0, 1.0)
                base += size
    else:
        stride = length
        if num_colors < (start + stride * nrows) - 1:
            raise ValueError('Not enough entries in palette to sort '
                             'complete rows! Got %d, expected >=%d' %
                             (num_colors, start + stride * nrows))

        for row_start in range(start, start + stride * nrows, stride):
            sublist = get_colors(row_start, row_start + stride)
            sublist.sort(key=lambda v:v[0], reverse=not ascending)
            for i, entry in zip(range(row_start, row_start + stride), sublist):
                pdb.gimp_palette_entry_set_name (palette, i, entry[1][0])
                pdb.gimp_palette_entry_set_color (palette, i, entry[1][1])

    return palette

register(
    "python-fu-palette-sort",
    N_("Sort the colors in a palette"),
    # FIXME: Write humanly readable help -
    # (I can't figure out what the plugin does, or how to use the parameters after
    # David's enhancements even looking at the code -
    # let alone someone just using GIMP (JS) )
    dedent("""\
    palette_sort (palette, selection, slice_expr, channel,
    channel2, quantize, ascending, pchannel, pquantize) -> new_palette
    Sorts a palette, or part of a palette, using several options.
    One can select two color channels over which to sort,
    and several auxiliary parameters create a 2D sorted
    palette with sorted rows, among other things.
    One can optionally install colormath
    (https://pypi.python.org/pypi/colormath/1.0.8)
    to GIMP's Python to get even more channels to choose from.
    """),
    "João S. O. Bueno, Carol Spears, David Gowers",
    "João S. O. Bueno, Carol Spears, David Gowers",
    "2006-2014",
    N_("_Sort Palette..."),
    "",
    [
        (PF_PALETTE, "palette",  _("Palette"), ""),
        (PF_OPTION, "selections", _("Se_lections"), SELECT_ALL,
                    (_("All"), _("Slice / Array"), _("Autoslice (fg->bg)"),
                     _("Partitioned"))),
        (PF_STRING,    "slice-expr", _("Slice _expression"), ''),
        (PF_OPTION,   "channel1",    _("Channel to _sort"), 3,
                                    AVAILABLE_CHANNELS),
        (PF_BOOL,  "ascending1", _("_Ascending"), True),
        (PF_OPTION,   "channel2",    _("Secondary Channel to s_ort"), 5,
                                    AVAILABLE_CHANNELS),
        (PF_BOOL,   "ascending2", _("_Ascending"), True),
        (PF_FLOAT,  "quantize", _("_Quantization"), 0.0),
        (PF_OPTION,   "pchannel",    _("_Partitioning channel"), 3,
                                            AVAILABLE_CHANNELS),
        (PF_FLOAT,  "pquantize", _("Partition q_uantization"), 0.0),
    ],
    [],
    palette_sort,
    menu="<Palettes>",
    domain=("gimp20-python", gimp.locale_directory)
    )

main ()
