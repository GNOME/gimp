/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablefilter.h
 * Copyright (C) 2024 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_DRAWABLE_FILTER (gimp_drawable_filter_get_type ())
G_DECLARE_FINAL_TYPE (GimpDrawableFilter,
                      gimp_drawable_filter,
                      GIMP, DRAWABLE_FILTER,
                      GObject)


gint32                     gimp_drawable_filter_get_id         (GimpDrawableFilter *filter);
GimpDrawableFilter       * gimp_drawable_filter_get_by_id      (gint32              filter_id);

gboolean                   gimp_drawable_filter_is_valid       (GimpDrawableFilter *filter);

void                       gimp_drawable_filter_set_opacity    (GimpDrawableFilter *filter,
                                                                gdouble             opacity);
void                       gimp_drawable_filter_set_blend_mode (GimpDrawableFilter *filter,
                                                                GimpLayerMode       mode);
void                       gimp_drawable_filter_set_aux_input  (GimpDrawableFilter *filter,
                                                                const gchar        *input_pad_name,
                                                                GimpDrawable       *input);

GimpDrawableFilterConfig * gimp_drawable_filter_get_config     (GimpDrawableFilter *filter);

void                       gimp_drawable_filter_update         (GimpDrawableFilter *filter);

G_END_DECLS
