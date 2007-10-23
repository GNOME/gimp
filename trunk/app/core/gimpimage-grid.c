/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-grid.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


GimpGrid *
gimp_image_get_grid (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->grid;
}

void
gimp_image_set_grid (GimpImage *image,
                     GimpGrid  *grid,
                     gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GRID (grid));

  if (gimp_config_is_equal_to (GIMP_CONFIG (image->grid), GIMP_CONFIG (grid)))
    return;

  if (push_undo)
    gimp_image_undo_push_image_grid (image, _("Grid"), image->grid);

  gimp_config_sync (G_OBJECT (grid), G_OBJECT (image->grid), 0);
}
