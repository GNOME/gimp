/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscanner.h
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_CONFIG_H_INSIDE__) && !defined (GIMP_CONFIG_COMPILATION)
#error "Only <libgimpconfig/gimpconfig.h> can be included directly."
#endif

#ifndef __GIMP_SCANNER_H__
#define __GIMP_SCANNER_H__


/**
 * GIMP_TYPE_SCANNER:
 *
 * The type ID of the GIMP scanner type which is a boxed type, used to
 * read config files.
 *
 * Since: 3.0
 */
#define GIMP_TYPE_SCANNER (gimp_scnner_get_type ())


GType         gimp_scanner_get_type                 (void) G_GNUC_CONST;

GimpScanner * gimp_scanner_new_file                 (GFile         *file,
                                                     GError       **error);
GimpScanner * gimp_scanner_new_stream               (GInputStream  *input,
                                                     GError       **error);
GimpScanner * gimp_scanner_new_string               (const gchar   *text,
                                                     gint           text_len,
                                                     GError       **error);

GimpScanner * gimp_scanner_ref                      (GimpScanner   *scanner);
void          gimp_scanner_unref                    (GimpScanner   *scanner);

gboolean      gimp_scanner_parse_token              (GimpScanner   *scanner,
                                                     GTokenType     token);
gboolean      gimp_scanner_parse_identifier         (GimpScanner   *scanner,
                                                     const gchar   *identifier);
gboolean      gimp_scanner_parse_string             (GimpScanner   *scanner,
                                                     gchar        **dest);
gboolean      gimp_scanner_parse_string_no_validate (GimpScanner   *scanner,
                                                     gchar        **dest);
gboolean      gimp_scanner_parse_data               (GimpScanner   *scanner,
                                                     gint           length,
                                                     guint8       **dest);
gboolean      gimp_scanner_parse_int                (GimpScanner   *scanner,
                                                     gint          *dest);
gboolean      gimp_scanner_parse_int64              (GimpScanner   *scanner,
                                                     gint64        *dest);
gboolean      gimp_scanner_parse_double             (GimpScanner   *scanner,
                                                     gdouble       *dest);
gboolean      gimp_scanner_parse_boolean            (GimpScanner   *scanner,
                                                     gboolean      *dest);
gboolean      gimp_scanner_parse_color              (GimpScanner   *scanner,
                                                     GeglColor    **color);
gboolean      gimp_scanner_parse_matrix2            (GimpScanner   *scanner,
                                                     GimpMatrix2   *dest);


#endif /* __GIMP_SCANNER_H__ */
