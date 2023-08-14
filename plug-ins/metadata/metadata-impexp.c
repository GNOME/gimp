/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata-editor.c
 * Copyright (C) 2016, 2017 Ben Touchette
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

#include <gexiv2/gexiv2.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metadata-xml.h"
#include "metadata-misc.h"
#include "metadata-tags.h"
#include "metadata-impexp.h"
#include "metadata-editor.h"

extern gboolean gimpmetadata;
extern gboolean xmptag;
extern gboolean iptctag;
extern gboolean tagvalue;
extern gboolean tagname;
extern gboolean force_write;
extern gchar *str_tag_value;
extern gchar *str_tag_name;

const GMarkupParser xml_markup_parser =
{
  xml_parser_start_element,
  xml_parser_end_element,
  xml_parser_data,
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};

typedef struct
{
  MetadataMode  mode;
  gchar        *mode_string;
} MetadataModeConversion;

const MetadataModeConversion metadata_mode_conversion[] =
{
  { MODE_SINGLE, "single" },
  { MODE_MULTI,  "multi"  },
  { MODE_COMBO,  "combo"  },
  { MODE_LIST,   "list"   },
};

/* ============================================================================
 * ==[ METADATA IMPORT TEMPLATE ]==============================================
 * ============================================================================
 */
void
import_file_metadata(metadata_editor *args)
{
  GimpXmlParser  *xml_parser;
  GError         *error = NULL;
  FILE           *file;

  gimpmetadata = FALSE;
  xmptag = FALSE;
  iptctag = FALSE;
  tagvalue = FALSE;
  tagname = FALSE;

  file = g_fopen (args->filename, "r");
  if (file != NULL)
    {
      /* parse xml data fetched from file */
      xml_parser = xml_parser_new (&xml_markup_parser, args);
      if (! xml_parser_parse_file (xml_parser, args->filename, &error))
        {
          g_warning ("Error parsing xml: %s.", error? error->message: "");
          g_clear_error (&error);
        }
      xml_parser_free (xml_parser);

      fclose (file);
    }
}

/* ============================================================================
 * ==[ METADATA EXPORT TEMPLATE ]==============================================
 * ============================================================================
 */
void
export_file_metadata (metadata_editor *args)
{
  FILE    *file;
  GString *xmldata;
  gint     i, size;

  if (force_write == TRUE)
    {
      /* Save fields in case of updates */
      metadata_editor_write_callback (args->dialog, args, args->image);
      /* Fetch a fresh copy of the metadata */
      args->metadata = GEXIV2_METADATA (gimp_image_get_metadata (args->image));
    }

  xmldata = g_string_new ("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                          "<gimp-metadata>\n");

  /* HANDLE IPTC */
  for (i = 0; i < n_equivalent_metadata_tags; i++)
    {
      int index = equivalent_metadata_tags[i].default_tag_index;
      g_string_append (xmldata, "\t<iptc-tag>\n");
      g_string_append (xmldata, "\t\t<tag-name>");
      g_string_append (xmldata, equivalent_metadata_tags[i].tag);
      g_string_append (xmldata, "</tag-name>\n");
      g_string_append (xmldata, "\t\t<tag-mode>");
      g_string_append (xmldata, metadata_mode_conversion[equivalent_metadata_tags[i].mode].mode_string);
      g_string_append (xmldata, "</tag-mode>\n");
      g_string_append (xmldata, "\t\t<tag-value>");

      if (default_metadata_tags[index].mode == MODE_SINGLE ||
          default_metadata_tags[index].mode == MODE_MULTI)
        {
          const gchar *value;

          value = get_tag_ui_text (args, default_metadata_tags[index].tag,
                                   default_metadata_tags[index].mode);

          if (value)
            {
              gchar *value_utf;

              value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
              g_string_append (xmldata, value_utf);
              g_free (value_utf);
            }
        }
      else if (default_metadata_tags[index].mode == MODE_COMBO)
        {
          gint data = get_tag_ui_combo (args, default_metadata_tags[index].tag,
                                         default_metadata_tags[index].mode);
          g_string_append_printf (xmldata, "%d", data);
        }
      else if (default_metadata_tags[i].mode == MODE_LIST)
        {
            /* No IPTC lists elements at this point */
        }

      g_string_append (xmldata, "</tag-value>\n");
      g_string_append (xmldata, "\t</iptc-tag>\n");
    }

  /* HANDLE XMP */
  for (i = 0; i < n_default_metadata_tags; i++)
    {
      g_string_append (xmldata, "\t<xmp-tag>\n");
      g_string_append (xmldata, "\t\t<tag-name>");
      g_string_append (xmldata, default_metadata_tags[i].tag);
      g_string_append (xmldata, "</tag-name>\n");
      g_string_append (xmldata, "\t\t<tag-mode>");
      g_string_append (xmldata, metadata_mode_conversion[default_metadata_tags[i].mode].mode_string);
      g_string_append (xmldata, "</tag-mode>\n");

      if (default_metadata_tags[i].mode == MODE_SINGLE ||
          default_metadata_tags[i].mode == MODE_MULTI)
        {
          const gchar *value;

          g_string_append (xmldata, "\t\t<tag-value>");
          value = get_tag_ui_text (args, default_metadata_tags[i].tag,
                                   default_metadata_tags[i].mode);

          if (value)
            {
              gchar   *value_utf;

              value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
              g_string_append (xmldata, value_utf);
              g_free (value_utf);
            }

          g_string_append (xmldata, "</tag-value>\n");
        }
      else if (default_metadata_tags[i].mode == MODE_COMBO)
        {
          gint data;

          g_string_append (xmldata, "\t\t<tag-value>");

          data = get_tag_ui_combo (args, default_metadata_tags[i].tag,
                                         default_metadata_tags[i].mode);
          g_string_append_printf (xmldata, "%d", data);

          g_string_append (xmldata, "</tag-value>\n");
        }
      else if (default_metadata_tags[i].mode == MODE_LIST)
        {
          gchar *data;

          g_string_append (xmldata, "\t\t<tag-list-value>\n");

          data = get_tag_ui_list (args, default_metadata_tags[i].tag,
                                        default_metadata_tags[i].mode);

          if (data)
            {
              g_string_append (xmldata, data);
              g_free(data);
            }

          g_string_append (xmldata, "\t\t</tag-list-value>\n");
        }

      g_string_append (xmldata, "\t</xmp-tag>\n");

    }

  g_string_append (xmldata, "</gimp-metadata>\n");


  size = strlen (xmldata->str);
  file = g_fopen (args->filename, "w");
  if (file != NULL)
    {
      GError *error = NULL;

      if (! g_file_set_contents (args->filename, xmldata->str, size, &error))
        {
          g_warning ("Error saving file: %s.", error? error->message: "");
          g_clear_error (&error);
        }
      fclose (file);
    }

  if (xmldata)
    {
      g_string_free(xmldata, TRUE);
    }
}

