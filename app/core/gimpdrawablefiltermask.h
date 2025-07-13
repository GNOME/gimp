/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablefiltermask.h
 * Copyright (C) 2024 Jehan
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

#include "gimpchannel.h"


#define GIMP_TYPE_DRAWABLE_FILTER_MASK            (gimp_drawable_filter_mask_get_type ())
#define GIMP_DRAWABLE_FILTER_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE_FILTER_MASK, GimpDrawableFilterMask))
#define GIMP_DRAWABLE_FILTER_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE_FILTER_MASK, GimpDrawableFilterMaskClass))
#define GIMP_IS_DRAWABLE_FILTER_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE_FILTER_MASK))
#define GIMP_IS_DRAWABLE_FILTER_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE_FILTER_MASK))
#define GIMP_DRAWABLE_FILTER_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE_FILTER_MASK, GimpDrawableFilterMaskClass))


typedef struct _GimpDrawableFilterMaskClass GimpDrawableFilterMaskClass;

struct _GimpDrawableFilterMask
{
  GimpChannel         parent_instance;

  GimpDrawableFilter *filter;
};

struct _GimpDrawableFilterMaskClass
{
  GimpChannelClass parent_class;
};


GType                    gimp_drawable_filter_mask_get_type  (void) G_GNUC_CONST;

GimpDrawableFilterMask * gimp_drawable_filter_mask_new        (GimpImage              *image,
                                                               gint                    width,
                                                               gint                    height);
void                     gimp_drawable_filter_mask_set_filter (GimpDrawableFilterMask *mask,
                                                               GimpDrawableFilter     *filter);
GimpDrawableFilter     * gimp_drawable_filter_mask_get_filter (GimpDrawableFilterMask *mask);
