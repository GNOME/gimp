/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_DRAWABLE_FILTER_H__
#define __LIGMA_DRAWABLE_FILTER_H__


#include "ligmafilter.h"


#define LIGMA_TYPE_DRAWABLE_FILTER            (ligma_drawable_filter_get_type ())
#define LIGMA_DRAWABLE_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DRAWABLE_FILTER, LigmaDrawableFilter))
#define LIGMA_DRAWABLE_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DRAWABLE_FILTER, LigmaDrawableFilterClass))
#define LIGMA_IS_DRAWABLE_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DRAWABLE_FILTER))
#define LIGMA_IS_DRAWABLE_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DRAWABLE_FILTER))
#define LIGMA_DRAWABLE_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DRAWABLE_FILTER, LigmaDrawableFilterClass))


typedef struct _LigmaDrawableFilterClass  LigmaDrawableFilterClass;

struct _LigmaDrawableFilterClass
{
  LigmaFilterClass  parent_class;

  void (* flush) (LigmaDrawableFilter *filter);
};


/*  Drawable Filter functions  */

/*  Successive apply() functions can be called, but eventually MUST be
 *  followed with an commit() or an abort() call, both of which will
 *  remove the live filter from the drawable.
 */

GType      ligma_drawable_filter_get_type       (void) G_GNUC_CONST;

LigmaDrawableFilter *
           ligma_drawable_filter_new            (LigmaDrawable        *drawable,
                                                const gchar         *undo_desc,
                                                GeglNode            *operation,
                                                const gchar         *icon_name);

LigmaDrawable *
           ligma_drawable_filter_get_drawable   (LigmaDrawableFilter  *filter);
GeglNode * ligma_drawable_filter_get_operation  (LigmaDrawableFilter  *filter);

void       ligma_drawable_filter_set_clip       (LigmaDrawableFilter  *filter,
                                                gboolean             clip);
void       ligma_drawable_filter_set_region     (LigmaDrawableFilter  *filter,
                                                LigmaFilterRegion     region);
void       ligma_drawable_filter_set_crop       (LigmaDrawableFilter  *filter,
                                                const GeglRectangle *rect,
                                                gboolean             update);
void       ligma_drawable_filter_set_preview    (LigmaDrawableFilter  *filter,
                                                gboolean             enabled);
void       ligma_drawable_filter_set_preview_split
                                               (LigmaDrawableFilter  *filter,
                                                gboolean             enabled,
                                                LigmaAlignmentType    alignment,
                                                gint                 split_position);
void       ligma_drawable_filter_set_opacity    (LigmaDrawableFilter  *filter,
                                                gdouble              opacity);
void       ligma_drawable_filter_set_mode       (LigmaDrawableFilter  *filter,
                                                LigmaLayerMode        paint_mode,
                                                LigmaLayerColorSpace  blend_space,
                                                LigmaLayerColorSpace  composite_space,
                                                LigmaLayerCompositeMode composite_mode);
void       ligma_drawable_filter_set_add_alpha  (LigmaDrawableFilter  *filter,
                                                gboolean             add_alpha);

void       ligma_drawable_filter_set_gamma_hack (LigmaDrawableFilter  *filter,
                                                gboolean             gamma_hack);

void       ligma_drawable_filter_set_override_constraints
                                               (LigmaDrawableFilter  *filter,
                                                gboolean             override_constraints);

const Babl *
           ligma_drawable_filter_get_format     (LigmaDrawableFilter  *filter);

void       ligma_drawable_filter_apply          (LigmaDrawableFilter  *filter,
                                                const GeglRectangle *area);

gboolean   ligma_drawable_filter_commit         (LigmaDrawableFilter  *filter,
                                                LigmaProgress        *progress,
                                                gboolean             cancellable);
void       ligma_drawable_filter_abort          (LigmaDrawableFilter  *filter);


#endif /* __LIGMA_DRAWABLE_FILTER_H__ */
