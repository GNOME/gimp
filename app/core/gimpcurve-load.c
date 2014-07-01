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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "core-types.h"

#include "gimpcurve.h"
#include "gimpcurve-load.h"

#include "gimp-intl.h"


GList *
gimp_curve_load (GFile   *file,
                 GError **error)
{
  gchar *path;
  FILE  *f;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  path = g_file_get_path (file);

  g_return_val_if_fail (g_path_is_absolute (path), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  f = g_fopen (path, "rb");
  g_free (path);

  if (! f)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* load curves */

  fclose (f);

  return NULL;
}
