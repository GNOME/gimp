/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-grid.h"
#include "gimpimage-private.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


GimpGrid *
gimp_image_get_grid (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->grid;
}

void
gimp_image_set_grid (GimpImage *image,
                     GimpGrid  *grid,
                     gboolean   push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GRID (grid));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (gimp_config_is_equal_to (GIMP_CONFIG (private->grid), GIMP_CONFIG (grid)))
    return;

  if (push_undo)
    gimp_image_undo_push_image_grid (image,
                                     C_("undo-type", "Grid"), private->grid);

  gimp_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
}
