/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-filters.h
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


/*  internal functions  */

void            _gimp_drawable_filters_init         (GimpDrawable *drawable);
void            _gimp_drawable_filters_finalize     (GimpDrawable *drawable);


/*  public functions  */

GimpContainer * gimp_drawable_get_filters           (GimpDrawable *drawable);

gboolean        gimp_drawable_has_visible_filters   (GimpDrawable *drawable);

gboolean        gimp_drawable_n_editable_filters    (GimpDrawable *drawable,
                                                     gint         *n_editable,
                                                     gint         *first,
                                                     gint         *last);

void            gimp_drawable_add_filter            (GimpDrawable *drawable,
                                                     GimpFilter   *filter);
void            gimp_drawable_remove_filter         (GimpDrawable *drawable,
                                                     GimpFilter   *filter);
void            gimp_drawable_clear_filters         (GimpDrawable *drawable);

gboolean        gimp_drawable_has_filter            (GimpDrawable *drawable,
                                                     GimpFilter   *filter);

gboolean        gimp_drawable_raise_filter          (GimpDrawable *drawable,
                                                     GimpFilter   *filter);
gboolean        gimp_drawable_lower_filter          (GimpDrawable *drawable,
                                                     GimpFilter   *filter);

void            gimp_drawable_merge_filters         (GimpDrawable *drawable);
gboolean        gimp_drawable_merge_filter          (GimpDrawable *drawable,
                                                     GimpFilter   *filter,
                                                     GimpProgress *progress,
                                                     const gchar  *undo_desc,
                                                     const Babl   *format,
                                                     gboolean      clip,
                                                     gboolean      cancellable,
                                                     gboolean      update);
