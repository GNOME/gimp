/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_DRAWABLE_FILTER_H__
#define __GIMP_DRAWABLE_FILTER_H__


#include "gimpfilter.h"


#define GIMP_TYPE_DRAWABLE_FILTER            (gimp_drawable_filter_get_type ())
#define GIMP_DRAWABLE_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE_FILTER, GimpDrawableFilter))
#define GIMP_DRAWABLE_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE_FILTER, GimpDrawableFilterClass))
#define GIMP_IS_DRAWABLE_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE_FILTER))
#define GIMP_IS_DRAWABLE_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE_FILTER))
#define GIMP_DRAWABLE_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE_FILTER, GimpDrawableFilterClass))


typedef struct _GimpDrawableFilterClass  GimpDrawableFilterClass;

struct _GimpDrawableFilterClass
{
  GimpFilterClass  parent_class;

  void (* flush) (GimpDrawableFilter *filter);
};


/*  Drawable Filter functions  */

/*  Successive apply() functions can be called, but eventually MUST be
 *  followed with an commit() or an abort() call, both of which will
 *  remove the live filter from the drawable.
 */

GType      gimp_drawable_filter_get_type       (void) G_GNUC_CONST;

GimpDrawableFilter *
           gimp_drawable_filter_new            (GimpDrawable        *drawable,
                                                const gchar         *undo_desc,
                                                GeglNode            *operation,
                                                const gchar         *icon_name);

GimpDrawable *
           gimp_drawable_filter_get_drawable   (GimpDrawableFilter  *filter);
GeglNode * gimp_drawable_filter_get_operation  (GimpDrawableFilter  *filter);

void       gimp_drawable_filter_set_clip       (GimpDrawableFilter  *filter,
                                                gboolean             clip);
void       gimp_drawable_filter_set_region     (GimpDrawableFilter  *filter,
                                                GimpFilterRegion     region);
void       gimp_drawable_filter_set_crop       (GimpDrawableFilter  *filter,
                                                const GeglRectangle *rect,
                                                gboolean             update);
void       gimp_drawable_filter_set_preview    (GimpDrawableFilter  *filter,
                                                gboolean             enabled);
void       gimp_drawable_filter_set_preview_split
                                               (GimpDrawableFilter  *filter,
                                                gboolean             enabled,
                                                GimpAlignmentType    alignment,
                                                gdouble              split_position);
void       gimp_drawable_filter_set_opacity    (GimpDrawableFilter  *filter,
                                                gdouble              opacity);
void       gimp_drawable_filter_set_mode       (GimpDrawableFilter  *filter,
                                                GimpLayerMode        paint_mode,
                                                GimpLayerColorSpace  blend_space,
                                                GimpLayerColorSpace  composite_space,
                                                GimpLayerCompositeMode composite_mode);
void       gimp_drawable_filter_set_add_alpha  (GimpDrawableFilter  *filter,
                                                gboolean             add_alpha);

void       gimp_drawable_filter_set_color_managed
                                               (GimpDrawableFilter  *filter,
                                                gboolean             managed);
void       gimp_drawable_filter_set_gamma_hack (GimpDrawableFilter  *filter,
                                                gboolean             gamma_hack);

void       gimp_drawable_filter_set_override_constraints
                                               (GimpDrawableFilter  *filter,
                                                gboolean             override_constraints);

const Babl *
           gimp_drawable_filter_get_format     (GimpDrawableFilter  *filter);

void       gimp_drawable_filter_apply          (GimpDrawableFilter  *filter,
                                                const GeglRectangle *area);

gboolean   gimp_drawable_filter_commit         (GimpDrawableFilter  *filter,
                                                GimpProgress        *progress,
                                                gboolean             cancellable);
void       gimp_drawable_filter_abort          (GimpDrawableFilter  *filter);


#endif /* __GIMP_DRAWABLE_FILTER_H__ */
