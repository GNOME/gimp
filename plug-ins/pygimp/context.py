# *- Mode: Python; py-indent-offset: 4 -*-
# Gimp-Python - allows the writing of Gimp plugins in Python.
# Copyright (C) 2008 Lars-Peter Clausen <lars@metafoo.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

"""
This module provides functions for retriving and manipulating the status of the
current GIMP context.
"""

from gimp import pdb as _pdb

def get_background():
    """
    Returns the current background color. The background color is used in a
    variety of tools such as blending, erasing (with non-alpha images), and
    image filling.
    """
    return _pdb.gimp_context_get_background()

def get_brush():
    """
    Returns a gimp.Brush instance for the active brush. All paint operations
    and stroke operations use this brush to control the application of paint to
    the image.
    """
    from gimp import Brush
    return Brush._id2obj(_pdb.gimp_context_get_brush())

def get_font():
    """
    Returns a sting containing the name of the active font.
    """
    return _pdb.gimp_context_get_font()

def get_foreground():
    """
    Returns the current foreground color. The foregroung color is used in a
    variety of tools such as paint tools, blending, and bucket fill.
    """
    return _pdb.gimp_context_get_foreground()

def get_gradient():
    """
    Returns a gimp.Gradient instance for the active gradient.
    """
    from gimp import Gradient
    return Gradient._id2obj(_pdb.gimp_context_get_gradient())

def get_opacity():
    """
    Returns the opacity setting. The return value is a floating
    point number between 0 and 100.
    """
    return _pdb.gimp_context_get_opacity()

def get_paint_method():
    """
    Returns the name of the active paint method.
    """
    return _pdb.gimp_context_get_paint_method()

def get_paint_mode():
    """
    Returns a enum value of gimp.enums.GimpLayerModeEffects for the active paint
    mode.
    """
    from gimp.enums import GimpLayerModeEffects
    return GimpLayerModeEffects(_pdb.gimp_context_get_paint_mode())

def get_palette():
    """
    Returns a gimp.Palette instance for the active Palette.
    """
    from gimp import Palette
    return Palette._id2obj(_pdb.gimp_context_get_palette())

def get_pattern():
    """
    Returns a gimp.Pattern instance for the active Pattern.
    """
    from gimp import Pattern
    return Pattern._id2obj(_pdb.gimp_context_get_pattern())

def list_paint_methods():
    """
    Returns a tuple of available paint methods. Any of those can be used for
    'set_paint_method'.
    """
    return _pdb.gimp_context_list_paint_methods()[1]

def pop():
    """
    Removes the topmost context from the plug-in's context stack. The context
    that was active before the corresponding call to 'push' becomes the new
    current context of the plug-in.
    """
    return _pdb.gimp_context_pop()

def push():
    """
    Creates a new context by copying the current context. This copy becomes the
    new context for the calling plug-in until it is popped again using 'pop'.
    """
    return _pdb.gimp_context_push()

def set_background(color):
    """
    Sets the current background color. The background color is used in a
    variety of tools suchs as blending, erasing (with non-alpha images), and
    image filling.
    color - A valid color instance ot a 3-tuple containing RGB values.
    """
    _pdb.gimp_context_set_background(color)

def set_brush(brush):
    """
    Sets the active brush.
    brush - Either a instance of gimp.Brush or a string containing the name of
    a valid brush.
    """
    _pdb.gimp_context_set_brush(brush)

def set_default_colors():
    """
    Sets the foreground and background colors to black and white.
    """
    _pdb.gimp_context_set_default_colors()

def set_font(font):
    """
    Sets the active font.
    font - A String containing the name of a valid font.
    """
    _pdb.gimp_context_set_font(font)

def set_foreground(color):
    """
    Sets the active foreground color.
    color - A valid color instance or a 3-tuple containg RGB values.
    """
    _pdb.gimp_context_set_foreground(color)

def set_gradient(gradient):
    """
    Sets the active gradient.
    gradient - Either a instance of gimp.Gradient or a string containing the name
    of a valid gradient.
    """
    _pdb.gimp_context_set_gradient(gradient)

def set_opacity(opacity):
    """
    Changes the opacity setting.
    opacity - A floating point value between 0.0 and 100.0.
    """
    _pdb.gimp_context_set_opacity(opacity)

def set_paint_method(paint_method):
    """
    Sets active paint method.
    paint_method - The name of a valid painnt method. For valid values see
    'list_paint_methods'.
    """
    _pdb.gimp_context_set_paint_method(paint_method)

def set_paint_mode(paint_mode):
    """
    Sets active paint mode.
    paint_mode - A enum value of gimp.enums.GimpLayerModeEffects.
    """
    _pdb.gimp_context_set_paint_mode(paint_mode)

def set_palette(palette):
    """
    Sets the active palette.
    palette - Either a instance of gimp.Palette or a string containing the name
    of a valid gradient.
    """
    _pdb.gimp_context_set_palette(palette)

def set_pattern(pattern):
    """
    Sets the active pattern.
    pattern - Either a instance of gimp.Pattern or a string containing the name
    of a valid pattern.
    """
    _pdb.gimp_context_set_pattern(pattern)

def swap_colors():
    """
    Swaps the current foreground and background colors, so that the new
    foreground color becomes the old background color and vice versa.
    """
    _pdb.gimp_context_swap_colors()

