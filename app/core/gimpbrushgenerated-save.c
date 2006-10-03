/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp_brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrushgenerated.h"
#include "gimpbrushgenerated-save.h"

#include "gimp-intl.h"


gboolean
gimp_brush_generated_save (GimpData  *data,
                           GError   **error)
{
  GimpBrushGenerated *brush = GIMP_BRUSH_GENERATED (data);
  const gchar        *name  = gimp_object_get_name (GIMP_OBJECT (data));
  FILE               *file;
  gchar               buf[G_ASCII_DTOSTR_BUF_SIZE];
  gboolean            have_shape = FALSE;

  g_return_val_if_fail (name != NULL && *name != '\0', FALSE);

  file = g_fopen (data->filename, "wb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (data->filename),
                   g_strerror (errno));
      return FALSE;
    }

  /* write magic header */
  fprintf (file, "GIMP-VBR\n");

  /* write version */
  if (brush->shape != GIMP_BRUSH_GENERATED_CIRCLE || brush->spikes > 2)
    {
      fprintf (file, "1.5\n");
      have_shape = TRUE;
    }
  else
    {
      fprintf (file, "1.0\n");
    }

  /* write name */
  fprintf (file, "%.255s\n", name);

  if (have_shape)
    {
      GEnumClass *enum_class;
      GEnumValue *shape_val;

      enum_class = g_type_class_peek (GIMP_TYPE_BRUSH_GENERATED_SHAPE);

      /* write shape */
      shape_val = g_enum_get_value (enum_class, brush->shape);
      fprintf (file, "%s\n", shape_val->value_nick);
    }

  /* write brush spacing */
  fprintf (file, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            GIMP_BRUSH (brush)->spacing));

  /* write brush radius */
  fprintf (file, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->radius));

  if (have_shape)
    {
      /* write brush spikes */
      fprintf (file, "%d\n", brush->spikes);
    }

  /* write brush hardness */
  fprintf (file, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->hardness));

  /* write brush aspect_ratio */
  fprintf (file, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->aspect_ratio));

  /* write brush angle */
  fprintf (file, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->angle));

  fclose (file);

  return TRUE;
}
