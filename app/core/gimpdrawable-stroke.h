/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-stroke.h
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


void       gimp_drawable_stroke_boundary     (GimpDrawable       *drawable,
                                              GimpStrokeOptions  *options,
                                              const GimpBoundSeg *bound_segs,
                                              gint                n_bound_segs,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gboolean            push_undo);

gboolean   gimp_drawable_stroke_path         (GimpDrawable       *drawable,
                                              GimpStrokeOptions  *options,
                                              GimpPath           *path,
                                              gboolean            push_undo,
                                              GError            **error);

void       gimp_drawable_stroke_scan_convert (GimpDrawable      *drawable,
                                              GimpStrokeOptions *options,
                                              GimpScanConvert   *scan_convert,
                                              gboolean           push_undo);
