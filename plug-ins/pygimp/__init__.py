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


def _prop(func):
    """Helper function for creating properties"""
    return property(doc=func.__doc__, *func())

class GimpNamedObject(object):
    """
    GimpNamedObject is a bases class for wrappers around gimp objetcs.
    """
    def __init__(self, name):
        """Initailises a new GimpNamedObject.
           name - The actuall name of the object."""
        self._name = name

    def __str__(self):
        """The string representation of a GimpNamedObject is its name.
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

class Palette(GimpNamedObject):
    """A gimp Palette object. A Palette instance provides a list like interface
       to access the palette entries.
       Caution using gimp.pdb.gimp_palette_* functions paralell to gimp.Palette
       instances can cause data inconsistency."""

    __metaclass__ = GimpNamedObjectMeta("gimp_palette_rename",
    "gimp_palette_delete", "gimp_palette_duplicate")

    def __init__(self, name):
        """Creates a new Palette
            name - The name for the palette"""
        GimpNamedObject.__init__(self, pdb.gimp_palette_new(name))

        # Dictionary of PaletteEntry instances that are bound to this Palette.
        # The key is the index of the entry.
        self._bound_entries = {}

    def __len__(self):
        """Returns the number of entries in the palette"""
        return pdb.gimp_palette_get_info(self)

    def __getitem__(self, key):
        if isinstance(key, int):
            if key < 0:
                key += self.__len__()
            if key < 0 or key >= self.__len__():
                raise IndexError("index out of range")

            if self._bound_entries.has_key(key):
                return self._bound_entries[key]

            entry = PaletteEntry(pdb.gimp_palette_entry_get_name(self,key),
                                 pdb.gimp_palette_entry_get_color(self, key))

            # Bind entry to palette
            entry._bound_to.append((self, key))
            self._bound_entries[key] = entry

            return entry

        raise TypeError

    def __setitem__(self, key, value):
        if isinstance(key, int):
            if key < 0:
                key += self.__len__()
            if key < 0 or key >= self.__len__():
                raise IndexError("index out of range")

            if isinstance(value, PaletteEntry):
                if self._bound_entries.has_key(key):
                    self._bound_entries[key]._bound_to.remove((self, key))
                
                # Bind entry to palette
                value._bound_to.append((self, key))
                self._bound_entries[key] = value

                pdb.gimp_palette_entry_set_name(self, key, value.name)
                pdb.gimp_palette_entry_set_color(self, key, value.color)
            else:
                pdb.gimp_palette_entry_set_color(self, key, value)
        else:
            raise TypeError

    def __delitem__(self, key):
        """Removes palette entry with index 'key' from the palette"""
        if isinstance(self, key):
            if key < 0:
                key += self.__len__()
            if key < 0 or key >= self.__len__():
                raise IndexError("index out of range")

            pdb.gimp_palette_delete_entry(self, key)
            if self._bound_entries.has_key(key):
                self._bound_entries[key]._bound_to.remove((self, key))
                del self._bound_entries[key]

                # Decrease index of all folowing entries
                for index, entry in self._bound_entries.items():
                    if index > key:
                        entry._bound_to.remove((self, index))
                        entry._bound_to.append((self, index-1))

                        del self._bound_entries[index]
                        self._bound_entries[index-1] = entry
        else:
            raise TypeError


    def append(self, entry):
        """Appends a entry to the palette.
           entry - A PaletteEntry instance."""
        pdb.gimp_palette_add_entry(self, entry.name, entry.color)
        index = self.__len__() - 1
        entry._bound_to.append((self, index))
        self._bound_entries[index] = entry

    editable = property(fget=lambda self:bool(pdb.gimp_palette_is_editable(self)),
                        doc="""True if the palette is editable.""")
    columns = property(fget=lambda self:pdb.gimp_palette_get_columns(self),
                       fset=lambda self, val:pdb.gimp_palette_set_columns(self, val),
                       doc="""Number of columes to use when the palette is
                       being displayed.""")
    @classmethod
    def _id2obj(cls, name):
        obj = super(Palette, cls)._id2obj(name)
        obj._bound_entries = {}
        return obj

class PaletteEntry(object):
    """A palette entry object."""
    def __init__(self, name, color):
        """Creates a new palette entry.
           name - Name of the entry
           color - Color of the entry"""
        self.__name = name
        self.__color = color

        # List of tuples to what Palettes a entry is bound. (palette, entry
        # index)
        self._bound_to = []

    def __repr__(self):
        return "<gimp.PaletteEntry '%s - (%d, %d, %d)'>" % self.__name,
        self.__color[0], self.__color[1], self.__color[2]

    def __set_name(self, name):
        self.__name = name
        for gradient, index in self._bound_to:
            pdb.gimp_palette_entry_set_name(gradient, index, name)

    def __set_color(self, color):
        self.__color = color
        for gradient, index in self._bound_to:
            pdb.gimp_palette_entry_set_color(gradient, index, color)

    name = property(fget=lambda self:self.__name, fset=__set_name,
                    doc="""Name of the entry.""")
    color = property(fget=lambda self:self.__color, fset=__set_color,
                     doc="""Color of the entry.""")

class Gradient(GimpNamedObject):
    """A gimp Gradient object."""

    __metaclass__ = GimpNamedObjectMeta("gimp_gradient_rename",
                                        "gimp_gradient_delete",
                                        "gimp_gradient_duplicate")

    def __init__(self, name):
        """Creates a new Gradient.
          name - The name for the gradient"""
        GimpNamedObject.__init__(self, pdb.gimp_gradient_new(name))


    def get_custom_samples(self, positions, reverse=False):
        """Samples the gradient in custom positions.
           Returns a list of samples as gimp.color.RGB
           positions - A list of positions to sample along the gradient.
           reverse - Whether to use the reverse gradient or not."""

        samples = pdb.gimp_gradient_get_custom_samples(self, len(positions),
                                                    positions, reverse)[1]
        return map(color.RGB, *(iter(samples),)*4)

    def get_uniform_samples(self, num_samples, reverse=False):
        """Samples the specfied uniform parts.
           Returns a list of samples as gimp.color.RGB.
           num_samples - Number of samples to return. 2 <= num_samples.
           reverse - Whether to use the reverse gradient or not."""
           
        samples = pdb.gimp_gradient_get_uniform_samples(self, num_samples,
                                                        reverse)[1]
        return map(color.RGB,*(iter(samples),)*4)

    segments = property(
        fget=lambda self: GradientSegmentRange(self, 0, 
                               pdb.gimp_gradient_get_number_of_segments(self)),
        fdel=lambda self:pdb.gimp_gradient_segment_range_delete(self, 0, -1),
        doc="""A gimp.GradientSegmentRange used to access the segemnts of the
               gradient.""")
    editable = property(
        fget=lambda self: pdb.gimp_gradient_is_editable(self) == 1,
        doc="""True if the segment is editable, otherwise False.""")

class GradientSegment(object):

    def __init__(self, gradient, index):
        self.gradient = gradient
        self.index = index

    def __repr__(self):
        return "<gimp.GradientSegment (%s, %d)>" % (str(self.gradient),
                                                   self.index)
    @_prop
    def blending_function():
        """The segments blending function.
           A enum value of gimp.enums.GimpGradientSegmentType."""
        def get(self):
            return enums.GimpGradientSegmentType(
                   pdb.gimp_gradient_segment_get_blending_function(
                                                    self.gradient, self.index))
        def set(self, value):
            pdb.gimp_gradient_segment_range_set_blending_function(self.gradient,
                                            self.index, self.index, value)
        return (get,set)
    
    @_prop
    def coloring_type():
        """The segments coloring type.
           A enum value of gimp.enums.GimpGradientSegmentColor"""
        def get(self):
            return enums.GimpGradientSegmentColor(
                   pdb.gimp_gradient_segment_get_coloring_type(self.gradient,
                                                               self.index))
        def set(self, value):
            pdb.gimp_gradient_segment_range_set_coloring_type(self.gradient,
                                                 self.index, self.index, value)
        return (get,set)
    
    def get_left_color(self):
        return pdb.gimp_gradient_segment_get_left_color(self.gradient,
                                                        self.index)[0]
    def set_left_color(self, color):
        # Get opacity from color
        if hasattr(color, "a"):
           opacity = color.a
        elif len(color) < 4:
            opacity = 100
        elif isinstance(color[3], float):
            opacity = int(color[3] * 100)
        else:
            opacity = int(int(color[3]) / 2.55)
        opacity = min(max(opacity, 0), 100)
        pdb.gimp_gradient_segment_set_left_color(self.gradient, self.index,
                                                 color, opacity)
    def get_right_color(self):
        return pdb.gimp_gradient_segment_get_right_color(self.gradient,
                                                        self.index)[0]
    def set_right_color(self, color):
        # Get opacity from color
        if hasattr(color, "a"):
           opacity = color.a
        elif len(color) < 4:
            opacity = 100
        elif isinstance(color[3], float):
            opacity = int(color[3] * 100)
        else:
            opacity = int(int(color[3]) / 2.55)
        opacity = min(max(opacity, 0), 100)
        pdb.gimp_gradient_segment_set_right_color(self.gradient, self.index,
                                                  color, opacity)
    @_prop
    def left_pos():
        """The left endpoint position of the segment."""
        def get(self):
            return pdb.gimp_gradient_segment_get_left_pos(self.gradient,
                                                          self.index)
        def set(self, value):
            pdb.gimp_gradient_segment_set_left_pos(self.gradient, self.index,
                                                   min(max(value), 0.0, 1.0))
        return (get, set)

    @_prop
    def middle_pos():
        """The middle point position of the segment."""
        def get(self):
            return pdb.gimp_gradient_segment_get_middle_pos(self.gradient,
                                                            self.index)
        def set(self, value):
            pdb.gimp_gradient_segment_set_middle_pos(self.gradient, self.index,
                                                     min(max(value, 0.0), 1.0))
        return (get, set)

    @_prop
    def right_pos():
        """The right endpoint position of the segment."""
        def get(self):
            return pdb.gimp_gradient_segment_get_right_pos(self.gradient,
                                                           self.index)
        def set(self, value):
            pdb.gimp_gradient_segment_set_right_pos(self.gradient, self.index,
                                                    min(max(value, 0.0), 1.0))
        return (get, set)


class GradientSegmentRange(object):
    
    def __init__(self, gradient, start, end):
        self.gradient = gradient
        self.start = start
        self.end = end

    def __repr__(self):
        return "<gimp.GradientSegmentRange (%s, %d, %d)>"% (str(self.gradient),
                                                          self.start, self.end)
    
    def __len__(self):
        return self.end - self.start

    def __getitem__(self, key):
        if isinstance(key, slice):
            start, end, step = key.indices(len(self))
            if step != 1:
                raise IndexError
            if start > end:
                start = end
            return GradientSegmentRange(self.gradient, start+self.start,
                                        end+self.start)
        elif isinstance(key, int):
            if key < 0:
                key += len(self)
            if key < 0 or key >= len(self):
                raise IndexError("index out of range")

            return GradientSegment(self.gradient, self.start + key)
        else:
            raise TypeError

    def __delitem__(self, key):
        if isinstance(key, slice):
            start, end, step = key.indices(len(self))
            if step != 1:
                raise IndexError
            if start > end:
                start = end
        elif isinstance(key, int):
            if key < 0:
                key += len(self)
            if key < 0 or key >= len(self):
                raise IndexError("index out of range")
            start = key
            end = start + 1
        else:
            raise TypeError

        pdb.gimp_gradient_segment_range_delete(self.gradient,
                                      self.start + start, self.start + end - 1)

    def blend_colors(self):
        """Blends the colors (but not the opacity) of the segment range. Using
           it, the colors' transition will be uniform across the range."""
        return pdb.gimp_gradient_segment_range_blend_colors(self.gradient,
                                                          self.start, self.end)

    def blend_opacity(self):
        """Blends the opacity (but not the colors) of the segment range. Using
           it, the opacity's transition will be uniform across the range."""
        return pdb.gimp_gradient_segment_range_blend_opacity(self.gradient,
                                                          self.start, self.end)

    def flip(self):
        """Flips the segment range."""
        return pdb.gimp_gradient_segment_range_flip(self.gradient, self.start,
                                                    self.end)

    def move(self, offset, compress_neighbors = True):
        """Move the position of an entire segment range.
           offset - The offset to move the segment range (-1 <= delta <= 1)
           compress_neighbores - Whether or not to compress the neighboring
           segments."""
        return pdb.gimp_gradient_segment_range_move(self.gradient, self.start,
                                          self.end, offset, compress_neighbors)

    def redistribute_handles(self):
        """Uniformly redistribute the segment range's handles."""
        return pdb.gimp_gradient_segment_range_redistribute_handles(
                                           self.gradient, self.start, self.end)

    def replicate(self, num_replications):
        """Replicates the segment range.
           num_replications - The number of replications (2 <= num_replications
           <= 20)"""
        return pdb.gimp_gradient_segment_range_replicate(self.gradient,
                                        self.start, self.end, num_replications)

    def set_blending_function(self, blending_function):
        """Sets the blending function for each segment in the segment range.
           blending_function - A enum value of 
           gimp.enums.GimpGradientSegmentType."""
        return pdb.gimp_gradient_segment_range_set_blending_function(
                        self.gradient, self.start, self.end, blending_function)

    def set_coloring_type(self, coloring_type):
        """Sets the coloring type for each segment in the segment range.
           coloring_type - A enum value of gimp.enums.GimpGradientSegmentColor.
           """
        return pdb.gimp_gradient_segment_range_set_coloring_type(self.gradient,
                                           self.start, self.end, coloring_type)

    def split_midpoint(self):
        """Splits each segment in the segment range at the segment midpoint."""
        return pdb.gimp_gradient_segment_range_split_midpoint(self.gradient,
                                                          self.start, self.end)

    def split_uniform(self, num_splits = 1):
        """Splits each segment in the segment range uniformly.
           num_splits - Number of splits per segment."""
        return pdb.gimp_gradient_segment_range_split_uniform(self.gradient,
                                              self.start, self.end, num_splits)


