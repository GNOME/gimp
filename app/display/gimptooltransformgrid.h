/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooltransformgrid.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_TRANSFORM_GRID_H__
#define __GIMP_TOOL_TRANSFORM_GRID_H__


#include "gimpcanvashandle.h"
#include "gimptoolwidget.h"


#define GIMP_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE \
  (1.5 * GIMP_CANVAS_HANDLE_SIZE_LARGE)


typedef enum
{
  GIMP_TRANSFORM_HANDLE_NONE,
  GIMP_TRANSFORM_HANDLE_NW_P,     /* north west perspective         */
  GIMP_TRANSFORM_HANDLE_NE_P,     /* north east perspective         */
  GIMP_TRANSFORM_HANDLE_SW_P,     /* south west perspective         */
  GIMP_TRANSFORM_HANDLE_SE_P,     /* south east perspective         */
  GIMP_TRANSFORM_HANDLE_NW,       /* north west                     */
  GIMP_TRANSFORM_HANDLE_NE,       /* north east                     */
  GIMP_TRANSFORM_HANDLE_SW,       /* south west                     */
  GIMP_TRANSFORM_HANDLE_SE,       /* south east                     */
  GIMP_TRANSFORM_HANDLE_N,        /* north                          */
  GIMP_TRANSFORM_HANDLE_S,        /* south                          */
  GIMP_TRANSFORM_HANDLE_E,        /* east                           */
  GIMP_TRANSFORM_HANDLE_W,        /* west                           */
  GIMP_TRANSFORM_HANDLE_CENTER,   /* center for moving              */
  GIMP_TRANSFORM_HANDLE_PIVOT,    /* pivot for rotation and scaling */
  GIMP_TRANSFORM_HANDLE_N_S,      /* north shearing                 */
  GIMP_TRANSFORM_HANDLE_S_S,      /* south shearing                 */
  GIMP_TRANSFORM_HANDLE_E_S,      /* east shearing                  */
  GIMP_TRANSFORM_HANDLE_W_S,      /* west shearing                  */
  GIMP_TRANSFORM_HANDLE_ROTATION, /* rotation                       */

  GIMP_N_TRANSFORM_HANDLES /* keep this last so *handles[] is the right size */
} GimpTransformHandle;


#define GIMP_TYPE_TOOL_TRANSFORM_GRID            (gimp_tool_transform_grid_get_type ())
#define GIMP_TOOL_TRANSFORM_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_TRANSFORM_GRID, GimpToolTransformGrid))
#define GIMP_TOOL_TRANSFORM_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_TRANSFORM_GRID, GimpToolTransformGridClass))
#define GIMP_IS_TOOL_TRANSFORM_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_TRANSFORM_GRID))
#define GIMP_IS_TOOL_TRANSFORM_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_TRANSFORM_GRID))
#define GIMP_TOOL_TRANSFORM_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_TRANSFORM_GRID, GimpToolTransformGridClass))


typedef struct _GimpToolTransformGrid        GimpToolTransformGrid;
typedef struct _GimpToolTransformGridPrivate GimpToolTransformGridPrivate;
typedef struct _GimpToolTransformGridClass   GimpToolTransformGridClass;

struct _GimpToolTransformGrid
{
  GimpToolWidget                parent_instance;

  GimpToolTransformGridPrivate *private;
};

struct _GimpToolTransformGridClass
{
  GimpToolWidgetClass  parent_class;
};


GType                 gimp_tool_transform_grid_get_type   (void) G_GNUC_CONST;

GimpToolWidget      * gimp_tool_transform_grid_new        (GimpDisplayShell      *shell,
                                                           const GimpMatrix3     *transform,
                                                           gdouble                x1,
                                                           gdouble                y1,
                                                           gdouble                x2,
                                                           gdouble                y2);

/*  protected functions  */

GimpTransformHandle   gimp_tool_transform_grid_get_handle (GimpToolTransformGrid *grid);


#endif /* __GIMP_TOOL_TRANSFORM_GRID_H__ */
