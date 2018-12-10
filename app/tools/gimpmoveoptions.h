/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_MOVE_OPTIONS_H__
#define __GIMP_MOVE_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_TYPE_MOVE_OPTIONS            (gimp_move_options_get_type ())
#define GIMP_MOVE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MOVE_OPTIONS, GimpMoveOptions))
#define GIMP_MOVE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MOVE_OPTIONS, GimpMoveOptionsClass))
#define GIMP_IS_MOVE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MOVE_OPTIONS))
#define GIMP_IS_MOVE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MOVE_OPTIONS))
#define GIMP_MOVE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MOVE_OPTIONS, GimpMoveOptionsClass))


typedef struct _GimpMoveOptions      GimpMoveOptions;
typedef struct _GimpToolOptionsClass GimpMoveOptionsClass;

struct _GimpMoveOptions
{
  GimpToolOptions    parent_instance;

  GimpTransformType  move_type;
  gboolean           move_current;

  /*  options gui  */
  GtkWidget         *type_box;
};


GType       gimp_move_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_move_options_gui      (GimpToolOptions *tool_options);


#endif /* __GIMP_MOVE_OPTIONS_H__ */
