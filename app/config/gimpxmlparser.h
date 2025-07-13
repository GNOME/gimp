/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpXmlParser
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

#pragma once


GimpXmlParser * gimp_xml_parser_new              (const GMarkupParser *markup_parser,
                                                  gpointer             user_data);
gboolean        gimp_xml_parser_parse_file       (GimpXmlParser       *parser,
                                                  const gchar         *filename,
                                                  GError             **error);
gboolean        gimp_xml_parser_parse_gfile      (GimpXmlParser       *parser,
                                                  GFile               *file,
                                                  GError             **error);
gboolean        gimp_xml_parser_parse_fd         (GimpXmlParser       *parser,
                                                  gint                 fd,
                                                  GError             **error);
gboolean        gimp_xml_parser_parse_io_channel (GimpXmlParser       *parser,
                                                  GIOChannel          *io,
                                                  GError             **error);
gboolean        gimp_xml_parser_parse_buffer     (GimpXmlParser       *parser,
                                                  const gchar         *buffer,
                                                  gssize               len,
                                                  GError             **error);
void            gimp_xml_parser_free             (GimpXmlParser       *parser);
