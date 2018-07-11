/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONFIG_WRITER_H__
#define __GIMP_CONFIG_WRITER_H__


GimpConfigWriter * gimp_config_writer_new_file     (const gchar       *filename,
                                                    gboolean           atomic,
                                                    const gchar       *header,
                                                    GError           **error);
GimpConfigWriter * gimp_config_writer_new_gfile    (GFile             *file,
                                                    gboolean           atomic,
                                                    const gchar       *header,
                                                    GError           **error);
GimpConfigWriter * gimp_config_writer_new_stream   (GOutputStream     *output,
                                                    const gchar       *header,
                                                    GError           **error);
GimpConfigWriter * gimp_config_writer_new_fd       (gint               fd);
GimpConfigWriter * gimp_config_writer_new_string   (GString           *string);

void               gimp_config_writer_open         (GimpConfigWriter  *writer,
                                                    const gchar       *name);
void               gimp_config_writer_comment_mode (GimpConfigWriter  *writer,
                                                    gboolean           enable);

void               gimp_config_writer_print        (GimpConfigWriter  *writer,
                                                    const gchar       *string,
                                                    gint               len);
void               gimp_config_writer_printf       (GimpConfigWriter  *writer,
                                                    const gchar       *format,
                                                    ...) G_GNUC_PRINTF (2, 3);
void               gimp_config_writer_identifier   (GimpConfigWriter  *writer,
                                                    const gchar       *identifier);
void               gimp_config_writer_string       (GimpConfigWriter  *writer,
                                                    const gchar       *string);
void               gimp_config_writer_data         (GimpConfigWriter  *writer,
                                                    gint               length,
                                                    const guint8      *data);
void               gimp_config_writer_comment      (GimpConfigWriter  *writer,
                                                    const gchar       *comment);
void               gimp_config_writer_linefeed     (GimpConfigWriter  *writer);


void               gimp_config_writer_revert       (GimpConfigWriter  *writer);
void               gimp_config_writer_close        (GimpConfigWriter  *writer);
gboolean           gimp_config_writer_finish       (GimpConfigWriter  *writer,
                                                    const gchar       *footer,
                                                    GError           **error);


#endif /* __GIMP_CONFIG_WRITER_H__ */
