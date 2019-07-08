/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "core/gimperror.h"

#include "gimptext.h"
#include "gimptext-parasite.h"
#include "gimptext-xlfd.h"

#include "gimp-intl.h"


/****************************************/
/*  The native GimpTextLayer parasite.  */
/****************************************/

const gchar *
gimp_text_parasite_name (void)
{
  return "gimp-text-layer";
}

GimpParasite *
gimp_text_to_parasite (const GimpText *text)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  str = gimp_config_serialize_to_string (GIMP_CONFIG (text), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite = gimp_parasite_new (gimp_text_parasite_name (),
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GimpText *
gimp_text_from_parasite (const GimpParasite  *parasite,
                         GError             **error)
{
  GimpText    *text;
  const gchar *str;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                gimp_text_parasite_name ()) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  str = gimp_parasite_data (parasite);

  text = g_object_new (GIMP_TYPE_TEXT, NULL);

  if (str != NULL)
    {
      gimp_config_deserialize_string (GIMP_CONFIG (text),
                                      str,
                                      gimp_parasite_data_size (parasite),
                                      NULL,
                                      error);
    }
  else
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Empty text parasite"));
    }

  return text;
}


/****************************************************************/
/*  Compatibility to plug-in GDynText 1.4.4 and later versions  */
/*  GDynText was written by Marco Lamberto <lm@geocities.com>   */
/****************************************************************/

const gchar *
gimp_text_gdyntext_parasite_name (void)
{
  return "plug_in_gdyntext/data";
}

enum
{
  TEXT            = 0,
  ANTIALIAS       = 1,
  ALIGNMENT       = 2,
  ROTATION        = 3,
  LINE_SPACING    = 4,
  COLOR           = 5,
  LAYER_ALIGNMENT = 6,
  XLFD            = 7,
  NUM_PARAMS
};

GimpText *
gimp_text_from_gdyntext_parasite (const GimpParasite *parasite)
{
  GimpText               *retval = NULL;
  GimpTextJustification   justify;
  const gchar            *str;
  gchar                  *text = NULL;
  gchar                 **params;
  gboolean                antialias;
  gdouble                 spacing;
  GimpRGB                 rgb;
  glong                   color;
  gint                    i;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                gimp_text_gdyntext_parasite_name ()) == 0,
                        NULL);

  str = gimp_parasite_data (parasite);
  g_return_val_if_fail (str != NULL, NULL);

  if (! g_str_has_prefix (str, "GDT10{"))  /*  magic value  */
    return NULL;

  params = g_strsplit (str + strlen ("GDT10{"), "}{", -1);

  /*  first check that we have the required number of parameters  */
  for (i = 0; i < NUM_PARAMS; i++)
    if (!params[i])
      goto cleanup;

  text = g_strcompress (params[TEXT]);

  if (! g_utf8_validate (text, -1, NULL))
    {
      gchar *tmp = gimp_any_to_utf8 (text, -1, NULL);

      g_free (text);
      text = tmp;
    }

  antialias = atoi (params[ANTIALIAS]) ? TRUE : FALSE;

  switch (atoi (params[ALIGNMENT]))
    {
    default:
    case 0:  justify = GIMP_TEXT_JUSTIFY_LEFT;   break;
    case 1:  justify = GIMP_TEXT_JUSTIFY_CENTER; break;
    case 2:  justify = GIMP_TEXT_JUSTIFY_RIGHT;  break;
    }

  spacing = g_strtod (params[LINE_SPACING], NULL);

  color = strtol (params[COLOR], NULL, 16);
  gimp_rgba_set_uchar (&rgb, color >> 16, color >> 8, color, 255);

  retval = g_object_new (GIMP_TYPE_TEXT,
                         "text",         text,
                         "antialias",    antialias,
                         "justify",      justify,
                         "line-spacing", spacing,
                         "color",        &rgb,
                         NULL);

  gimp_text_set_font_from_xlfd (GIMP_TEXT (retval), params[XLFD]);

 cleanup:
  g_free (text);
  g_strfreev (params);

  return retval;
}
