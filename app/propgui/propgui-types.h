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

#pragma once

#include "display/display-enums.h"

#include "widgets/widgets-types.h"


/*  enums, move to propgui-enums.h if we get more  */

typedef enum
{
  GIMP_CONTROLLER_TYPE_LINE,
  GIMP_CONTROLLER_TYPE_SLIDER_LINE,
  GIMP_CONTROLLER_TYPE_TRANSFORM_GRID,
  GIMP_CONTROLLER_TYPE_TRANSFORM_GRIDS,
  GIMP_CONTROLLER_TYPE_GYROSCOPE,
  GIMP_CONTROLLER_TYPE_FOCUS
} GimpControllerType;


/*  structs  */

typedef struct
{
  gdouble        value;           /*  slider value                           */
  gdouble        min;             /*  minimal allowable slider value         */
  gdouble        max;             /*  maximal allowable slider value         */

  gboolean       visible    : 1;  /*  slider is visible                      */
  gboolean       selectable : 1;  /*  slider is selectable                   */
  gboolean       movable    : 1;  /*  slider movable                         */
  gboolean       removable  : 1;  /*  slider is removable                    */

  gboolean       autohide   : 1;  /*  whether to autohide the slider         */
  GimpHandleType type;            /*  slider handle type                     */
  gdouble        size;            /*  slider handle size, as a fraction of   *
                                   *  the default size                       */

  gpointer       data;            /*  user data                              */
} GimpControllerSlider;

#define GIMP_CONTROLLER_SLIDER_DEFAULT                                         \
  ((const GimpControllerSlider) {                                              \
    .value      = 0.0,                                                         \
    .min        = 0.0,                                                         \
    .max        = 1.0,                                                         \
                                                                               \
    .visible    = TRUE,                                                        \
    .selectable = TRUE,                                                        \
    .movable    = TRUE,                                                        \
    .removable  = FALSE,                                                       \
                                                                               \
    .autohide   = FALSE,                                                       \
    .type       = GIMP_HANDLE_FILLED_DIAMOND,                                  \
    .size       = 1.0,                                                         \
                                                                               \
    .data       = NULL                                                         \
  })


/*  function types  */

typedef void (* GimpPickerCallback)                   (gpointer                    data,
                                                       gpointer                    identifier,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       const Babl                 *sample_format,
                                                       GeglColor                  *color);

typedef void (* GimpControllerLineCallback)           (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2);
typedef void (* GimpControllerSliderLineCallback)     (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2,
                                                       const GimpControllerSlider *sliders,
                                                       gint                        n_sliders);
typedef void (* GimpControllerTransformGridCallback)  (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const GimpMatrix3          *transform);
typedef void (* GimpControllerTransformGridsCallback) (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const GimpMatrix3          *transforms,
                                                       gint                        n_transforms);
typedef void (* GimpControllerGyroscopeCallback)      (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     yaw,
                                                       gdouble                     pitch,
                                                       gdouble                     roll,
                                                       gdouble                     zoom,
                                                       gboolean                    invert);
typedef void (* GimpControllerFocusCallback)          (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       GimpLimitType               type,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       gdouble                     radius,
                                                       gdouble                     aspect_ratio,
                                                       gdouble                     angle,
                                                       gdouble                     inner_limit,
                                                       gdouble                     midpoint);


typedef GtkWidget * (* GimpCreatePickerFunc)          (gpointer                    creator,
                                                       const gchar                *property_name,
                                                       const gchar                *icon_name,
                                                       const gchar                *tooltip,
                                                       gboolean                    pick_abyss,
                                                       GimpPickerCallback          callback,
                                                       gpointer                    callback_data);

typedef GCallback   (* GimpCreateControllerFunc)      (gpointer                    creator,
                                                       GimpControllerType          controller_type,
                                                       const gchar                *status_title,
                                                       GCallback                   callback,
                                                       gpointer                    callback_data,
                                                       gpointer                   *set_func_data);

