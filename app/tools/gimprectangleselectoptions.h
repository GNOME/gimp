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

#ifndef __LIGMA_RECTANGLE_SELECT_OPTIONS_H__
#define __LIGMA_RECTANGLE_SELECT_OPTIONS_H__


#include "ligmaselectionoptions.h"


#define LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS            (ligma_rectangle_select_options_get_type ())
#define LIGMA_RECTANGLE_SELECT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS, LigmaRectangleSelectOptions))
#define LIGMA_RECTANGLE_SELECT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS, LigmaRectangleSelectOptionsClass))
#define LIGMA_IS_RECTANGLE_SELECT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS))
#define LIGMA_IS_RECTANGLE_SELECT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS))
#define LIGMA_RECTANGLE_SELECT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_RECTANGLE_SELECT_OPTIONS, LigmaRectangleSelectOptionsClass))


typedef struct _LigmaRectangleSelectOptions LigmaRectangleSelectOptions;
typedef struct _LigmaToolOptionsClass       LigmaRectangleSelectOptionsClass;

struct _LigmaRectangleSelectOptions
{
  LigmaSelectionOptions  parent_instance;

  gboolean              round_corners;
  gdouble               corner_radius;
};


GType       ligma_rectangle_select_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_rectangle_select_options_gui      (LigmaToolOptions *tool_options);


#endif /* __LIGMA_RECTANGLE_SELECT_OPTIONS_H__ */
