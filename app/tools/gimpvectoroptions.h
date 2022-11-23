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

#ifndef __LIGMA_VECTOR_OPTIONS_H__
#define __LIGMA_VECTOR_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_VECTOR_OPTIONS            (ligma_vector_options_get_type ())
#define LIGMA_VECTOR_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VECTOR_OPTIONS, LigmaVectorOptions))
#define LIGMA_VECTOR_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VECTOR_OPTIONS, LigmaVectorOptionsClass))
#define LIGMA_IS_VECTOR_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VECTOR_OPTIONS))
#define LIGMA_IS_VECTOR_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VECTOR_OPTIONS))
#define LIGMA_VECTOR_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VECTOR_OPTIONS, LigmaVectorOptionsClass))


typedef struct _LigmaVectorOptions    LigmaVectorOptions;
typedef struct _LigmaToolOptionsClass LigmaVectorOptionsClass;

struct _LigmaVectorOptions
{
  LigmaToolOptions  parent_instance;

  LigmaVectorMode   edit_mode;
  gboolean         polygonal;

  /*  options gui  */
  GtkWidget       *to_selection_button;
  GtkWidget       *fill_button;
  GtkWidget       *stroke_button;
};


GType       ligma_vector_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_vector_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_VECTOR_OPTIONS_H__  */
