#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
gradient_from_index_range GIMP plug-in
    by Sergio Jiménez Herena <sergio.jimenez.herena at gmail.com>, 2018

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""

from gimpfu import *

__author__ = 'Sergio Jiménez Herena'
__copyright__ = '2018, Sergio Jiménez Herena'
__license__ = 'GPL3'

gettext.install('gimp20-python', gimp.locale_directory, unicode=True)

NORMAL = 0  # Smoothed transitions between colors
CYCLIC = 1  # Same as smooth, but the first and last colors are the same.
DISCRETE = 2  # Hard transitions between colors

NON = 0  # None
LIG = 1  # Luminance
VAL = 2  # Value
HUE = 3  # Hue
SAT = 4  # Saturation
RED = 5  # Red
GRE = 6  # Green
BLU = 7  # Blue

grad_str = {
    NORMAL: _('Normal'),
    CYCLIC: _('Cyclic'),
    DISCRETE: _('Discrete')
}

invert_string = {
    False: '',
    True: _(', inverted ')
}

sort_str = {
    NON: '',
    LIG: _(', ordered by lightness'),
    VAL: _(', ordered by value'),
    HUE: _(', ordered by hue'),
    SAT: _(', ordered by saturation'),
    RED: _(', ordered by red'),
    GRE: _(', ordered by green'),
    BLU: _(', ordered by blue')
}


def clamp(n, min_val, max_val):
    return max(min_val, min(n, max_val))


def gradient_from_palette(palette, first_index, last_index,
                          grad_type, order, invert):
    pal_color_num, pal_colors = pdb.gimp_palette_get_colors(palette)

    if not pal_color_num:
        pdb.gimp_message(_("The palette is empty."))
        return

    first_index = int(first_index)
    last_index = int(last_index)

    if first_index > last_index:
        first_index, last_index = last_index, first_index

    first_index = clamp(first_index, 0, pal_color_num - 1)
    last_index = clamp(last_index, 0, pal_color_num - 1)

    if first_index == last_index:
        pdb.gimp_message(_("The gradient must contain at least two colors."))
        return

    grad_colors = []

    for pal_index in range(first_index, last_index + 1):
        grad_colors.append(pal_colors[pal_index])

    if order:
        if order == LIG:
            grad_colors.sort(key=lambda c: c.luminance())
        elif order == VAL:
            grad_colors.sort(key=lambda c: (c.to_hsv()).v)
        elif order == HUE:
            grad_colors.sort(key=lambda c: (c.to_hsl()).h)
        elif order == SAT:
            grad_colors.sort(key=lambda c: (c.to_hsl()).s)
        elif order == RED:
            grad_colors.sort(key=lambda c: c.r)
        elif order == GRE:
            grad_colors.sort(key=lambda c: c.g)
        elif order == BLU:
            grad_colors.sort(key=lambda c: c.b)

    if invert:
        grad_colors = list(reversed(grad_colors))

    grad_name = '{} - {}{}{} - {}-{}'.format(palette,
                                             grad_str[grad_type],
                                             invert_string[invert],
                                             sort_str[order],
                                             str(first_index).zfill(3),
                                             str(last_index).zfill(3)
                                             )

    new_gradient = pdb.gimp_gradient_new(grad_name)

    grad_segments = last_index - first_index

    if grad_type == CYCLIC or grad_type == DISCRETE:
        grad_segments += 1

    if grad_segments > 1:
        pdb.gimp_gradient_segment_range_split_uniform(new_gradient,
                                                      0, -1, grad_segments)

    for segment in range(0, grad_segments):

        if grad_type == DISCRETE:
            color_l = color_r = grad_colors[segment]
        else:
            color_l = grad_colors[segment]
            if grad_type == CYCLIC and segment == grad_segments - 1:
                color_r = grad_colors[0]
            else:
                color_r = grad_colors[segment + 1]

        pdb.gimp_gradient_segment_set_left_color(
            new_gradient,
            segment,
            color_l,
            100.0
        )

        pdb.gimp_gradient_segment_set_right_color(
            new_gradient,
            segment,
            color_r,
            100.0
        )

    pdb.gimp_context_set_gradient(new_gradient)
    pdb.gimp_gradients_refresh()

    return new_gradient


register(
    'gradient_from_palette',
    N_('Generate a new gradient using colors from a palette.'),
    'This plugin will generate a new gradient using the colors in a range '
    'between two palette indexes.',
    'Sergio Jiménez Herena',
    'Sergio Jiménez Herena',
    '2018',
    N_('Gradient from palette...'),
    '',
    [
        (PF_PALETTE, 'palette', 'Context palette', None),
        (PF_SPINNER, 'first_index', _('_First index'), 0, (0, 1023, 1)),
        (PF_SPINNER, 'last_index', _('_Last index'), 255, (1, 1024, 1)),
        (PF_OPTION, 'grad_type', _('Gradient _type'), NORMAL,
         (
             _('Normal'),
             _('Cyclic'),
             _('Discrete')
         )
         ),
        (PF_OPTION, 'order', _('_Sort colors\t\t'), NON,
         (
             _('Do not sort'),
             _('Lightness (HSL)'),
             _('Value (HSV)'),
             _('Hue (HSL)'),
             _('Saturation (HSL)'),
             _('Red (RGB)'),
             _('Green (RGB)'),
             _('Blue (RGB)')
         )
         ),
        (PF_BOOL, 'invert', _('_Invert'), False)
    ],
    [(PF_GRADIENT, 'new_gradient', 'result')],
    gradient_from_palette,
    menu='<Palettes>',
    domain=('gimp20-python', gimp.locale_directory)
)

main()
