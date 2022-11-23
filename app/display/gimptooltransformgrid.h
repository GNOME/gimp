/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooltransformgrid.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_TRANSFORM_GRID_H__
#define __LIGMA_TOOL_TRANSFORM_GRID_H__


#include "ligmacanvashandle.h"
#include "ligmatoolwidget.h"


#define LIGMA_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE \
  (1.5 * LIGMA_CANVAS_HANDLE_SIZE_LARGE)


typedef enum
{
  LIGMA_TRANSFORM_HANDLE_NONE,
  LIGMA_TRANSFORM_HANDLE_NW_P,     /* north west perspective         */
  LIGMA_TRANSFORM_HANDLE_NE_P,     /* north east perspective         */
  LIGMA_TRANSFORM_HANDLE_SW_P,     /* south west perspective         */
  LIGMA_TRANSFORM_HANDLE_SE_P,     /* south east perspective         */
  LIGMA_TRANSFORM_HANDLE_NW,       /* north west                     */
  LIGMA_TRANSFORM_HANDLE_NE,       /* north east                     */
  LIGMA_TRANSFORM_HANDLE_SW,       /* south west                     */
  LIGMA_TRANSFORM_HANDLE_SE,       /* south east                     */
  LIGMA_TRANSFORM_HANDLE_N,        /* north                          */
  LIGMA_TRANSFORM_HANDLE_S,        /* south                          */
  LIGMA_TRANSFORM_HANDLE_E,        /* east                           */
  LIGMA_TRANSFORM_HANDLE_W,        /* west                           */
  LIGMA_TRANSFORM_HANDLE_CENTER,   /* center for moving              */
  LIGMA_TRANSFORM_HANDLE_PIVOT,    /* pivot for rotation and scaling */
  LIGMA_TRANSFORM_HANDLE_N_S,      /* north shearing                 */
  LIGMA_TRANSFORM_HANDLE_S_S,      /* south shearing                 */
  LIGMA_TRANSFORM_HANDLE_E_S,      /* east shearing                  */
  LIGMA_TRANSFORM_HANDLE_W_S,      /* west shearing                  */
  LIGMA_TRANSFORM_HANDLE_ROTATION, /* rotation                       */

  LIGMA_N_TRANSFORM_HANDLES /* keep this last so *handles[] is the right size */
} LigmaTransformHandle;


#define LIGMA_TYPE_TOOL_TRANSFORM_GRID            (ligma_tool_transform_grid_get_type ())
#define LIGMA_TOOL_TRANSFORM_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_TRANSFORM_GRID, LigmaToolTransformGrid))
#define LIGMA_TOOL_TRANSFORM_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_TRANSFORM_GRID, LigmaToolTransformGridClass))
#define LIGMA_IS_TOOL_TRANSFORM_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_TRANSFORM_GRID))
#define LIGMA_IS_TOOL_TRANSFORM_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_TRANSFORM_GRID))
#define LIGMA_TOOL_TRANSFORM_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_TRANSFORM_GRID, LigmaToolTransformGridClass))


typedef struct _LigmaToolTransformGrid        LigmaToolTransformGrid;
typedef struct _LigmaToolTransformGridPrivate LigmaToolTransformGridPrivate;
typedef struct _LigmaToolTransformGridClass   LigmaToolTransformGridClass;

struct _LigmaToolTransformGrid
{
  LigmaToolWidget                parent_instance;

  LigmaToolTransformGridPrivate *private;
};

struct _LigmaToolTransformGridClass
{
  LigmaToolWidgetClass  parent_class;
};


GType                 ligma_tool_transform_grid_get_type   (void) G_GNUC_CONST;

LigmaToolWidget      * ligma_tool_transform_grid_new        (LigmaDisplayShell      *shell,
                                                           const LigmaMatrix3     *transform,
                                                           gdouble                x1,
                                                           gdouble                y1,
                                                           gdouble                x2,
                                                           gdouble                y2);

/*  protected functions  */

LigmaTransformHandle   ligma_tool_transform_grid_get_handle (LigmaToolTransformGrid *grid);


#endif /* __LIGMA_TOOL_TRANSFORM_GRID_H__ */
