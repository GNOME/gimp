/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONFIG_IFACE_H__
#define __LIGMA_CONFIG_IFACE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#define LIGMA_TYPE_CONFIG (ligma_config_get_type ())
G_DECLARE_INTERFACE (LigmaConfig, ligma_config, LIGMA, CONFIG, GObject)

struct _LigmaConfigInterface
{
  GTypeInterface base_iface;

  gboolean     (* serialize)            (LigmaConfig       *config,
                                         LigmaConfigWriter *writer,
                                         gpointer          data);
  gboolean     (* deserialize)          (LigmaConfig       *config,
                                         GScanner         *scanner,
                                         gint              nest_level,
                                         gpointer          data);
  gboolean     (* serialize_property)   (LigmaConfig       *config,
                                         guint             property_id,
                                         const GValue     *value,
                                         GParamSpec       *pspec,
                                         LigmaConfigWriter *writer);
  gboolean     (* deserialize_property) (LigmaConfig       *config,
                                         guint             property_id,
                                         GValue           *value,
                                         GParamSpec       *pspec,
                                         GScanner         *scanner,
                                         GTokenType       *expected);
  LigmaConfig * (* duplicate)            (LigmaConfig       *config);
  gboolean     (* equal)                (LigmaConfig       *a,
                                         LigmaConfig       *b);
  void         (* reset)                (LigmaConfig       *config);
  gboolean     (* copy)                 (LigmaConfig       *src,
                                         LigmaConfig       *dest,
                                         GParamFlags       flags);
};


gboolean   ligma_config_serialize_to_file     (LigmaConfig          *config,
                                              GFile               *file,
                                              const gchar         *header,
                                              const gchar         *footer,
                                              gpointer             data,
                                              GError             **error);
gboolean   ligma_config_serialize_to_stream   (LigmaConfig          *config,
                                              GOutputStream       *output,
                                              const gchar         *header,
                                              const gchar         *footer,
                                              gpointer             data,
                                              GError             **error);
gboolean   ligma_config_serialize_to_fd       (LigmaConfig          *config,
                                              gint                 fd,
                                              gpointer             data);
gchar    * ligma_config_serialize_to_string   (LigmaConfig          *config,
                                              gpointer             data);
LigmaParasite *
           ligma_config_serialize_to_parasite (LigmaConfig          *config,
                                              const gchar         *parasite_name,
                                              guint                parasite_flags,
                                              gpointer             data);

gboolean   ligma_config_deserialize_file      (LigmaConfig          *config,
                                              GFile               *file,
                                              gpointer             data,
                                              GError             **error);
gboolean   ligma_config_deserialize_stream    (LigmaConfig          *config,
                                              GInputStream        *input,
                                              gpointer             data,
                                              GError             **error);
gboolean   ligma_config_deserialize_string    (LigmaConfig          *config,
                                              const gchar         *text,
                                              gint                 text_len,
                                              gpointer             data,
                                              GError             **error);
gboolean   ligma_config_deserialize_parasite  (LigmaConfig          *config,
                                              const LigmaParasite  *parasite,
                                              gpointer             data,
                                              GError             **error);

gboolean   ligma_config_deserialize_return    (GScanner            *scanner,
                                              GTokenType           expected_token,
                                              gint                 nest_level);

gboolean   ligma_config_serialize             (LigmaConfig          *config,
                                              LigmaConfigWriter    *writer,
                                              gpointer             data);
gboolean   ligma_config_deserialize           (LigmaConfig          *config,
                                              GScanner            *scanner,
                                              gint                 nest_level,
                                              gpointer             data);
gpointer   ligma_config_duplicate             (LigmaConfig          *config);
gboolean   ligma_config_is_equal_to           (LigmaConfig          *a,
                                              LigmaConfig          *b);
void       ligma_config_reset                 (LigmaConfig          *config);
gboolean   ligma_config_copy                  (LigmaConfig          *src,
                                              LigmaConfig          *dest,
                                              GParamFlags          flags);


G_END_DECLS

#endif  /* __LIGMA_CONFIG_IFACE_H__ */
