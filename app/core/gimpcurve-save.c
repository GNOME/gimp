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

#include "gimpcurve.h"
#include "gimpcurve-save.h"

#include "gimp-intl.h"


gboolean
gimp_curve_save (GimpData  *data,
                 GError   **error)
{
  /* GimpCurve *curve; */
  FILE      *file;

  g_return_val_if_fail (GIMP_IS_CURVE (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* curve = GIMP_CURVE (data); */

  file = g_fopen (gimp_data_get_filename (data), "wb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (gimp_data_get_filename (data)),
                   g_strerror (errno));
      return FALSE;
    }

  /* FIXME: write curve */

  fclose (file);

  return TRUE;
}
