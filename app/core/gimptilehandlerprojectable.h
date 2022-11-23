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

#ifndef __LIGMA_TILE_HANDLER_PROJECTABLE_H__
#define __LIGMA_TILE_HANDLER_PROJECTABLE_H__


#include "gegl/ligmatilehandlervalidate.h"


/***
 * LigmaTileHandlerProjectable is a GeglTileHandler that renders a
 * projectable, calling the projectable's begin_render() and end_render()
 * before/after the actual rendering.
 *
 * Note that the tile handler does not own a reference to the projectable.
 * It's the user's responsibility to manage the handler's and projectable's
 * lifetime.
 */

#define LIGMA_TYPE_TILE_HANDLER_PROJECTABLE            (ligma_tile_handler_projectable_get_type ())
#define LIGMA_TILE_HANDLER_PROJECTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TILE_HANDLER_PROJECTABLE, LigmaTileHandlerProjectable))
#define LIGMA_TILE_HANDLER_PROJECTABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_TILE_HANDLER_PROJECTABLE, LigmaTileHandlerProjectableClass))
#define LIGMA_IS_TILE_HANDLER_PROJECTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TILE_HANDLER_PROJECTABLE))
#define LIGMA_IS_TILE_HANDLER_PROJECTABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_TILE_HANDLER_PROJECTABLE))
#define LIGMA_TILE_HANDLER_PROJECTABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_TILE_HANDLER_PROJECTABLE, LigmaTileHandlerProjectableClass))


typedef struct _LigmaTileHandlerProjectable      LigmaTileHandlerProjectable;
typedef struct _LigmaTileHandlerProjectableClass LigmaTileHandlerProjectableClass;

struct _LigmaTileHandlerProjectable
{
  LigmaTileHandlerValidate  parent_instance;

  LigmaProjectable         *projectable;
};

struct _LigmaTileHandlerProjectableClass
{
  LigmaTileHandlerValidateClass  parent_class;
};


GType             ligma_tile_handler_projectable_get_type (void) G_GNUC_CONST;

GeglTileHandler * ligma_tile_handler_projectable_new      (LigmaProjectable *projectable);


#endif /* __LIGMA_TILE_HANDLER_PROJECTABLE_H__ */
