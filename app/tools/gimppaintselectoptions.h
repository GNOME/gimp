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


#ifndef __LIGMA_PAINT_SELECT_OPTIONS_H__
#define __LIGMA_PAINT_SELECT_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_PAINT_SELECT_OPTIONS            (ligma_paint_select_options_get_type ())
#define LIGMA_PAINT_SELECT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINT_SELECT_OPTIONS, LigmaPaintSelectOptions))
#define LIGMA_PAINT_SELECT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINT_SELECT_OPTIONS, LigmaPaintSelectOptionsClass))
#define LIGMA_IS_PAINT_SELECT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINT_SELECT_OPTIONS))
#define LIGMA_IS_PAINT_SELECT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINT_SELECT_OPTIONS))
#define LIGMA_PAINT_SELECT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINT_SELECT_OPTIONS, LigmaPaintSelectOptionsClass))


typedef struct _LigmaPaintSelectOptions      LigmaPaintSelectOptions;
typedef struct _LigmaPaintSelectOptionsClass LigmaPaintSelectOptionsClass;

struct _LigmaPaintSelectOptions
{
  LigmaToolOptions        parent_instance;

  LigmaPaintSelectMode    mode;
  gint                   stroke_width;
  gboolean               show_scribbles;
};

struct _LigmaPaintSelectOptionsClass
{
  LigmaToolOptionsClass  parent_class;
};


GType       ligma_paint_select_options_get_type       (void) G_GNUC_CONST;

GtkWidget * ligma_paint_select_options_gui            (LigmaToolOptions             *tool_options);



#endif /* __LIGMA_PAINT_SELECT_OPTIONS_H__ */