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

#include <stdlib.h>
#include <ctype.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <glib.h>
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
  gchar          *xmldata;

  gimpmetadata = FALSE;
  xmptag = FALSE;
  iptctag = FALSE;
  tagvalue = FALSE;
  tagname = FALSE;

  file = g_fopen (args->filename, "r");
  if (file != NULL)
    {
      /* get xml data from file */
      g_file_get_contents (args->filename, &xmldata, NULL, &error);

      /* parse xml data fetched from file */
      xml_parser = xml_parser_new (&xml_markup_parser, args);
      xml_parser_parse_file (xml_parser, args->filename, &error);
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
  GError *error = NULL;
  FILE   *file;
  gchar  *value;
  gchar  *value_utf;
  gchar  *xmldata;
  gint    i, size;

  if (force_write == TRUE)
    {
      /* Save fields in case of updates */
      metadata_editor_write_callback (args->dialog, args->builder, args->image_id);
      /* Fetch a fresh copy of the metadata */
      args->metadata = GEXIV2_METADATA (gimp_image_get_metadata (args->image_id));
    }

  xmldata = g_strconcat ("<?xml version=“1.0” encoding=“utf-8”?>\n",
                         "<gimp-metadata>\n", NULL);

  /* HANDLE IPTC */
  for (i = 0; i < n_equivalent_metadata_tags; i++)
    {
      int index = equivalent_metadata_tags[i].other_tag_index;
      xmldata = g_strconcat (xmldata, "\t<iptc-tag>\n", NULL);
      xmldata = g_strconcat (xmldata, "\t\t<tag-name>", NULL);
      xmldata = g_strconcat (xmldata, equivalent_metadata_tags[i].tag, NULL);
      xmldata = g_strconcat (xmldata, "</tag-name>\n", NULL);
      xmldata = g_strconcat (xmldata, "\t\t<tag-mode>", NULL);
      xmldata = g_strconcat (xmldata, equivalent_metadata_tags[i].mode, NULL);
      xmldata = g_strconcat (xmldata, "</tag-mode>\n", NULL);

      xmldata = g_strconcat (xmldata, "\t\t<tag-value>", NULL);

      if (!strcmp("single", default_metadata_tags[index].mode) ||
          !strcmp("multi", default_metadata_tags[index].mode))
        {
          const gchar *value;

          value = get_tag_ui_text (args, default_metadata_tags[index].tag,
                                   default_metadata_tags[index].mode);

          if (value)
            {
              value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
              xmldata = g_strconcat (xmldata, value_utf, NULL);
            }
        }
      else if (!strcmp("combo", default_metadata_tags[index].mode))
        {
          gint data = get_tag_ui_combo (args, default_metadata_tags[index].tag,
                                         default_metadata_tags[index].mode);
          value = g_malloc(1024);
          g_sprintf(value, "%d", data);
          xmldata = g_strconcat (xmldata, value, NULL);
          g_free(value);
        }
      else if (!strcmp("list", default_metadata_tags[i].mode))
        {
            /* No IPTC lists elements at this point */
        }

      xmldata = g_strconcat (xmldata, "</tag-value>\n", NULL);
      xmldata = g_strconcat (xmldata, "\t</iptc-tag>\n", NULL);
    }

  /* HANDLE XMP */
  for (i = 0; i < n_default_metadata_tags; i++)
    {
      xmldata = g_strconcat (xmldata, "\t<xmp-tag>\n", NULL);
      xmldata = g_strconcat (xmldata, "\t\t<tag-name>", NULL);
      xmldata = g_strconcat (xmldata, default_metadata_tags[i].tag, NULL);
      xmldata = g_strconcat (xmldata, "</tag-name>\n", NULL);
      xmldata = g_strconcat (xmldata, "\t\t<tag-mode>", NULL);
      xmldata = g_strconcat (xmldata, default_metadata_tags[i].mode, NULL);
      xmldata = g_strconcat (xmldata, "</tag-mode>\n", NULL);

      if (!strcmp("single", default_metadata_tags[i].mode) ||
          !strcmp("multi", default_metadata_tags[i].mode))
        {
          const gchar *value;

          xmldata = g_strconcat (xmldata, "\t\t<tag-value>", NULL);
          value = get_tag_ui_text (args, default_metadata_tags[i].tag,
                                   default_metadata_tags[i].mode);

          if (value)
            {
              value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
              xmldata = g_strconcat (xmldata, value_utf, NULL);
            }

          xmldata = g_strconcat (xmldata, "</tag-value>\n", NULL);
        }
      else if (!strcmp("combo", default_metadata_tags[i].mode))
        {
          gint data;

          xmldata = g_strconcat (xmldata, "\t\t<tag-value>", NULL);

          data = get_tag_ui_combo (args, default_metadata_tags[i].tag,
                                         default_metadata_tags[i].mode);
          value = g_malloc(1024);
          g_sprintf(value, "%d", data);
          xmldata = g_strconcat (xmldata, value, NULL);
          g_free(value);

          xmldata = g_strconcat (xmldata, "</tag-value>\n", NULL);
        }
      else if (!strcmp("list", default_metadata_tags[i].mode))
        {
          gchar *data;

          xmldata = g_strconcat (xmldata, "\t\t<tag-list-value>\n", NULL);

          data = get_tag_ui_list (args, default_metadata_tags[i].tag,
                                         default_metadata_tags[i].mode);
          xmldata = g_strconcat (xmldata, data, NULL);

          if (data)
            g_free(data);

          xmldata = g_strconcat (xmldata, "\t\t</tag-list-value>\n", NULL);
        }

      xmldata = g_strconcat (xmldata, "\t</xmp-tag>\n", NULL);
    }

  xmldata = g_strconcat(xmldata, "</gimp-metadata>\n", NULL);

  size = strlen (xmldata);
  file = g_fopen (args->filename, "w");
  if (file != NULL)
    {
      g_file_set_contents (args->filename, xmldata, size, &error);
      fclose (file);
    }

  if (xmldata)
    {
      g_free (xmldata);
    }
}

