/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONFIG_IFACE_H__
#define __GIMP_CONFIG_IFACE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_CONFIG               (gimp_config_get_type ())
#define GIMP_IS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONFIG))
#define GIMP_CONFIG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONFIG, GimpConfig))
#define GIMP_CONFIG_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_CONFIG, GimpConfigInterface))


typedef struct _GimpConfigInterface GimpConfigInterface;

struct _GimpConfigInterface
{
  GTypeInterface base_iface;

  gboolean     (* serialize)            (GimpConfig       *config,
                                         GimpConfigWriter *writer,
                                         gpointer          data);
  gboolean     (* deserialize)          (GimpConfig       *config,
                                         GScanner         *scanner,
                                         gint              nest_level,
                                         gpointer          data);
  gboolean     (* serialize_property)   (GimpConfig       *config,
                                         guint             property_id,
                                         const GValue     *value,
                                         GParamSpec       *pspec,
                                         GimpConfigWriter *writer);
  gboolean     (* deserialize_property) (GimpConfig       *config,
                                         guint             property_id,
                                         GValue           *value,
                                         GParamSpec       *pspec,
                                         GScanner         *scanner,
                                         GTokenType       *expected);
  GimpConfig * (* duplicate)            (GimpConfig       *config);
  gboolean     (* equal)                (GimpConfig       *a,
                                         GimpConfig       *b);
  void         (* reset)                (GimpConfig       *config);
  gboolean     (* copy)                 (GimpConfig       *src,
                                         GimpConfig       *dest,
                                         GParamFlags       flags);
};


GType      gimp_config_get_type            (void) G_GNUC_CONST;

GIMP_DEPRECATED_FOR (gimp_config_get_type)
GType      gimp_config_interface_get_type  (void) G_GNUC_CONST;

gboolean   gimp_config_serialize_to_file   (GimpConfig       *config,
                                            const gchar      *filename,
                                            const gchar      *header,
                                            const gchar      *footer,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_serialize_to_gfile  (GimpConfig       *config,
                                            GFile            *file,
                                            const gchar      *header,
                                            const gchar      *footer,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_serialize_to_stream (GimpConfig       *config,
                                            GOutputStream    *output,
                                            const gchar      *header,
                                            const gchar      *footer,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_serialize_to_fd     (GimpConfig       *config,
                                            gint              fd,
                                            gpointer          data);
gchar    * gimp_config_serialize_to_string (GimpConfig       *config,
                                            gpointer          data);
gboolean   gimp_config_deserialize_file    (GimpConfig       *config,
                                            const gchar      *filename,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_deserialize_gfile   (GimpConfig       *config,
                                            GFile            *file,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_deserialize_stream  (GimpConfig       *config,
                                            GInputStream     *input,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_deserialize_string  (GimpConfig       *config,
                                            const gchar      *text,
                                            gint              text_len,
                                            gpointer          data,
                                            GError          **error);
gboolean   gimp_config_deserialize_return  (GScanner         *scanner,
                                            GTokenType        expected_token,
                                            gint              nest_level);

gboolean   gimp_config_serialize           (GimpConfig       *config,
                                            GimpConfigWriter *writer,
                                            gpointer          data);
gboolean   gimp_config_deserialize         (GimpConfig       *config,
                                            GScanner         *scanner,
                                            gint              nest_level,
                                            gpointer          data);
gpointer   gimp_config_duplicate           (GimpConfig       *config);
gboolean   gimp_config_is_equal_to         (GimpConfig       *a,
                                            GimpConfig       *b);
void       gimp_config_reset               (GimpConfig       *config);
gboolean   gimp_config_copy                (GimpConfig       *src,
                                            GimpConfig       *dest,
                                            GParamFlags       flags);


G_END_DECLS

#endif  /* __GIMP_CONFIG_IFACE_H__ */
