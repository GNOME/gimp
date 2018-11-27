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

#include "config.h"

#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "gimpprojectable.h"

#include "gimptilehandlerprojectable.h"


static void   gimp_tile_handler_projectable_begin_validate (GimpTileHandlerValidate *validate);
static void   gimp_tile_handler_projectable_end_validate   (GimpTileHandlerValidate *validate);


G_DEFINE_TYPE (GimpTileHandlerProjectable, gimp_tile_handler_projectable,
               GIMP_TYPE_TILE_HANDLER_VALIDATE)

#define parent_class gimp_tile_handler_projectable_parent_class


static void
gimp_tile_handler_projectable_class_init (GimpTileHandlerProjectableClass *klass)
{
  GimpTileHandlerValidateClass *validate_class;

  validate_class = GIMP_TILE_HANDLER_VALIDATE_CLASS (klass);

  validate_class->begin_validate = gimp_tile_handler_projectable_begin_validate;
  validate_class->end_validate   = gimp_tile_handler_projectable_end_validate;
}

static void
gimp_tile_handler_projectable_init (GimpTileHandlerProjectable *projectable)
{
}

static void
gimp_tile_handler_projectable_begin_validate (GimpTileHandlerValidate *validate)
{
  GimpTileHandlerProjectable *handler = GIMP_TILE_HANDLER_PROJECTABLE (validate);

  GIMP_TILE_HANDLER_VALIDATE_CLASS (parent_class)->begin_validate (validate);

  gimp_projectable_begin_render (handler->projectable);
}

static void
gimp_tile_handler_projectable_end_validate (GimpTileHandlerValidate *validate)
{
  GimpTileHandlerProjectable *handler = GIMP_TILE_HANDLER_PROJECTABLE (validate);

  gimp_projectable_end_render (handler->projectable);

  GIMP_TILE_HANDLER_VALIDATE_CLASS (parent_class)->end_validate (validate);
}

GeglTileHandler *
gimp_tile_handler_projectable_new (GimpProjectable *projectable)
{
  GimpTileHandlerProjectable *handler;

  g_return_val_if_fail (GIMP_IS_PROJECTABLE (projectable), NULL);

  handler = g_object_new (GIMP_TYPE_TILE_HANDLER_PROJECTABLE, NULL);

  GIMP_TILE_HANDLER_VALIDATE (handler)->graph =
    g_object_ref (gimp_projectable_get_graph (projectable));

  handler->projectable = projectable;

  return GEGL_TILE_HANDLER (handler);
}
