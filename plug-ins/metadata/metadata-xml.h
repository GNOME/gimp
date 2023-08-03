/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
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

#ifndef __METADATA_XML_H__
#define __METADATA_XML_H__

#include "metadata-misc.h"

struct _GimpXmlParser
{
  GMarkupParseContext *context;
};

typedef struct _GimpXmlParser GimpXmlParser;

void
xml_parser_start_element                     (GMarkupParseContext   *context,
                                              const gchar           *element_name,
                                              const gchar          **attribute_names,
                                              const gchar          **attribute_values,
                                              gpointer               user_data,
                                              GError               **error);

void
xml_parser_data                              (GMarkupParseContext  *context,
                                              const gchar          *text,
                                              gsize                 text_len,
                                              gpointer              user_data,
                                              GError              **error);

void
set_tag_ui                                   (metadata_editor      *args,
                                              gint                  index,
                                              gchar                *name,
                                              gchar                *value,
                                              MetadataMode          mode);

const gchar *
get_tag_ui_text                              (metadata_editor      *args,
                                              gchar                *name,
                                              MetadataMode          mode);

gchar *
get_tag_ui_list                              (metadata_editor      *args,
                                              gchar                *name,
                                              MetadataMode          mode);

gint
get_tag_ui_combo                             (metadata_editor      *args,
                                              gchar                *name,
                                              MetadataMode          mode);

void
xml_parser_end_element                       (GMarkupParseContext  *context,
                                              const gchar          *element_name,
                                              gpointer              user_data,
                                              GError              **error);

gboolean
xml_parser_parse_file                        (GimpXmlParser        *parser,
                                              const gchar          *filename,
                                              GError              **error);

void
xml_parser_free                              (GimpXmlParser        *parser);

gboolean
parse_encoding                               (const gchar          *text,
                                              gint                  text_len,
                                              gchar               **encoding);

gboolean
xml_parser_parse_io_channel                  (GimpXmlParser        *parser,
                                              GIOChannel           *io,
                                              GError              **error);

GimpXmlParser *
xml_parser_new                               (const GMarkupParser  *markup_parser,
                                              gpointer              user_data);

#endif /* __METADATA_XML_H__ */

