# -*- Mode: Python; py-indent-offset: 3 -*-
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

"""
Initialisation file for pygimp module.
"""

from _gimp import *
from _gimp import _id2image, _id2drawable, _id2vectors, _id2display, _PyGimp_API

import enums
import context
import shelf
try:
    import ui
except:
    pass

class GimpNamedObject(object):
    """
    GimpNamedObject is a bases class for wrappers around gimp objetcs.
    """
    def __init__(self, name):
        """Initailises a new GimpNamedObject.
           name - The actuall name of the object."""
        self._name = name

    def __str__(self):
        """The representation of a GimpNamedObject is its name.
           That whay it can be passed to functions that exspect the name of a
           object."""
        return self._name

    def __repr__(self):
        return "<gimp.%s '%s'>" % (self.__class__.__name__, str(self))

    def rename(self, newname):
        """Changes the name of the object."""
        self._name = newname

    @classmethod
    def _id2obj(cls, name):
        """Creates a new object for a existing gimp object with the name
        'name'."""
        try:
            obj = object.__new__(cls)
            obj._name = name
            return obj
        except:
            return None


    name = property(fget=lambda self:self._name)

def GimpNamedObjectMeta(rename_func = None, delete_func = None,
                        duplicate_func = None):
    """
    Returns a meta class that installs common functionality for GimpNamedObject
    derivatives.
    rename_func - Name of the pdb function that is used to rename the object
    delete_func - Name of the pdb function that is used to delete the object
    duplicate_func - Name of the pdb function that is used to duplicate the object
    """
    
    class Meta(type):
        def __new__(cls, name, bases, dict):
            if rename_func:
                dict['rename'] = lambda self, newname: GimpNamedObject.rename(
                                         self, pdb[rename_func](self, newname))

            if duplicate_func != None:
                def duplicate(self, name = None):
                    dup = self._id2obj(pdb[duplicate_func](self))
                    if name != None:
                        dup.rename(name)
                    return dup
                dict['duplicate'] = duplicate

            if delete_func != None:
                def delete(self):
                    pdb[delete_func](self)
                    self._name = ""
                dict['delete'] = delete


            return type.__new__(cls, name, bases, dict)

    return Meta

class Brush(GimpNamedObject):
    """A gimp brush."""

    __metaclass__ = GimpNamedObjectMeta("gimp_brush_rename",
    "gimp_brush_delete", "gimp_brush_duplicate")

    def __init__(self):
        raise NotImplemented("Can not create Brush instance")

    def __new__(self, *args):
        obj = super(Brush, self).__new__(GeneratedBrush, *args)
        return obj
         
    @classmethod
    def _id2obj(cls, name):
        """Creates a new Brush object for a existing gimp brush with the name
        'name'. Returns either a gimp.Brush or a gimp.GeneratedBrush."""
        try:
            if pdb.gimp_brush_is_generated(name):
                brush = object.__new__(GeneratedBrush)
            else:
                brush = object.__new__(Brush)
            brush._name = name
            return brush
        except:
            return None

    width = property(fget=lambda self:pdb.gimp_brush_get_info(self)[0],
                     doc="""The brushes width in pixel.""")
    height = property(fget=lambda self:pdb.gimp_brush_get_info(self)[1],
                      doc="""The brushes height in pixel.""")
    mask_bpp = property(fget=lambda self:pdb.gimp_brush_get_info(self)[2],
                        doc="""The number of bits per pixel of the brushes mask
                             property.""")
    mask = property(fget=lambda self:pdb.gimp_brush_get_pixels(self)[4],
                    doc="""The brushes mask. A height-tuple of width-tuples""")
    pixel_bpp = property(fget=lambda self:pdb.gimp_brush_get_info(self)[3],
                        doc="""The number of bits per pixel of the brushes pixel
                             property.""")
    pixel = property(fget=lambda self:pdb.gimp_brush_get_pixels(self)[7],
                    doc="""The brushes pixel data. A height-tuple of
                           width-tuples""")
    spacing = property(fget=lambda self:pdb.gimp_brush_get_spacing(self),
                   fset=lambda self, val:pdb.gimp_brush_set_spacing(self, val),
                   doc="""The brushes spacing. Valid values are 0 <= spacing <=
                   1000.""")

    editable = property(fget=lambda self:bool(pdb.gimp_brush_is_editable(self)),
                        doc="""True if the brush is editable.""")

class GeneratedBrush(Brush):
    """A gimp generated brush."""
    def __init__(self, name, angle = None, aspect_ratio = None, hardness = None,
                 radius = None, shape = None, spikes = None):
        GimpNamedObject.__init__(self, pdb.gimp_brush_new(name))

        # Set all passed attributes
        for arg, value in locals().items():
            if arg != "self" and arg != "name" and value != None:
                setattr(self, arg, value)

    angle = property(fget=lambda self:pdb.gimp_brush_get_angle(self),
                     fset=lambda self, val:pdb.gimp_brush_set_angle(self, val),
                     doc="""The brushes rotation angle in degrees.""")
    aspect_ratio = property(
                    fget=lambda self:pdb.gimp_brush_get_aspect_ratio(self),
               fset=lambda self,val:pdb.gimp_brush_set_aspect_ratio(self, val),
               doc="""The brushes aspect ratio.""")
    hardness = property(fget=lambda self:pdb.gimp_brush_get_hardness(self),
                  fset=lambda self, val:pdb.gimp_brush_set_hardness(self, val),
                  doc="""A floating point value containing the hardness of the
                  brush.""")
    radius = property(fget=lambda self:pdb.gimp_brush_get_radius(self),
                    fset=lambda self, val:pdb.gimp_brush_set_radius(self, val),
                    doc="""A floating point value containing the radius of the
                    brush.""")
    shape = property(fget=lambda self:enums.GimpBrushGeneratedShape(pdb.gimp_brush_get_shape(self)),
                     fset=lambda self, val:pdb.gimp_brush_set_shape(self,
                     int(val)),
                     doc="""The brushes shape. Can be any of
                     gimp.enums.GimpBrushGeneratedShape""")
    spikes = property(fget=lambda self:pdb.gimp_brush_get_spikes(self),
                    fset=lambda self, val:pdb.gimp_brush_set_spikes(self, val),
                    doc="""The number of spikes for the brush.""")

class Pattern(GimpNamedObject):
    """A gimp Pattern object."""

    def __init__(self, name):
        raise NotImplementedError("Can not create gimp.Palette instances")

    width = property(fget=lambda self:pdb.gimp_pattern_get_info(self)[0],
    doc="""Width of the pattern in pixels""")
    height = property(fget=lambda self:pdb.gimp_pattern_get_info(self)[1],
    doc="""Height of the pattern in pixels""")
    pixel_bpp = property(fget=lambda self:pdb.gimp_pattern_get_info(self)[2],
    doc="""Bytes per pixel of the pixels attribute""")
    pixels = property(fget=lambda self:pdb.gimp_pattern_get_pixels(self)[4],
    doc="""A tupel containing the pixel values for the pattern. It length is
    bpp*widht*height.""")

