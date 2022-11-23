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

#ifndef __LIGMA_TILE_HANDLER_ISCISSORS_H__
#define __LIGMA_TILE_HANDLER_ISCISSORS_H__


#include "gegl/ligmatilehandlervalidate.h"


/***
 * LigmaTileHandlerIscissors is a GeglTileHandler that renders the
 * Iscissors tool's gradmap.
 */

#define LIGMA_TYPE_TILE_HANDLER_ISCISSORS            (ligma_tile_handler_iscissors_get_type ())
#define LIGMA_TILE_HANDLER_ISCISSORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TILE_HANDLER_ISCISSORS, LigmaTileHandlerIscissors))
#define LIGMA_TILE_HANDLER_ISCISSORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_TILE_HANDLER_ISCISSORS, LigmaTileHandlerIscissorsClass))
#define LIGMA_IS_TILE_HANDLER_ISCISSORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TILE_HANDLER_ISCISSORS))
#define LIGMA_IS_TILE_HANDLER_ISCISSORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_TILE_HANDLER_ISCISSORS))
#define LIGMA_TILE_HANDLER_ISCISSORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_TILE_HANDLER_ISCISSORS, LigmaTileHandlerIscissorsClass))


typedef struct _LigmaTileHandlerIscissors      LigmaTileHandlerIscissors;
typedef struct _LigmaTileHandlerIscissorsClass LigmaTileHandlerIscissorsClass;

struct _LigmaTileHandlerIscissors
{
  LigmaTileHandlerValidate  parent_instance;

  LigmaPickable            *pickable;
};

struct _LigmaTileHandlerIscissorsClass
{
  LigmaTileHandlerValidateClass  parent_class;
};


GType             ligma_tile_handler_iscissors_get_type (void) G_GNUC_CONST;

GeglTileHandler * ligma_tile_handler_iscissors_new      (LigmaPickable *pickable);


#endif /* __LIGMA_TILE_HANDLER_ISCISSORS_H__ */
