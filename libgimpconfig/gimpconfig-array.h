/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpconfig-array.h
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

#ifndef __GIMP_CONFIG_ARRAY_H__
#define __GIMP_CONFIG_ARRAY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/* Functions to ser/des arrays. */

/* FIXME: move gimp_config_serialize_value_array here.
 * FIXME: implement other arrays for which plugins can declare args e.g. int32 array.
 * FIXME: doesn't need to be introspected, these are internal
 */

gboolean    gimp_config_serialize_strv   (const GValue *value,
                                          GString      *str);
GTokenType  gimp_config_deserialize_strv (GValue       *value,
                                          GScanner     *scanner);

G_END_DECLS

#endif /* __GIMP_CONFIG_ARRAY_H__ */
