/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
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

#ifndef __GIMP_CONFIG_H__
#define __GIMP_CONFIG_H__


#define GIMP_TYPE_CONFIG               (gimp_config_interface_get_type ())
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
};

typedef void  (* GimpConfigForeachFunc) (const gchar *key,
                                         const gchar *value,
                                         gpointer     user_data);


GType         gimp_config_interface_get_type    (void) G_GNUC_CONST;

gboolean      gimp_config_serialize_to_file     (GimpConfig   *config,
                                                 const gchar  *filename,
                                                 const gchar  *header,
                                                 const gchar  *footer,
                                                 gpointer      data,
                                                 GError      **error);
gboolean      gimp_config_serialize_to_fd       (GimpConfig   *config,
                                                 gint          fd,
                                                 gpointer      data);
gchar       * gimp_config_serialize_to_string   (GimpConfig   *config,
						 gpointer      data);
gboolean      gimp_config_deserialize_file      (GimpConfig   *config,
                                                 const gchar  *filename,
                                                 gpointer      data,
                                                 GError      **error);
gboolean      gimp_config_deserialize_string    (GimpConfig   *config,
                                                 const gchar  *text,
                                                 gint          text_len,
                                                 gpointer      data,
                                                 GError      **error);
gboolean      gimp_config_deserialize_return    (GScanner     *scanner,
                                                 GTokenType    expected_token,
                                                 gint          nest_level);

gpointer      gimp_config_duplicate             (GimpConfig   *config);
gboolean      gimp_config_is_equal_to           (GimpConfig   *a,
                                                 GimpConfig   *b);
void          gimp_config_reset                 (GimpConfig   *config);

void          gimp_config_add_unknown_token     (GimpConfig   *config,
                                                 const gchar  *key,
                                                 const gchar  *value);
const gchar * gimp_config_lookup_unknown_token  (GimpConfig   *config,
                                                 const gchar  *key);
void          gimp_config_foreach_unknown_token (GimpConfig   *config,
                                                 GimpConfigForeachFunc  func,
                                                 gpointer      user_data);


#endif  /* __GIMP_CONFIG_H__ */
