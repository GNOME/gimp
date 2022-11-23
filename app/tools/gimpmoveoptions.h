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

#ifndef __LIGMA_MOVE_OPTIONS_H__
#define __LIGMA_MOVE_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_MOVE_OPTIONS            (ligma_move_options_get_type ())
#define LIGMA_MOVE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MOVE_OPTIONS, LigmaMoveOptions))
#define LIGMA_MOVE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MOVE_OPTIONS, LigmaMoveOptionsClass))
#define LIGMA_IS_MOVE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MOVE_OPTIONS))
#define LIGMA_IS_MOVE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MOVE_OPTIONS))
#define LIGMA_MOVE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MOVE_OPTIONS, LigmaMoveOptionsClass))


typedef struct _LigmaMoveOptions      LigmaMoveOptions;
typedef struct _LigmaToolOptionsClass LigmaMoveOptionsClass;

struct _LigmaMoveOptions
{
  LigmaToolOptions    parent_instance;

  LigmaTransformType  move_type;
  gboolean           move_current;

  /*  options gui  */
  GtkWidget         *type_box;
};


GType       ligma_move_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_move_options_gui      (LigmaToolOptions *tool_options);


#endif /* __LIGMA_MOVE_OPTIONS_H__ */
