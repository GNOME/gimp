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
#include <pango/pango.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "core/gimperror.h"

#include "gimpfont.h"
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
gimp_text_to_parasite (GimpText *text)
{
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  return gimp_config_serialize_to_parasite (GIMP_CONFIG (text),
                                            gimp_text_parasite_name (),
                                            GIMP_PARASITE_PERSISTENT,
                                            NULL);
}

GimpText *
gimp_text_from_parasite (const GimpParasite  *parasite,
                         Gimp                *gimp,
                         gboolean            *before_xcf_v19,
                         GError             **error)
{
  GimpText *text;
  gchar    *parasite_data;
  guint32   parasite_data_size;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_get_name (parasite),
                                gimp_text_parasite_name ()) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  text = g_object_new (GIMP_TYPE_TEXT, "gimp", gimp, NULL);
  g_object_set (text, "font", gimp_font_get_standard(), NULL);

  parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_data_size);
  if (parasite_data)
    {
      gboolean      has_markup   = g_str_has_prefix (parasite_data, "(markup ");
      GimpParasite *new_parasite = NULL;
      GString      *new_data;

      *before_xcf_v19 = (strstr (parasite_data, "(font \"GimpFont\"") == NULL);
      /* This is for backward compatibility with older xcf files.
       * font used to be serialized as a string, but now it is serialized/deserialized as
       * GimpFont, so the object Type name is inserted for the GimpFont deserialization function to be called.
       * And more importantly, fonts in the markup are extracted into their own fields for deserialization.
       */
      if (*before_xcf_v19)
        {
          new_data = g_string_new (parasite_data);
          g_string_replace (new_data, "\")\n(font", "\")\n(font \"GimpFont\"", 1);

          if (has_markup)
            {
              char *markup_start = strstr (parasite_data, "\"<");
              char *markup_end   = strstr (parasite_data, ">\")");

              if (markup_start != NULL && markup_end != NULL)
                {
                  PangoAttrList *attr_list;
                  gchar         *desc;
                  guint          length;
                  GSList        *list             = NULL;
                  GSList        *fonts            = NULL;
                  GString       *markup_fonts     = g_string_new (NULL);
                  glong          markup_start_pos;
                  glong          markup_end_pos;
                  gchar         *markup_str;
                  GString       *markup;

                  markup_start_pos = (glong) (markup_start - parasite_data) + 1;
                  markup_end_pos   = (glong) (markup_end - parasite_data) + 1;
                  markup_str       = g_utf8_substring (parasite_data, markup_start_pos, markup_end_pos);
                  markup           = g_string_new (markup_str);
                  g_string_replace (markup, "\\\"", "\"", 0);
                  pango_parse_markup (markup->str, -1, 0, &attr_list, NULL, NULL, NULL);

                  list   = pango_attr_list_get_attributes (attr_list);
                  length = g_slist_length (list);

                  for (guint i = 0; i < length; ++i)
                    {
                      PangoAttrFontDesc *attr_font_desc = pango_attribute_as_font_desc ((PangoAttribute*)g_slist_nth_data (list, i));

                      if (attr_font_desc != NULL)
                        {
                          desc = pango_font_description_to_string (attr_font_desc->desc);

                          if (g_slist_find_custom (fonts, (gconstpointer) desc, (GCompareFunc) g_strcmp0) == NULL)
                            {
                              fonts = g_slist_prepend (fonts, (gpointer) desc);
                              /*duplicate font name to making parsing easier when deserializing*/
                              g_string_append_printf (markup_fonts,
                                                      "\n\"%s\" \"%s\"",
                                                      desc, desc);
                            }
                          else
                            {
                              g_free (desc);
                            }
                        }
                    }
                  g_slist_free_full (fonts, (GDestroyNotify) g_free);
                  g_slist_free_full (list,  (GDestroyNotify) pango_attribute_destroy);
                  pango_attr_list_unref (attr_list);

                  g_string_insert (new_data, markup_end_pos + 1, markup_fonts->str);

                  g_free (markup_str);
                  g_string_free (markup_fonts, TRUE);
                  g_string_free (markup, TRUE);
                }
              else
                {
                  /* We could not find the markup delimiters. */
                  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                       _("Invalid markup format in text parasite"));
                }
            }

          new_parasite = gimp_parasite_new (gimp_parasite_get_name  (parasite),
                                            gimp_parasite_get_flags (parasite),
                                            new_data->len+1,
                                            new_data->str);
          parasite = new_parasite;

          g_string_free (new_data, TRUE);
        }

      if (error == NULL || *error == NULL)
        gimp_config_deserialize_parasite (GIMP_CONFIG (text),
                                          parasite,
                                          NULL,
                                          error);

      gimp_parasite_free (new_parasite);
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
gimp_text_from_gdyntext_parasite (Gimp               *gimp,
                                  const GimpParasite *parasite)
{
  GimpText               *retval = NULL;
  GimpTextJustification   justify;
  gchar                  *str;
  gchar                  *text = NULL;
  gchar                 **params;
  guint32                 parasite_data_size;
  gboolean                antialias;
  gdouble                 spacing;
  GeglColor              *rgb  = gegl_color_new ("none");
  glong                   color;
  gint                    i;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_get_name (parasite),
                                gimp_text_gdyntext_parasite_name ()) == 0,
                        NULL);

  str = (gchar *) gimp_parasite_get_data (parasite, &parasite_data_size);
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
  gegl_color_set_rgba (rgb, (color >> 16) / 255.0f, (color >> 8) / 255.0f,
                       color / 255.0f, 1.0);

  retval = g_object_new (GIMP_TYPE_TEXT,
                         "gimp",         gimp,
                         "text",         text,
                         "antialias",    antialias,
                         "justify",      justify,
                         "line-spacing", spacing,
                         "color",        rgb,
                         NULL);

  gimp_text_set_font_from_xlfd (GIMP_TEXT (retval), params[XLFD]);

 cleanup:
  g_free (str);
  g_free (text);
  g_strfreev (params);
  g_object_unref (rgb);

  return retval;
}
