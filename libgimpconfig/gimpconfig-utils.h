/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utility functions for LigmaConfig.
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

#ifndef __LIGMA_CONFIG_UTILS_H__
#define __LIGMA_CONFIG_UTILS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GList    * ligma_config_diff                  (GObject      *a,
                                              GObject      *b,
                                              GParamFlags   flags);

gboolean   ligma_config_sync                  (GObject      *src,
                                              GObject      *dest,
                                              GParamFlags   flags);

void       ligma_config_reset_properties      (GObject      *object);
void       ligma_config_reset_property        (GObject      *object,
                                              const gchar  *property_name);

void       ligma_config_string_append_escaped (GString      *string,
                                              const gchar  *val);


G_END_DECLS

#endif  /* __LIGMA_CONFIG_UTILS_H__ */
