/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include "text/text-types.h"

#include "config/gimpconfig.h"

#include "gimptext.h"
#include "gimptext-parasite.h"


GimpParasite *
gimp_text_to_parasite (const GimpText *text)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  str = gimp_config_serialize_to_string (G_OBJECT (text), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite = gimp_parasite_new (gimp_text_parasite_name (),
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GimpText *
gimp_text_from_parasite (const GimpParasite *parasite)
{
  GimpText    *text;
  const gchar *str;
  GError      *error = NULL;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                gimp_text_parasite_name ()) == 0, NULL);

  str = gimp_parasite_data (parasite);
  g_return_val_if_fail (str != NULL, NULL);
 
  text = g_object_new (GIMP_TYPE_TEXT, NULL);

  if (! gimp_config_deserialize_string (G_OBJECT (text),
                                        str,
                                        gimp_parasite_data_size (parasite),
                                        NULL,
                                        &error))
    {
      g_warning ("Failed to deserialize text parasite: %s", error->message);
      g_error_free (error);
    }

  return text;
}

const gchar *
gimp_text_parasite_name (void)
{
  return "gimp-text-layer";
}
