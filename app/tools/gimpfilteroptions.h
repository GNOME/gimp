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

#ifndef __LIGMA_FILTER_OPTIONS_H__
#define __LIGMA_FILTER_OPTIONS_H__


#include "ligmacoloroptions.h"


#define LIGMA_TYPE_FILTER_OPTIONS            (ligma_filter_options_get_type ())
#define LIGMA_FILTER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILTER_OPTIONS, LigmaFilterOptions))
#define LIGMA_FILTER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILTER_OPTIONS, LigmaFilterOptionsClass))
#define LIGMA_IS_FILTER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILTER_OPTIONS))
#define LIGMA_IS_FILTER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILTER_OPTIONS))
#define LIGMA_FILTER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILTER_OPTIONS, LigmaFilterOptionsClass))


typedef struct _LigmaFilterOptionsClass LigmaFilterOptionsClass;

struct _LigmaFilterOptions
{
  LigmaColorOptions   parent_instance;

  gboolean           preview;
  gboolean           preview_split;
  LigmaAlignmentType  preview_split_alignment;
  gint               preview_split_position;
  gboolean           controller;

  gboolean           blending_options_expanded;
  gboolean           color_options_expanded;
};

struct _LigmaFilterOptionsClass
{
  LigmaColorOptionsClass  parent_instance;
};


GType   ligma_filter_options_get_type                   (void) G_GNUC_CONST;

void    ligma_filter_options_switch_preview_side        (LigmaFilterOptions *options);
void    ligma_filter_options_switch_preview_orientation (LigmaFilterOptions *options,
                                                        gint               position_x,
                                                        gint               position_y);


#endif /* __LIGMA_FILTER_OPTIONS_H__ */
