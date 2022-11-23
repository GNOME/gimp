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

#include "config.h"

#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "ligmaprojectable.h"

#include "ligmatilehandlerprojectable.h"


static void   ligma_tile_handler_projectable_begin_validate (LigmaTileHandlerValidate *validate);
static void   ligma_tile_handler_projectable_end_validate   (LigmaTileHandlerValidate *validate);


G_DEFINE_TYPE (LigmaTileHandlerProjectable, ligma_tile_handler_projectable,
               LIGMA_TYPE_TILE_HANDLER_VALIDATE)

#define parent_class ligma_tile_handler_projectable_parent_class


static void
ligma_tile_handler_projectable_class_init (LigmaTileHandlerProjectableClass *klass)
{
  LigmaTileHandlerValidateClass *validate_class;

  validate_class = LIGMA_TILE_HANDLER_VALIDATE_CLASS (klass);

  validate_class->begin_validate = ligma_tile_handler_projectable_begin_validate;
  validate_class->end_validate   = ligma_tile_handler_projectable_end_validate;
}

static void
ligma_tile_handler_projectable_init (LigmaTileHandlerProjectable *projectable)
{
}

static void
ligma_tile_handler_projectable_begin_validate (LigmaTileHandlerValidate *validate)
{
  LigmaTileHandlerProjectable *handler = LIGMA_TILE_HANDLER_PROJECTABLE (validate);

  LIGMA_TILE_HANDLER_VALIDATE_CLASS (parent_class)->begin_validate (validate);

  ligma_projectable_begin_render (handler->projectable);
}

static void
ligma_tile_handler_projectable_end_validate (LigmaTileHandlerValidate *validate)
{
  LigmaTileHandlerProjectable *handler = LIGMA_TILE_HANDLER_PROJECTABLE (validate);

  ligma_projectable_end_render (handler->projectable);

  LIGMA_TILE_HANDLER_VALIDATE_CLASS (parent_class)->end_validate (validate);
}

GeglTileHandler *
ligma_tile_handler_projectable_new (LigmaProjectable *projectable)
{
  LigmaTileHandlerProjectable *handler;

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), NULL);

  handler = g_object_new (LIGMA_TYPE_TILE_HANDLER_PROJECTABLE, NULL);

  LIGMA_TILE_HANDLER_VALIDATE (handler)->graph =
    g_object_ref (ligma_projectable_get_graph (projectable));

  handler->projectable = projectable;

  return GEGL_TILE_HANDLER (handler);
}
