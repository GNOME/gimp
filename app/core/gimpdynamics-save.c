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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpdynamics.h"
#include "gimpdynamics-save.h"

#include "gimp-intl.h"


gboolean
gimp_dynamics_save (GimpData  *data,
                    GError   **error)
{
  GimpDynamics *dynamics;
  FILE         *file;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dynamics = GIMP_DYNAMICS (data);

  file = g_fopen (data->filename, "wb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (data->filename),
                   g_strerror (errno));
      return FALSE;
    }

  /* FIXME: write dynamics */

  fclose (file);

  return TRUE;
}
