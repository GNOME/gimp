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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig.h"

#include "gimp.h"
#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-grid.h"
#include "gimpimage-undo.h"
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
  g_return_if_fail (grid == NULL || GIMP_IS_GRID (grid));

  if (grid != gimage->grid)
    {
      if (push_undo)
        gimp_image_undo_push_image_grid (gimage, _("Grid"), gimage->grid);

      if (gimage->grid)
        g_object_unref (gimage->grid);

      gimage->grid = grid;

      if (gimage->grid)
        g_object_ref (gimage->grid);
    }

  gimp_image_grid_changed (gimage);
}

const gchar *
gimp_grid_parasite_name (void)
{
  return "gimp-image-grid";
}

GimpParasite *
gimp_grid_to_parasite (const GimpGrid *grid)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_GRID (grid), NULL);

  str = gimp_config_serialize_to_string (G_OBJECT (grid), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite = gimp_parasite_new (gimp_grid_parasite_name (),
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GimpGrid *
gimp_grid_from_parasite (const GimpParasite *parasite)
{
  GimpGrid    *grid;
  const gchar *str;
  GError      *error = NULL;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                gimp_grid_parasite_name ()) == 0, NULL);

  str = gimp_parasite_data (parasite);
  g_return_val_if_fail (str != NULL, NULL);
 
  grid = g_object_new (GIMP_TYPE_GRID, NULL);

  if (! gimp_config_deserialize_string (G_OBJECT (grid),
                                        str,
                                        gimp_parasite_data_size (parasite),
                                        NULL,
                                        &error))
    {
      g_warning ("Failed to deserialize grid parasite: %s", error->message);
      g_error_free (error);
    }

  return grid;
}
