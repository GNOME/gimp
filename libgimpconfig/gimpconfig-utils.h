/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONFIG_UTILS_H__
#define __GIMP_CONFIG_UTILS_H__


void       gimp_config_connect               (GObject      *a,
                                              GObject      *b,
                                              const gchar  *property_name);
void       gimp_config_disconnect            (GObject      *a,
                                              GObject      *b);

GList    * gimp_config_diff                  (GimpConfig   *a,
                                              GimpConfig   *b,
                                              GParamFlags   flags);
gboolean   gimp_config_sync                  (GimpConfig   *src,
                                              GimpConfig   *dest,
                                              GParamFlags   flags);

void       gimp_config_reset_properties      (GimpConfig   *config);

void       gimp_config_string_append_escaped (GString      *string,
                                              const gchar  *val);

gchar    * gimp_config_build_data_path       (const gchar  *name);
gchar    * gimp_config_build_writable_path   (const gchar *name);
gchar    * gimp_config_build_plug_in_path    (const gchar  *name);

gboolean   gimp_config_file_copy             (const gchar  *source,
                                              const gchar  *dest,
                                              GError      **error);
gboolean   gimp_config_file_backup_on_error  (const gchar  *filename,
                                              const gchar  *name,
                                              GError      **error);

#endif  /* __GIMP_CONFIG_UTILS_H__ */
