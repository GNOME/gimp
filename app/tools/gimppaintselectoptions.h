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


#ifndef __GIMP_PAINT_SELECT_OPTIONS_H__
#define __GIMP_PAINT_SELECT_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_TYPE_PAINT_SELECT_OPTIONS            (gimp_paint_select_options_get_type ())
#define GIMP_PAINT_SELECT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_SELECT_OPTIONS, GimpPaintSelectOptions))
#define GIMP_PAINT_SELECT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_SELECT_OPTIONS, GimpPaintSelectOptionsClass))
#define GIMP_IS_PAINT_SELECT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_SELECT_OPTIONS))
#define GIMP_IS_PAINT_SELECT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_SELECT_OPTIONS))
#define GIMP_PAINT_SELECT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_SELECT_OPTIONS, GimpPaintSelectOptionsClass))


typedef struct _GimpPaintSelectOptions      GimpPaintSelectOptions;
typedef struct _GimpPaintSelectOptionsClass GimpPaintSelectOptionsClass;

struct _GimpPaintSelectOptions
{
  GimpToolOptions        parent_instance;

  GimpPaintSelectMode    mode;
  gint                   stroke_width;
  gboolean               show_scribbles;
};

struct _GimpPaintSelectOptionsClass
{
  GimpToolOptionsClass  parent_class;
};


GType       gimp_paint_select_options_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_paint_select_options_gui            (GimpToolOptions             *tool_options);



#endif /* __GIMP_PAINT_SELECT_OPTIONS_H__ */