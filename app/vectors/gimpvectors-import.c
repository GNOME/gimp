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

#include <string.h>

#include <glib-object.h>

#include "vectors-types.h"

#include "core/gimpimage.h"

#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-import.h"

#include "gimp-intl.h"


GimpVectors *
gimp_vectors_import (GimpImage    *image,
                     const gchar  *filename,
                     GError      **error)
{
  GimpVectors *vectors;
  FILE        *file;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = fopen (filename, "r");
  if (!file)
    {
      g_set_error (error, 0, 0,
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return NULL;
    }

  vectors = gimp_vectors_new (image, "Imported Path");

  /* FIXME:  add export implementation here */
  g_warning ("gimp_vectors_import: unimplemented");

  fclose (file);

  return vectors;
}
