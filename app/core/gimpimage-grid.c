/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmagrid.h"
#include "ligmaimage.h"
#include "ligmaimage-grid.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo-push.h"

#include "ligma-intl.h"


LigmaGrid *
ligma_image_get_grid (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->grid;
}

void
ligma_image_set_grid (LigmaImage *image,
                     LigmaGrid  *grid,
                     gboolean   push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GRID (grid));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_config_is_equal_to (LIGMA_CONFIG (private->grid), LIGMA_CONFIG (grid)))
    return;

  if (push_undo)
    ligma_image_undo_push_image_grid (image,
                                     C_("undo-type", "Grid"), private->grid);

  ligma_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
}
