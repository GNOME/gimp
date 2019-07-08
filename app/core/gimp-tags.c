/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-tags.c
 * Copyright (C) 2009 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimp-utils.h"
#include "gimp-tags.h"

#include "gimp-intl.h"


#define GIMP_TAGS_FILE "tags.xml"

typedef struct
{
  const gchar *locale;
  GString     *buf;
  gboolean     locale_matches;
} GimpTagsInstaller;


static  void        gimp_tags_installer_load_start_element (GMarkupParseContext *context,
                                                            const gchar         *element_name,
                                                            const gchar        **attribute_names,
                                                            const gchar        **attribute_values,
                                                            gpointer             user_data,
                                                            GError             **error);
static void         gimp_tags_installer_load_end_element   (GMarkupParseContext *context,
                                                            const gchar         *element_name,
                                                            gpointer             user_data,
                                                            GError             **error);
static void         gimp_tags_installer_load_text          (GMarkupParseContext *context,
                                                            const gchar         *text,
                                                            gsize                text_len,
                                                            gpointer             user_data,
                                                            GError             **error);
static const gchar* attribute_name_to_value                (const gchar        **attribute_names,
                                                            const gchar        **attribute_values,
                                                            const gchar         *name);


gboolean
gimp_tags_user_install (void)
{
  GFile             *file;
  GOutputStream     *output;
  GMarkupParser      markup_parser;
  GimpXmlParser     *xml_parser;
  const char        *tags_locale;
  GimpTagsInstaller  tags_installer = { 0, };
  GError            *error          = NULL;
  gboolean           result         = TRUE;

  /* This is a special string to specify the language identifier to
   * look for in the gimp-tags-default.xml file. Please translate the
   * C in it according to the name of the po file used for
   * gimp-tags-default.xml. E.g. lithuanian for the translation,
   * that would be "tags-locale:lt".
   */
  tags_locale = _("tags-locale:C");

  if (g_str_has_prefix (tags_locale, "tags-locale:"))
    {
      tags_locale += strlen ("tags-locale:");

      if (*tags_locale && *tags_locale != 'C')
        tags_installer.locale = tags_locale;
    }
  else
    {
      g_warning ("Wrong translation for 'tags-locale:', fix the translation!");
    }

  tags_installer.buf = g_string_new (NULL);

  g_string_append (tags_installer.buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (tags_installer.buf, "<tags>\n");

  markup_parser.start_element = gimp_tags_installer_load_start_element;
  markup_parser.end_element   = gimp_tags_installer_load_end_element;
  markup_parser.text          = gimp_tags_installer_load_text;
  markup_parser.passthrough   = NULL;
  markup_parser.error         = NULL;

  xml_parser = gimp_xml_parser_new (&markup_parser, &tags_installer);

  file = gimp_data_directory_file ("tags", "gimp-tags-default.xml", NULL);
  result = gimp_xml_parser_parse_gfile (xml_parser, file, &error);
  g_object_unref (file);

  gimp_xml_parser_free (xml_parser);

  if (! result)
    {
      g_string_free (tags_installer.buf, TRUE);
      return FALSE;
    }

  g_string_append (tags_installer.buf, "\n</tags>\n");

  file = gimp_directory_file (GIMP_TAGS_FILE, NULL);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      g_printerr ("%s\n", error->message);
      result = FALSE;
    }
  else if (! g_output_stream_write_all (output,
                                        tags_installer.buf->str,
                                        tags_installer.buf->len,
                                        NULL, NULL, &error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_printerr (_("Error writing '%s': %s"),
                  gimp_file_get_utf8_name (file), error->message);
      result = FALSE;

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }
  else if (! g_output_stream_close (output, NULL, &error))
    {
      g_printerr (_("Error closing '%s': %s"),
                  gimp_file_get_utf8_name (file), error->message);
      result = FALSE;
    }

  if (output)
    g_object_unref (output);

  g_clear_error (&error);
  g_object_unref (file);
  g_string_free (tags_installer.buf, TRUE);

  return result;
}

static  void
gimp_tags_installer_load_start_element (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        gpointer              user_data,
                                        GError              **error)
{
  GimpTagsInstaller *tags_installer = user_data;

  if (! strcmp (element_name, "resource"))
    {
      g_string_append_printf (tags_installer->buf, "\n  <resource");

      while (*attribute_names)
        {
          g_string_append_printf (tags_installer->buf, " %s=\"%s\"",
                                  *attribute_names, *attribute_values);

          attribute_names++;
          attribute_values++;
        }

      g_string_append_printf (tags_installer->buf, ">\n");
    }
  else if (! strcmp (element_name, "thetag"))
    {
      const char *current_locale;

      current_locale = attribute_name_to_value (attribute_names, attribute_values,
                                                "xml:lang");

      if (current_locale && tags_installer->locale)
        {
          tags_installer->locale_matches = ! strcmp (current_locale,
                                                     tags_installer->locale);
        }
      else
        {
          tags_installer->locale_matches = (current_locale ==
                                            tags_installer->locale);
        }
    }
}

static void
gimp_tags_installer_load_end_element (GMarkupParseContext  *context,
                                      const gchar          *element_name,
                                      gpointer              user_data,
                                      GError              **error)
{
  GimpTagsInstaller *tags_installer = user_data;

  if (strcmp (element_name, "resource") == 0)
    {
      g_string_append (tags_installer->buf, "  </resource>\n");
    }
}

static void
gimp_tags_installer_load_text (GMarkupParseContext  *context,
                               const gchar          *text,
                               gsize                 text_len,
                               gpointer              user_data,
                               GError              **error)
{
  GimpTagsInstaller *tags_installer = user_data;
  const gchar       *current_element;
  gchar             *tag_string;

  current_element = g_markup_parse_context_get_element (context);

  if (tags_installer->locale_matches &&
      current_element &&
      strcmp (current_element, "thetag") == 0)
    {
      tag_string = g_markup_escape_text (text, text_len);
      g_string_append_printf (tags_installer->buf, "    <tag>%s</tag>\n",
                              tag_string);
      g_free (tag_string);
    }
}

static const gchar *
attribute_name_to_value (const gchar **attribute_names,
                         const gchar **attribute_values,
                         const gchar  *name)
{
  while (*attribute_names)
    {
      if (! strcmp (*attribute_names, name))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }

  return NULL;
}
