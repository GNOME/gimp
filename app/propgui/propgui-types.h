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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPGUI_TYPES_H__
#define __PROPGUI_TYPES_H__


#include "widgets/widgets-types.h"


/*  enums, move to propgui-enums.h if we get more  */

typedef enum
{
  GIMP_CONTROLLER_TYPE_LINE,
  GIMP_CONTROLLER_TYPE_SLIDER_LINE
} GimpControllerType;


/*  structs  */

typedef struct
{
  gdouble value;
  gdouble min;
  gdouble max;
} GimpControllerSlider;


/*  function types  */

typedef void (* GimpControllerLineCallback)       (gpointer                    data,
                                                   GeglRectangle              *area,
                                                   gdouble                     x1,
                                                   gdouble                     y1,
                                                   gdouble                     x2,
                                                   gdouble                     y2);
typedef void (* GimpControllerSliderLineCallback) (gpointer                    data,
                                                   GeglRectangle              *area,
                                                   gdouble                     x1,
                                                   gdouble                     y1,
                                                   gdouble                     x2,
                                                   gdouble                     y2,
                                                   const GimpControllerSlider *sliders,
                                                   gint                        n_sliders);


typedef GtkWidget * (* GimpCreatePickerFunc)     (gpointer            creator,
                                                  const gchar        *property_name,
                                                  const gchar        *icon_name,
                                                  const gchar        *tooltip,
                                                  gboolean            pick_abyss);

typedef GCallback   (* GimpCreateControllerFunc) (gpointer            creator,
                                                  GimpControllerType  controller_type,
                                                  GCallback           callback,
                                                  gpointer            callback_data,
                                                  gpointer           *set_func_data);


#endif /* __PROPGUI_TYPES_H__ */
