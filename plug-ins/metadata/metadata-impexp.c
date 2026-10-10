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

extern gboolean xmptag;
extern gboolean iptctag;
extern gboolean tagvalue;
extern gboolean tagname;
#ifdef _ENABLE_FORCE_WRITE_
extern gboolean force_write;
#endif
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

static gboolean export_ignore_tag               (gchar   *tag);

static void     export_write_tag_header         (GString *xmldata,
                                                 gchar   *tag_category,
                                                 gchar   *tag,
                                                 gchar   *mode);

static void     export_write_tag_footer         (GString *xmldata,
                                                 gchar   *tag_category);

static void     export_write_string_value       (GString *xmldata,
                                                 gchar   *value_utf8);

static void     export_write_int_value          (GString *xmldata,
                                                 gint     value);

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
          g_warning (_("Error parsing xml: %s."), error? error->message: "");
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

gboolean
export_ignore_tag (gchar *tag)
{
  /* Eventually we may want to add handling a list of items,
   * but for now, just block Date Created:
   * It doesn't make sense to me to overwrite the date an image
   * was created with a template. */

  return strcmp (tag, "Xmp.photoshop.DateCreated")     == 0 ||
         strcmp (tag, "Iptc.Application2.DateCreated") == 0 ;
}

void
export_write_tag_header (GString *xmldata,
                         gchar   *tag_category,
                         gchar   *tag,
                         gchar   *mode)
{
  g_string_append (xmldata, "\t<");
  g_string_append (xmldata, tag_category);
  g_string_append (xmldata, "-tag>\n");
  g_string_append (xmldata, "\t\t<tag-name>");
  g_string_append (xmldata, tag);
  g_string_append (xmldata, "</tag-name>\n");
  g_string_append (xmldata, "\t\t<tag-mode>");
  g_string_append (xmldata, mode);
  g_string_append (xmldata, "</tag-mode>\n");
}

void
export_write_tag_footer (GString *xmldata,
                         gchar   *tag_category)
{
  g_string_append (xmldata, "\t</");
  g_string_append (xmldata, tag_category);
  g_string_append (xmldata, "-tag>\n");
}

void
export_write_string_value (GString *xmldata,
                           gchar   *value_utf8)
{
  g_string_append (xmldata, "\t\t<tag-value>");
  g_string_append (xmldata, value_utf8);
  g_string_append (xmldata, "</tag-value>\n");
}

void
export_write_int_value (GString *xmldata,
                        gint     value)
{
  g_string_append (xmldata, "\t\t<tag-value>");
  g_string_append_printf (xmldata, "%d", value);
  g_string_append (xmldata, "</tag-value>\n");
}

void
export_file_metadata (metadata_editor *args)
{
  GString *xmldata;
  gint     i;

#ifdef _ENABLE_FORCE_WRITE_
  if (force_write == TRUE)
    {
      /* Save fields in case of updates */
      metadata_editor_write_callback (args->dialog, args, args->image);
      /* Fetch a fresh copy of the metadata */
      args->metadata = GEXIV2_METADATA (gimp_image_get_metadata (args->image));
    }
#endif

  xmldata = g_string_new ("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                          "<gimp-metadata>\n");

#ifdef _ENABLE_IPTC_TAG_
  /* HANDLE IPTC */
  for (i = 0; i < n_equivalent_metadata_tags; i++)
    {
      int index = equivalent_metadata_tags[i].default_tag_index;

      if (export_ignore_tag (equivalent_metadata_tags[i].tag))
        continue;

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
              if (strlen(value_utf) > 0)
                {
                  export_write_tag_header (xmldata, "iptc", equivalent_metadata_tags[i].tag,
                            metadata_mode_conversion[equivalent_metadata_tags[i].mode].mode_string);
                  export_write_string_value (xmldata, value_utf);
                  export_write_tag_footer (xmldata, "iptc");
                  }
              g_free (value_utf);
            }
        }
      else if (default_metadata_tags[index].mode == MODE_COMBO)
        {
          gint data = get_tag_ui_combo (args, default_metadata_tags[index].tag,
                                         default_metadata_tags[index].mode);
          if (data > 0)
            {
              export_write_tag_header (xmldata, "iptc", equivalent_metadata_tags[i].tag,
                        metadata_mode_conversion[equivalent_metadata_tags[i].mode].mode_string);
              export_write_int_value (xmldata, data);
              export_write_tag_footer (xmldata, "iptc");
            }
        }
      else if (default_metadata_tags[i].mode == MODE_LIST)
        {
            /* No IPTC lists elements at this point */
        }
    }
#endif

  /* HANDLE XMP */
  for (i = 0; i < n_default_metadata_tags; i++)
    {
      if (export_ignore_tag (default_metadata_tags[i].tag))
        continue;

      if (default_metadata_tags[i].mode == MODE_SINGLE ||
          default_metadata_tags[i].mode == MODE_MULTI)
        {
          const gchar *value;

          value = get_tag_ui_text (args, default_metadata_tags[i].tag,
                                   default_metadata_tags[i].mode);

          if (value)
            {
              gchar *value_utf = NULL;

              value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
              if (strlen(value_utf) > 0)
                {
                  export_write_tag_header (xmldata, "xmp", default_metadata_tags[i].tag,
                                           metadata_mode_conversion[default_metadata_tags[i].mode].mode_string);
                  export_write_string_value (xmldata, value_utf);
                  export_write_tag_footer (xmldata, "xmp");
                }
              g_free (value_utf);
            }
        }
      else if (default_metadata_tags[i].mode == MODE_COMBO)
        {
          gint data;

          data = get_tag_ui_combo (args, default_metadata_tags[i].tag,
                                         default_metadata_tags[i].mode);
          if (data > 0)
            {
              export_write_tag_header (xmldata, "xmp", default_metadata_tags[i].tag,
                                       metadata_mode_conversion[default_metadata_tags[i].mode].mode_string);
              export_write_int_value (xmldata, data);
              export_write_tag_footer (xmldata, "xmp");
            }
        }
      else if (default_metadata_tags[i].mode == MODE_LIST)
        {
          gchar *data = NULL;

          data = get_tag_ui_list (args, default_metadata_tags[i].tag,
                                        default_metadata_tags[i].mode);

          if (data)
            {
              export_write_tag_header (xmldata, "xmp", default_metadata_tags[i].tag,
                                       metadata_mode_conversion[default_metadata_tags[i].mode].mode_string);
              export_write_string_value (xmldata, data);
              export_write_tag_footer (xmldata, "xmp");
            }
          g_free(data);
        }
    }

  g_string_append (xmldata, "</gimp-metadata>\n");


  if (args->filename != NULL)
    {
      GError *error = NULL;

      /* Write directly without pre-truncating. Pre-truncation can leave 0-byte
       * files when the final write fails for any reason (permissions, invalid
       * path, etc.). */
      if (! g_file_set_contents (args->filename, xmldata->str, xmldata->len, &error))
        {
          g_warning (_("Error saving file: %s."), error? error->message: "");
          g_clear_error (&error);
        }
    }

  if (xmldata)
    {
      g_string_free(xmldata, TRUE);
    }
}
