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

#ifndef __PROPGUI_TYPES_H__
#define __PROPGUI_TYPES_H__


#include "display/display-enums.h"

#include "widgets/widgets-types.h"


/*  enums, move to propgui-enums.h if we get more  */

typedef enum
{
  LIGMA_CONTROLLER_TYPE_LINE,
  LIGMA_CONTROLLER_TYPE_SLIDER_LINE,
  LIGMA_CONTROLLER_TYPE_TRANSFORM_GRID,
  LIGMA_CONTROLLER_TYPE_TRANSFORM_GRIDS,
  LIGMA_CONTROLLER_TYPE_GYROSCOPE,
  LIGMA_CONTROLLER_TYPE_FOCUS
} LigmaControllerType;


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
  LigmaHandleType type;            /*  slider handle type                     */
  gdouble        size;            /*  slider handle size, as a fraction of   *
                                   *  the default size                       */

  gpointer       data;            /*  user data                              */
} LigmaControllerSlider;

#define LIGMA_CONTROLLER_SLIDER_DEFAULT                                         \
  ((const LigmaControllerSlider) {                                              \
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
    .type       = LIGMA_HANDLE_FILLED_DIAMOND,                                  \
    .size       = 1.0,                                                         \
                                                                               \
    .data       = NULL                                                         \
  })


/*  function types  */

typedef void (* LigmaPickerCallback)                   (gpointer                    data,
                                                       gpointer                    identifier,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       const Babl                 *sample_format,
                                                       const LigmaRGB              *color);

typedef void (* LigmaControllerLineCallback)           (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2);
typedef void (* LigmaControllerSliderLineCallback)     (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2,
                                                       const LigmaControllerSlider *sliders,
                                                       gint                        n_sliders);
typedef void (* LigmaControllerTransformGridCallback)  (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const LigmaMatrix3          *transform);
typedef void (* LigmaControllerTransformGridsCallback) (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const LigmaMatrix3          *transforms,
                                                       gint                        n_transforms);
typedef void (* LigmaControllerGyroscopeCallback)      (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     yaw,
                                                       gdouble                     pitch,
                                                       gdouble                     roll,
                                                       gdouble                     zoom,
                                                       gboolean                    invert);
typedef void (* LigmaControllerFocusCallback)          (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       LigmaLimitType               type,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       gdouble                     radius,
                                                       gdouble                     aspect_ratio,
                                                       gdouble                     angle,
                                                       gdouble                     inner_limit,
                                                       gdouble                     midpoint);


typedef GtkWidget * (* LigmaCreatePickerFunc)          (gpointer                    creator,
                                                       const gchar                *property_name,
                                                       const gchar                *icon_name,
                                                       const gchar                *tooltip,
                                                       gboolean                    pick_abyss,
                                                       LigmaPickerCallback          callback,
                                                       gpointer                    callback_data);

typedef GCallback   (* LigmaCreateControllerFunc)      (gpointer                    creator,
                                                       LigmaControllerType          controller_type,
                                                       const gchar                *status_title,
                                                       GCallback                   callback,
                                                       gpointer                    callback_data,
                                                       gpointer                   *set_func_data);


#endif /* __PROPGUI_TYPES_H__ */
