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

#ifndef __GIMP_CONFIG_TYPES_H__
#define __GIMP_CONFIG_TYPES_H__


#include <libgimpbase/gimpbasetypes.h>


typedef struct _GimpConfig        GimpConfig; /* dummy typedef */
typedef struct _GimpConfigWriter  GimpConfigWriter;
typedef gchar *                   GimpConfigPath; /* to satisfy docs */

typedef struct _GimpColorConfig   GimpColorConfig;

#include <libgimpconfig/gimpconfigenums.h>


#endif  /* __GIMP_CONFIG_TYPES_H__ */
