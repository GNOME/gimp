/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmascanner.h
 * Copyright (C) 2002  Sven Neumann <sven@ligma.org>
 *                     Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_CONFIG_H_INSIDE__) && !defined (LIGMA_CONFIG_COMPILATION)
#error "Only <libligmaconfig/ligmaconfig.h> can be included directly."
#endif

#ifndef __LIGMA_SCANNER_H__
#define __LIGMA_SCANNER_H__


/**
 * LIGMA_TYPE_SCANNER:
 *
 * The type ID of the LIGMA scanner type which is a boxed type, used to
 * read config files.
 *
 * Since: 3.0
 */
#define LIGMA_TYPE_SCANNER (ligma_scnner_get_type ())


GType         ligma_scanner_get_type                 (void) G_GNUC_CONST;

LigmaScanner * ligma_scanner_new_file                 (GFile         *file,
                                                     GError       **error);
LigmaScanner * ligma_scanner_new_stream               (GInputStream  *input,
                                                     GError       **error);
LigmaScanner * ligma_scanner_new_string               (const gchar   *text,
                                                     gint           text_len,
                                                     GError       **error);

LigmaScanner * ligma_scanner_ref                      (LigmaScanner   *scanner);
void          ligma_scanner_unref                    (LigmaScanner   *scanner);

gboolean      ligma_scanner_parse_token              (LigmaScanner   *scanner,
                                                     GTokenType     token);
gboolean      ligma_scanner_parse_identifier         (LigmaScanner   *scanner,
                                                     const gchar   *identifier);
gboolean      ligma_scanner_parse_string             (LigmaScanner   *scanner,
                                                     gchar        **dest);
gboolean      ligma_scanner_parse_string_no_validate (LigmaScanner   *scanner,
                                                     gchar        **dest);
gboolean      ligma_scanner_parse_data               (LigmaScanner   *scanner,
                                                     gint           length,
                                                     guint8       **dest);
gboolean      ligma_scanner_parse_int                (LigmaScanner   *scanner,
                                                     gint          *dest);
gboolean      ligma_scanner_parse_int64              (LigmaScanner   *scanner,
                                                     gint64        *dest);
gboolean      ligma_scanner_parse_float              (LigmaScanner   *scanner,
                                                     gdouble       *dest);
gboolean      ligma_scanner_parse_boolean            (LigmaScanner   *scanner,
                                                     gboolean      *dest);
gboolean      ligma_scanner_parse_color              (LigmaScanner   *scanner,
                                                     LigmaRGB       *dest);
gboolean      ligma_scanner_parse_matrix2            (LigmaScanner   *scanner,
                                                     LigmaMatrix2   *dest);


#endif /* __LIGMA_SCANNER_H__ */
