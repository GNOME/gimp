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

#include "gegl/gimptilehandlervalidate.h"


/***
 * GimpTileHandlerIscissors is a GeglTileHandler that renders the
 * Iscissors tool's gradmap.
 */

#define GIMP_TYPE_TILE_HANDLER_ISCISSORS            (gimp_tile_handler_iscissors_get_type ())
#define GIMP_TILE_HANDLER_ISCISSORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TILE_HANDLER_ISCISSORS, GimpTileHandlerIscissors))
#define GIMP_TILE_HANDLER_ISCISSORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_TILE_HANDLER_ISCISSORS, GimpTileHandlerIscissorsClass))
#define GIMP_IS_TILE_HANDLER_ISCISSORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TILE_HANDLER_ISCISSORS))
#define GIMP_IS_TILE_HANDLER_ISCISSORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_TILE_HANDLER_ISCISSORS))
#define GIMP_TILE_HANDLER_ISCISSORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_TILE_HANDLER_ISCISSORS, GimpTileHandlerIscissorsClass))


typedef struct _GimpTileHandlerIscissors      GimpTileHandlerIscissors;
typedef struct _GimpTileHandlerIscissorsClass GimpTileHandlerIscissorsClass;

struct _GimpTileHandlerIscissors
{
  GimpTileHandlerValidate  parent_instance;

  GimpPickable            *pickable;
};

struct _GimpTileHandlerIscissorsClass
{
  GimpTileHandlerValidateClass  parent_class;
};


GType             gimp_tile_handler_iscissors_get_type (void) G_GNUC_CONST;

GeglTileHandler * gimp_tile_handler_iscissors_new      (GimpPickable *pickable);
