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

"""This module provides functions for painting and selecting regions on an image
   or drawable."""

import gimp

# Helper functions

def _active_image():
    """Returns the active image as gimp.Image or None."""
    il = gimp.image_list()
    if(len(il) > 0):
        return il[0]
    return None

def _active_drawable():
    """Returns the active drawable of the active image as gimp.Drawable or 
       None."""
    active_image = _active_image()
    if active_image:
        return active_image.active_drawable
    return None

# Paint tools

def brush(drawable, first_point, *args):
    if drawable == None:
        drawable = _active_drawable()
    points = [first_point] + list(args)
    points = sum(map(list, points), [])
    gimp.pdb.gimp_brush(drawable, len(points), points)
    
def clone(drawable, strokes, src_drawable = None, src_x = 0, src_y = 0,
          clone_type = gimp.enums.IMAGE_CLONE):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    if not src_drawable:
        src_drawable = drawable

    gimp.pdb.gimp_clone(drawable, src_drawable, clone_type, src_x, src_y,
                        len(strokes), strokes)

def convolve(drawable, strokes, pressure = 50,
             convolve_type = gimp.enums.CONVOLVE_BLUR):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    gimp.pdb.gimp_convolve(drawable, pressure, convolve_type, len(strokes), strokes)

def dodgeburn(drawable, strokes, exposure = 50, type = gimp.enums.DODGE,
              mode = gimp.enums.SHADOWS):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    gimp.pdb.gimp_dodgeburn(drawable, exposure, type, mode, len(strokes),
                            strokes)

def eraser(drawable, strokes, soft = False, method = gimp.enums.PAINT_CONSTANT):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    gimp.pdb.gimp_eraser(drawable, len(strokes), strokes, soft, method)

def paintbrush(drawable, strokes, fade_out = 0, 
               method = gimp.enums.PAINT_CONSTANT, gradient_length = 0):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    gimp.pdb.gimp_paintbrush(drawable, fade_out, len(strokes), strokes, method,
                             gradient_length)

def pencil(drawable, first_point, *args):
    if drawable == None:
        drawable = _active_drawable()
    points = [first_point] + list(args)
    points = sum(map(list, points), [])
    gimp.pdb.gimp_pencil(drawable, len(points), points)

def smudge(drawable, strokes, pressure = 50):
    if drawable == None:
        drawable = _active_drawable()
    strokes = sum(map(list, strokes), [])
    gimp.pdb.gimp_smudge(drawable, pressure, len(strokes), strokes)

# Edit tools

def blend(drawable, blend_mode = gimp.enums.FG_BG_RGB_MODE, 
          paint_mode = gimp.enums.NORMAL_MODE,
          gradient_type = gimp.enums.GRADIENT_LINEAR,
          opacity = 100, offset = 0, repeat_mode = gimp.enums.REPEAT_NONE,
          reverse = False, supersample = False, max_supersampling_depth = 1,
          supersampling_threshold = 1, dither = False, x1 = 0, y1 = 0, x2 = 0,
          y2 = 0):
    if drawable == None:
        drawable = _active_drawable()
    gimp.pdb.gimp_edit_blend(drawable, blend_mode, paint_mode, gradient_type,
                             opacity, offset, repeat_mode, reverse, supersample,
                             max_supersampling_depth, supersampling_threshold,
                             dither, x1, y1, x2, y2)

def fill(drawable, fill_mode = gimp.enums.FOREGROUND_FILL):
    if drawable == None:
        drawable = _active_drawable()
    gimp.pdb.gimp_edit_fill(drawable, fill_mode)

def bucket_fill(drawable, fill_mode = gimp.enums.FG_BUCKET_FILL,
                paint_mode = gimp.enums.NORMAL_MODE, opacity = 100,
                threshold = 0, sample_merged = False, fill_transparent = False,
                select_criterion = gimp.enums.SELECT_CRITERION_COMPOSITE,
                x = 0, y = 0):
    if drawable == None:
        drawable = _active_drawable()
    gimp.pdb.gimp_edit_bucket_fill_full(drawable, fill_mode, paint_mode,
                                        opacity, threshold, sample_merged,
                                        fill_transparent, select_criterion,
                                        x, y)

def stroke_selection(drawable):
    if drawable == None:
        drawable = _active_drawable()
    gimp.pdb.gimp_edit_stroke(drawable)
    
def stroke_vectors(drawable, vectors):
    if drawable == None:
        drawable = _active_drawable()
    gimp.pdb.gimp_edit_stroke_vectors(drawable, vectors)

# Select tools

def _feather_helper(feather_radius):
    if feather_radius == None:
        return False, 0.0
    return True, feather_radius

def _radius_helper(radius):
    if isinstance(radius, tuple):
        return radius
    return radius, radius

def select_by_color(drawable, color, threshold = 0,
                    operation = gimp.enums.CHANNEL_OP_REPLACE,
                    antialias = False, feather_radius = 0,
                    sample_merged = False, select_transparent = False,
                    select_criterion = gimp.enums.SELECT_CRITERION_COMPOSITE):
    if drawable == None:
        drawable = _active_drawable()
    do_feather, feather_radius = _feather_helper(feater_radius)
    feather_radius_x, feather_radius_y = _radius_helper(feather_radius)
    
    gimp.pdb.gimp_by_color_select(drawable, color, threshold, operation,
                                  antialias, do_feather, feather_radius_x,
                                  feather_radius_y, sample_merged,
                                  select_transparent, select_criterion)

def select_elipse(image, x, y, width, height,
                  operation = gimp.enums.CHANNEL_OP_REPLACE,
                  feather_radius = None):
    do_feather, feather_radius = _feather_helper(feater_radius)
    
    gimp.pdb.gimp_elipse_select(image, x, y, width, height, operation,
                                do_feather, feather_radius)

def select_free(image, points, operation = gimp.enums.CHANNEL_OP_REPLACE,
                antialias = False, feather_radius = None):
    do_feather, feather_radius = _feather_helper(feater_radius)
    points = sum(map(list, points), [])
   
    gimp.pdb.free_select(image, points, len(points), operation, antialias,
                         do_feather, feather_radius)

def select_fuzzy(drawable, x, y, threshold = 0,
                 operation = gimp.enums.CHANNEL_OP_REPLACE, antialias = False,
                 feather_radius = None, sample_merged = False,
                 select_transparent = False,
                 select_criterion = gimp.enums.SELECT_CRITERION_COMPOSITE):
    if drawable == None:
        drawable = _active_drawable()
    do_feather, feather_radius = _feather_helper(feater_radius)
    
    gimp.pdb.fuzzy_select(image, x, y, threshold, antialias, do_feather,
                          feather_radius, sample_merged, select_transparent,
                          select_criterion)

def select_rect(image, x, y, width, height,
                operation = gimp.enums.CHANNEL_OP_REPLACE,
                feather_radius = None):
    do_feather, feather_radius = _feather_helper(feater_radius)
    
    gimp.pdb.gimp_rect_select(image, x, y, width, height, operation,
                              do_feather, feather_radius)

def select_round_rect(image, x, y, width, height, corner_radius,
                      operation = gimp.enums.CHANNEL_OP_REPLACE,
                      antialias = False, feather_radius = None):
    do_feather, feather_radius = _feather_helper(feather_radius)
    feather_radius_x, feather_radius_y = _radius_helper(feather_radius)
    corner_radius_x, corner_radius_y = _radius_helper(corner_radius)

    gimp.pdb.gimp_round_rect_select(image, x, y, width, height, corner_radius_x,
                                    corner_radius_y, operation, antialias,
                                    do_feather, feather_radius_x,
                                    feather_radius_y)


    
