/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * LigmaXmlParser
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

#ifndef __LIGMA_XML_PARSER_H__
#define __LIGMA_XML_PARSER_H__


LigmaXmlParser * ligma_xml_parser_new              (const GMarkupParser *markup_parser,
                                                  gpointer             user_data);
gboolean        ligma_xml_parser_parse_file       (LigmaXmlParser       *parser,
                                                  const gchar         *filename,
                                                  GError             **error);
gboolean        ligma_xml_parser_parse_gfile      (LigmaXmlParser       *parser,
                                                  GFile               *file,
                                                  GError             **error);
gboolean        ligma_xml_parser_parse_fd         (LigmaXmlParser       *parser,
                                                  gint                 fd,
                                                  GError             **error);
gboolean        ligma_xml_parser_parse_io_channel (LigmaXmlParser       *parser,
                                                  GIOChannel          *io,
                                                  GError             **error);
gboolean        ligma_xml_parser_parse_buffer     (LigmaXmlParser       *parser,
                                                  const gchar         *buffer,
                                                  gssize               len,
                                                  GError             **error);
void            ligma_xml_parser_free             (LigmaXmlParser       *parser);


#endif  /* __LIGMA_XML_PARSER_H__ */
