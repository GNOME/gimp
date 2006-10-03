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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppalette.h"
#include "gimppalette-save.h"

#include "gimp-intl.h"


gboolean
gimp_palette_save (GimpData  *data,
                   GError   **error)
{
  GimpPalette *palette = GIMP_PALETTE (data);
  GList       *list;
  FILE        *file;

  file = g_fopen (data->filename, "w");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (data->filename),
                   g_strerror (errno));
      return FALSE;
    }

  fprintf (file, "GIMP Palette\n");
  fprintf (file, "Name: %s\n", GIMP_OBJECT (palette)->name);
  fprintf (file, "Columns: %d\n#\n", CLAMP (palette->n_columns, 0, 256));

  for (list = palette->colors; list; list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;
      guchar            r, g, b;

      gimp_rgb_get_uchar (&entry->color, &r, &g, &b);

      fprintf (file, "%3d %3d %3d\t%s\n", r, g, b, entry->name);
    }

  fclose (file);

  return TRUE;
}
