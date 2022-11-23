/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_REGION_SELECT_OPTIONS_H__
#define __LIGMA_REGION_SELECT_OPTIONS_H__


#include "ligmaselectionoptions.h"


#define LIGMA_TYPE_REGION_SELECT_OPTIONS            (ligma_region_select_options_get_type ())
#define LIGMA_REGION_SELECT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_REGION_SELECT_OPTIONS, LigmaRegionSelectOptions))
#define LIGMA_REGION_SELECT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_REGION_SELECT_OPTIONS, LigmaRegionSelectOptionsClass))
#define LIGMA_IS_REGION_SELECT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_REGION_SELECT_OPTIONS))
#define LIGMA_IS_REGION_SELECT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_REGION_SELECT_OPTIONS))
#define LIGMA_REGION_SELECT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_REGION_SELECT_OPTIONS, LigmaRegionSelectOptionsClass))


typedef struct _LigmaRegionSelectOptions LigmaRegionSelectOptions;
typedef struct _LigmaToolOptionsClass    LigmaRegionSelectOptionsClass;

struct _LigmaRegionSelectOptions
{
  LigmaSelectionOptions  parent_instance;

  gboolean              select_transparent;
  gboolean              sample_merged;
  gboolean              diagonal_neighbors;
  gdouble               threshold;
  LigmaSelectCriterion   select_criterion;
  gboolean              draw_mask;
};


GType       ligma_region_select_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_region_select_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_REGION_SELECT_OPTIONS_H__  */
