/* The LIGMA -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmafilloptions.h
 * Copyright (C) 2003 Simon Budig
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

#ifndef __LIGMA_FILL_OPTIONS_H__
#define __LIGMA_FILL_OPTIONS_H__


#include "ligmacontext.h"


#define LIGMA_TYPE_FILL_OPTIONS            (ligma_fill_options_get_type ())
#define LIGMA_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILL_OPTIONS, LigmaFillOptions))
#define LIGMA_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILL_OPTIONS, LigmaFillOptionsClass))
#define LIGMA_IS_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILL_OPTIONS))
#define LIGMA_IS_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILL_OPTIONS))
#define LIGMA_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILL_OPTIONS, LigmaFillOptionsClass))


typedef struct _LigmaFillOptionsClass LigmaFillOptionsClass;

struct _LigmaFillOptions
{
  LigmaContext  parent_instance;
};

struct _LigmaFillOptionsClass
{
  LigmaContextClass  parent_class;
};


GType             ligma_fill_options_get_type         (void) G_GNUC_CONST;

LigmaFillOptions * ligma_fill_options_new              (Ligma                *ligma,
                                                      LigmaContext         *context,
                                                      gboolean             use_context_color);

LigmaFillStyle     ligma_fill_options_get_style        (LigmaFillOptions     *options);
void              ligma_fill_options_set_style        (LigmaFillOptions     *options,
                                                      LigmaFillStyle        style);

gboolean          ligma_fill_options_get_antialias    (LigmaFillOptions     *options);
void              ligma_fill_options_set_antialias    (LigmaFillOptions     *options,
                                                      gboolean             antialias);

gboolean          ligma_fill_options_get_feather      (LigmaFillOptions     *options,
                                                      gdouble             *radius);
void              ligma_fill_options_set_feather      (LigmaFillOptions     *options,
                                                      gboolean             feather,
                                                      gdouble              radius);

gboolean          ligma_fill_options_set_by_fill_type (LigmaFillOptions     *options,
                                                      LigmaContext         *context,
                                                      LigmaFillType         fill_type,
                                                      GError             **error);
gboolean          ligma_fill_options_set_by_fill_mode (LigmaFillOptions     *options,
                                                      LigmaContext         *context,
                                                      LigmaBucketFillMode   fill_mode,
                                                      GError             **error);

const gchar     * ligma_fill_options_get_undo_desc    (LigmaFillOptions     *options);

const Babl      * ligma_fill_options_get_format       (LigmaFillOptions     *options,
                                                      LigmaDrawable        *drawable);

GeglBuffer      * ligma_fill_options_create_buffer    (LigmaFillOptions     *options,
                                                      LigmaDrawable        *drawable,
                                                      const GeglRectangle *rect,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);
void              ligma_fill_options_fill_buffer      (LigmaFillOptions     *options,
                                                      LigmaDrawable        *drawable,
                                                      GeglBuffer          *buffer,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);


#endif /* __LIGMA_FILL_OPTIONS_H__ */
