/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "text-types.h"

#include "core/ligmaerror.h"

#include "ligmatext.h"
#include "ligmatext-parasite.h"
#include "ligmatext-xlfd.h"

#include "ligma-intl.h"


/****************************************/
/*  The native LigmaTextLayer parasite.  */
/****************************************/

const gchar *
ligma_text_parasite_name (void)
{
  return "ligma-text-layer";
}

LigmaParasite *
ligma_text_to_parasite (LigmaText *text)
{
  g_return_val_if_fail (LIGMA_IS_TEXT (text), NULL);

  return ligma_config_serialize_to_parasite (LIGMA_CONFIG (text),
                                            ligma_text_parasite_name (),
                                            LIGMA_PARASITE_PERSISTENT,
                                            NULL);
}

LigmaText *
ligma_text_from_parasite (const LigmaParasite  *parasite,
                         Ligma                *ligma,
                         GError             **error)
{
  LigmaText *text;
  gchar    *parasite_data;
  guint32   parasite_data_size;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (ligma_parasite_get_name (parasite),
                                ligma_text_parasite_name ()) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  text = g_object_new (LIGMA_TYPE_TEXT, "ligma", ligma, NULL);

  parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_data_size);
  if (parasite_data)
    {
      parasite_data = g_strndup (parasite_data, parasite_data_size);
      ligma_config_deserialize_parasite (LIGMA_CONFIG (text),
                                        parasite,
                                        NULL,
                                        error);
      g_free (parasite_data);
    }
  else
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Empty text parasite"));
    }

  return text;
}


/****************************************************************/
/*  Compatibility to plug-in GDynText 1.4.4 and later versions  */
/*  GDynText was written by Marco Lamberto <lm@geocities.com>   */
/****************************************************************/

const gchar *
ligma_text_gdyntext_parasite_name (void)
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

LigmaText *
ligma_text_from_gdyntext_parasite (const LigmaParasite *parasite)
{
  LigmaText               *retval = NULL;
  LigmaTextJustification   justify;
  gchar                  *str;
  gchar                  *text = NULL;
  gchar                 **params;
  guint32                 parasite_data_size;
  gboolean                antialias;
  gdouble                 spacing;
  LigmaRGB                 rgb;
  glong                   color;
  gint                    i;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (ligma_parasite_get_name (parasite),
                                ligma_text_gdyntext_parasite_name ()) == 0,
                        NULL);

  str = (gchar *) ligma_parasite_get_data (parasite, &parasite_data_size);
  str = g_strndup (str, parasite_data_size);
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
      gchar *tmp = ligma_any_to_utf8 (text, -1, NULL);

      g_free (text);
      text = tmp;
    }

  antialias = atoi (params[ANTIALIAS]) ? TRUE : FALSE;

  switch (atoi (params[ALIGNMENT]))
    {
    default:
    case 0:  justify = LIGMA_TEXT_JUSTIFY_LEFT;   break;
    case 1:  justify = LIGMA_TEXT_JUSTIFY_CENTER; break;
    case 2:  justify = LIGMA_TEXT_JUSTIFY_RIGHT;  break;
    }

  spacing = g_strtod (params[LINE_SPACING], NULL);

  color = strtol (params[COLOR], NULL, 16);
  ligma_rgba_set_uchar (&rgb, color >> 16, color >> 8, color, 255);

  retval = g_object_new (LIGMA_TYPE_TEXT,
                         "text",         text,
                         "antialias",    antialias,
                         "justify",      justify,
                         "line-spacing", spacing,
                         "color",        &rgb,
                         NULL);

  ligma_text_set_font_from_xlfd (LIGMA_TEXT (retval), params[XLFD]);

 cleanup:
  g_free (str);
  g_free (text);
  g_strfreev (params);

  return retval;
}
