/* The GIMP -- an image manipulation program
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

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-grid.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


GimpGrid *
gimp_image_get_grid (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->grid;
}

void
gimp_image_set_grid (GimpImage *gimage,
                     GimpGrid  *grid,
                     gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_GRID (grid));

  if (gimp_config_is_equal_to (GIMP_CONFIG (gimage->grid), GIMP_CONFIG (grid)))
    return;

  if (push_undo)
    gimp_image_undo_push_image_grid (gimage, _("Grid"), gimage->grid);

  gimp_config_sync (GIMP_CONFIG (grid), GIMP_CONFIG (gimage->grid), 0);
}
