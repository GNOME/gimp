/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmastrokeoptions.h
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

#ifndef __LIGMA_STROKE_OPTIONS_H__
#define __LIGMA_STROKE_OPTIONS_H__


#include "ligmafilloptions.h"


#define LIGMA_TYPE_STROKE_OPTIONS            (ligma_stroke_options_get_type ())
#define LIGMA_STROKE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_STROKE_OPTIONS, LigmaStrokeOptions))
#define LIGMA_STROKE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_STROKE_OPTIONS, LigmaStrokeOptionsClass))
#define LIGMA_IS_STROKE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_STROKE_OPTIONS))
#define LIGMA_IS_STROKE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_STROKE_OPTIONS))
#define LIGMA_STROKE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_STROKE_OPTIONS, LigmaStrokeOptionsClass))


typedef struct _LigmaStrokeOptionsClass LigmaStrokeOptionsClass;

struct _LigmaStrokeOptions
{
  LigmaFillOptions  parent_instance;
};

struct _LigmaStrokeOptionsClass
{
  LigmaFillOptionsClass  parent_class;

  void (* dash_info_changed) (LigmaStrokeOptions *stroke_options,
                              LigmaDashPreset     preset);
};


GType               ligma_stroke_options_get_type             (void) G_GNUC_CONST;

LigmaStrokeOptions * ligma_stroke_options_new                  (Ligma              *ligma,
                                                              LigmaContext       *context,
                                                              gboolean           use_context_color);

LigmaStrokeMethod    ligma_stroke_options_get_method           (LigmaStrokeOptions *options);

gdouble             ligma_stroke_options_get_width            (LigmaStrokeOptions *options);
LigmaUnit            ligma_stroke_options_get_unit             (LigmaStrokeOptions *options);
LigmaCapStyle        ligma_stroke_options_get_cap_style        (LigmaStrokeOptions *options);
LigmaJoinStyle       ligma_stroke_options_get_join_style       (LigmaStrokeOptions *options);
gdouble             ligma_stroke_options_get_miter_limit      (LigmaStrokeOptions *options);
gdouble             ligma_stroke_options_get_dash_offset      (LigmaStrokeOptions *options);
GArray            * ligma_stroke_options_get_dash_info        (LigmaStrokeOptions *options);

LigmaPaintOptions  * ligma_stroke_options_get_paint_options    (LigmaStrokeOptions *options);
gboolean            ligma_stroke_options_get_emulate_dynamics (LigmaStrokeOptions *options);

void                ligma_stroke_options_take_dash_pattern    (LigmaStrokeOptions *options,
                                                              LigmaDashPreset     preset,
                                                              GArray            *pattern);

void                ligma_stroke_options_prepare              (LigmaStrokeOptions *options,
                                                              LigmaContext       *context,
                                                              LigmaPaintOptions  *paint_options);
void                ligma_stroke_options_finish               (LigmaStrokeOptions *options);


#endif /* __LIGMA_STROKE_OPTIONS_H__ */
