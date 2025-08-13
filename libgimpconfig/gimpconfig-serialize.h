/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Object properties serialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONFIG_SERIALIZE_H__
#define __GIMP_CONFIG_SERIALIZE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean  gimp_config_serialize_properties         (GimpConfig       *config,
                                                    GimpConfigWriter *writer);
gboolean  gimp_config_serialize_changed_properties (GimpConfig       *config,
                                                    GimpConfigWriter *writer);

gboolean  gimp_config_serialize_property           (GimpConfig       *config,
                                                    GParamSpec       *param_spec,
                                                    GimpConfigWriter *writer);
gboolean  gimp_config_serialize_property_by_name   (GimpConfig       *config,
                                                    const gchar      *prop_name,
                                                    GimpConfigWriter *writer);
gboolean  gimp_config_serialize_value              (const GValue     *value,
                                                    GString          *str,
                                                    gboolean          escaped);


G_END_DECLS

#endif /* __GIMP_CONFIG_SERIALIZE_H__ */
