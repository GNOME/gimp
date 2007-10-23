/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscanner.h
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_SCANNER_H__
#define __GIMP_SCANNER_H__


GScanner * gimp_scanner_new_file                 (const gchar  *filename,
                                                  GError      **error);
GScanner * gimp_scanner_new_string               (const gchar  *text,
                                                  gint          text_len,
                                                  GError      **error);
void       gimp_scanner_destroy                  (GScanner     *scanner);

gboolean   gimp_scanner_parse_token              (GScanner     *scanner,
                                                  GTokenType    token);
gboolean   gimp_scanner_parse_identifier         (GScanner     *scanner,
                                                  const gchar  *identifier);
gboolean   gimp_scanner_parse_string             (GScanner     *scanner,
                                                  gchar       **dest);
gboolean   gimp_scanner_parse_string_no_validate (GScanner     *scanner,
                                                  gchar       **dest);
gboolean   gimp_scanner_parse_data               (GScanner     *scanner,
                                                  gint          length,
                                                  guint8      **dest);
gboolean   gimp_scanner_parse_int                (GScanner     *scanner,
                                                  gint         *dest);
gboolean   gimp_scanner_parse_float              (GScanner     *scanner,
                                                  gdouble      *dest);
gboolean   gimp_scanner_parse_color              (GScanner     *scanner,
                                                  GimpRGB      *dest);
gboolean   gimp_scanner_parse_matrix2            (GScanner     *scanner,
                                                  GimpMatrix2  *dest);


#endif /* __GIMP_SCANNER_H__ */
