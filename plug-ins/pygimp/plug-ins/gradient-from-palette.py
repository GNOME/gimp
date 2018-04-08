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

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

__author__    = 'Sergio Jiménez Herena'
__copyright__ = '2018, Sergio Jiménez Herena'
__license__   = 'GPL3'

GRAD_TYPE_NORMAL   = 0
GRAD_TYPE_CYCLIC   = 1
GRAD_TYPE_DISCRETE = 2


def clamp(n, min_val, max_val):
    return max(min_val, min(n, max_val))


def palette_range_to_gradient(palette, first_index, last_index,
                              new_grad_type, sort, invert):

    type_format_string = (_('Normal'), _('Cyclic'), _('Discrete'))

    inverted_format_string = ('', _(', inverted'))

    sort_options = (
        {'format_string': '',
         'function': None},
        {'format_string': _(', ordered by luminance '),
         'function': (lambda c: c.luminance())},
        {'format_string': _(', ordered by lightness (HSL) '),
         'function': (lambda c: c.to_hsl().l)},
        {'format_string': _(', ordered by value (HSV) '),
         'function': (lambda c: (c.to_hsv()).v)},
        {'format_string': _(', ordered by hue '),
         'function': (lambda c: (c.to_hsl()).h)},
        {'format_string': _(', ordered by saturation'),
         'function': (lambda c: (c.to_hsl()).s)},
        {'format_string': _(', ordered by red component '),
         'function': (lambda c: c.r)},
        {'format_string': _(', ordered by green component '),
         'function': (lambda c: c.g)},
        {'format_string': _(', ordered by blue component '),
         'function': (lambda c: c.b)}
    )

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

    new_grad_colors = [pal_colors[color] for color in
                       range(first_index, last_index + 1)]

    if sort:
        new_grad_colors.sort(key=sort_options[sort]['function'])

    if invert:
        new_grad_colors = list(reversed(new_grad_colors))

    new_grad_name = '{} ({}-{}) - {}{}{}'.format(
        palette,
        str(first_index),
        str(last_index),
        type_format_string[new_grad_type],
        sort_options[sort]['format_string'],
        inverted_format_string[invert]
    )

    new_grad = pdb.gimp_gradient_new(new_grad_name)

    new_grad_segments = last_index - first_index

    if new_grad_type == GRAD_TYPE_CYCLIC or \
       new_grad_type == GRAD_TYPE_DISCRETE:
        new_grad_segments += 1

    if new_grad_segments > 1:
        pdb.gimp_gradient_segment_range_split_uniform(new_grad,
                                                      0, -1,
                                                      new_grad_segments)

    for segment in range(0, new_grad_segments):

        if new_grad_type == GRAD_TYPE_DISCRETE:
            left_color = right_color = new_grad_colors[segment]
        else:
            left_color = new_grad_colors[segment]
            if new_grad_type == GRAD_TYPE_CYCLIC and \
               segment == new_grad_segments - 1:
                right_color = new_grad_colors[0]
            else:
                right_color = new_grad_colors[segment + 1]

        pdb.gimp_gradient_segment_set_left_color(
            new_grad,
            segment,
            left_color,
            100.0
        )

        pdb.gimp_gradient_segment_set_right_color(
            new_grad,
            segment,
            right_color,
            100.0
        )

    pdb.gimp_context_set_gradient(new_grad)
    pdb.gimp_gradients_refresh()

    return new_grad


def gradient_from_palette(palette):

    palette_range_to_gradient(palette,
                              0,
                              pdb.gimp_palette_get_colors(palette)[0],
                              GRAD_TYPE_NORMAL,
                              0,
                              False)


def gradient_from_palette_repeated(palette):

    palette_range_to_gradient(palette,
                              0,
                              pdb.gimp_palette_get_colors(palette)[0],
                              GRAD_TYPE_CYCLIC,
                              0,
                              False)


register(
    'python-fu-palette-range-to-gradient',
    N_('Create a gradient using a range of the palette colors.'),
    'Create a new gradient using the colors in the range '
    'between two palette indexes.\n\n'
    'grad_type {NORMAL(0), CYCLIC(1), DISCRETE(2)}\n\n'
    'sort {DO_NOT_SORT(0), BY_LUMINANCE(1), BY_LIGHTNESS_HSL(2), '
    'BY_VALUE_HSV(3), BY_HUE(4), BY_SATURATION(5), '
    'BY_RED_COMPONENT(6), BY_GREEN_COMPONENT(7), BY_BLUE_COMPONENT(8)}\n\n'
    'invert {FALSE(0), TRUE(1)}',
    'Sergio Jiménez Herena, inspired by previous works by Carol Spears, '
    'Adrian Likins and Jeff Trefftz',
    'Sergio Jiménez Herena',
    '2018',
    N_('Palette range to gradient...'),
    '',
    [
        (PF_PALETTE, 'palette', 'Context palette', None),
        (PF_SPINNER, 'first_index', _('_First index'), 0, (0, 1024, 1)),
        (PF_SPINNER, 'last_index', _('_Last index'), 255, (0, 1024, 1)),
        (PF_OPTION,  'grad_type', _('_Type'), 0,
         (
             _('Normal'),
             _('Cyclic'),
             _('Discrete')
         )
         ),
        (PF_OPTION, 'sort', _('_Sort colors'), 0,
         (
             _('Do not sort'),
             _('Luminance'),
             _('Lightness (HSL)'),
             _('Value (HSV)'),
             _('Hue'),
             _('Saturation'),
             _('Red component'),
             _('Green component'),
             _('Blue component')
         )
         ),
        (PF_BOOL, 'invert', '_Invert', False)
    ],
    [(PF_GRADIENT, 'new_gradient', 'result')],
    palette_range_to_gradient,
    menu='<Palettes>',
    domain=('gimp20-python', gimp.locale_directory)
)

register(
    'python-fu-palette-to-gradient',
    N_("Create a gradient using colors from the palette"),
    'Create a new repeating gradient using colors from the palette.',
    'Sergio Jiménez Herena, inspired by previous works by Carol Spears, '
    'Adrian Likins and Jeff Trefftz',
    'Sergio Jiménez Herena',
    '2018',
    N_("Palette to _Gradient"),
    '',
    [(PF_PALETTE, 'palette', 'Context palette', None)],
    [(PF_GRADIENT, 'new_gradient', 'result')],
    gradient_from_palette,
    menu='<Palettes>',
    domain=('gimp20-python', gimp.locale_directory)
)

register(
    'python-fu-palette-to-gradient-repeating',
    N_("Create a repeating gradient using colors from the palette"),
    'Create a new repeating gradient using colors from the palette.',
    'Sergio Jiménez Herena, inspired by previous works by Carol Spears, '
    'Adrian Likins and Jeff Trefftz',
    'Sergio Jiménez Herena',
    '2018',
    N_("Palette to _Repeating Gradient"),
    '',
    [(PF_PALETTE, 'palette', 'Context palette', None)],
    [(PF_GRADIENT, 'new_gradient', 'result')],
    gradient_from_palette_repeated,
    menu='<Palettes>',
    domain=('gimp20-python', gimp.locale_directory)
)

main()
