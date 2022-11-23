/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolhandlegrid.h
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

#ifndef __LIGMA_TOOL_HANDLE_GRID_H__
#define __LIGMA_TOOL_HANDLE_GRID_H__


#include "ligmatooltransformgrid.h"


#define LIGMA_TYPE_TOOL_HANDLE_GRID            (ligma_tool_handle_grid_get_type ())
#define LIGMA_TOOL_HANDLE_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_HANDLE_GRID, LigmaToolHandleGrid))
#define LIGMA_TOOL_HANDLE_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_HANDLE_GRID, LigmaToolHandleGridClass))
#define LIGMA_IS_TOOL_HANDLE_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_HANDLE_GRID))
#define LIGMA_IS_TOOL_HANDLE_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_HANDLE_GRID))
#define LIGMA_TOOL_HANDLE_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_HANDLE_GRID, LigmaToolHandleGridClass))


typedef struct _LigmaToolHandleGrid        LigmaToolHandleGrid;
typedef struct _LigmaToolHandleGridPrivate LigmaToolHandleGridPrivate;
typedef struct _LigmaToolHandleGridClass   LigmaToolHandleGridClass;

struct _LigmaToolHandleGrid
{
  LigmaToolTransformGrid      parent_instance;

  LigmaToolHandleGridPrivate *private;
};

struct _LigmaToolHandleGridClass
{
  LigmaToolTransformGridClass  parent_class;
};


GType            ligma_tool_handle_grid_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_handle_grid_new      (LigmaDisplayShell  *shell,
                                                 gdouble            x1,
                                                 gdouble            y1,
                                                 gdouble            x2,
                                                 gdouble            y2);


#endif /* __LIGMA_TOOL_HANDLE_GRID_H__ */
