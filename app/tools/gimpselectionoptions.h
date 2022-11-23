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

#ifndef __LIGMA_SELECTION_OPTIONS_H__
#define __LIGMA_SELECTION_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_SELECTION_OPTIONS            (ligma_selection_options_get_type ())
#define LIGMA_SELECTION_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SELECTION_OPTIONS, LigmaSelectionOptions))
#define LIGMA_SELECTION_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SELECTION_OPTIONS, LigmaSelectionOptionsClass))
#define LIGMA_IS_SELECTION_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SELECTION_OPTIONS))
#define LIGMA_IS_SELECTION_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SELECTION_OPTIONS))
#define LIGMA_SELECTION_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SELECTION_OPTIONS, LigmaSelectionOptionsClass))


typedef struct _LigmaSelectionOptions LigmaSelectionOptions;
typedef struct _LigmaToolOptionsClass LigmaSelectionOptionsClass;

struct _LigmaSelectionOptions
{
  LigmaToolOptions  parent_instance;

  LigmaChannelOps   operation;
  gboolean         antialias;
  gboolean         feather;
  gdouble          feather_radius;

  /*  options gui  */
  GtkWidget       *mode_box;
  GtkWidget       *antialias_toggle;
};


GType       ligma_selection_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_selection_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_SELECTION_OPTIONS_H__  */
