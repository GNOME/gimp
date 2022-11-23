/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONFIG_WRITER_H__
#define __LIGMA_CONFIG_WRITER_H__


/**
 * LIGMA_TYPE_CONFIG_WRITER:
 *
 * The type ID of the "LigmaConfigWriter" type which is a boxed type,
 * used to write config files.
 *
 * Since: 3.0
 */
#define LIGMA_TYPE_CONFIG_WRITER (ligma_config_writer_get_type ())


GType              ligma_config_writer_get_type     (void) G_GNUC_CONST;

LigmaConfigWriter * ligma_config_writer_new_from_file     (GFile             *file,
                                                         gboolean           atomic,
                                                         const gchar       *header,
                                                         GError           **error);
LigmaConfigWriter * ligma_config_writer_new_from_stream   (GOutputStream     *output,
                                                         const gchar       *header,
                                                         GError           **error);
LigmaConfigWriter * ligma_config_writer_new_from_fd       (gint               fd);
LigmaConfigWriter * ligma_config_writer_new_from_string   (GString           *string);

LigmaConfigWriter * ligma_config_writer_ref          (LigmaConfigWriter  *writer);
void               ligma_config_writer_unref        (LigmaConfigWriter  *writer);

void               ligma_config_writer_open         (LigmaConfigWriter  *writer,
                                                    const gchar       *name);
void               ligma_config_writer_comment_mode (LigmaConfigWriter  *writer,
                                                    gboolean           enable);

void               ligma_config_writer_print        (LigmaConfigWriter  *writer,
                                                    const gchar       *string,
                                                    gint               len);
void               ligma_config_writer_printf       (LigmaConfigWriter  *writer,
                                                    const gchar       *format,
                                                    ...) G_GNUC_PRINTF (2, 3);
void               ligma_config_writer_identifier   (LigmaConfigWriter  *writer,
                                                    const gchar       *identifier);
void               ligma_config_writer_string       (LigmaConfigWriter  *writer,
                                                    const gchar       *string);
void               ligma_config_writer_data         (LigmaConfigWriter  *writer,
                                                    gint               length,
                                                    const guint8      *data);
void               ligma_config_writer_comment      (LigmaConfigWriter  *writer,
                                                    const gchar       *comment);
void               ligma_config_writer_linefeed     (LigmaConfigWriter  *writer);


void               ligma_config_writer_revert       (LigmaConfigWriter  *writer);
void               ligma_config_writer_close        (LigmaConfigWriter  *writer);
gboolean           ligma_config_writer_finish       (LigmaConfigWriter  *writer,
                                                    const gchar       *footer,
                                                    GError           **error);


#endif /* __LIGMA_CONFIG_WRITER_H__ */
