/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdio.h>
#include <errno.h>

#include <glib-object.h>

#include "vectors-types.h"

#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-export.h"

#include "gimp-intl.h"


gboolean
gimp_vectors_export (const GimpVectors  *vectors,
                     const gchar        *filename,
                     GError            **error)
{
  FILE *file;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = fopen (filename, "w");
  if (!file)
    {
      g_set_error (error, 0, 0,
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  /* FIXME:  add export implementation here */
  g_warning ("gimp_vectors_export: unimplemented");

  if (fclose (file))
    {
      g_set_error (error, 0, 0,
                   _("Error while writing '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}
