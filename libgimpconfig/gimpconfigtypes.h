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

#ifndef __LIGMA_CONFIG_TYPES_H__
#define __LIGMA_CONFIG_TYPES_H__


#include <libligmabase/ligmabasetypes.h>


typedef struct _LigmaConfig        LigmaConfig; /* dummy typedef */
typedef struct _LigmaConfigWriter  LigmaConfigWriter;
typedef gchar *                   LigmaConfigPath; /* to satisfy docs */
typedef struct _GScanner          LigmaScanner;

typedef struct _LigmaColorConfig   LigmaColorConfig;

#include <libligmaconfig/ligmaconfigenums.h>


#endif  /* __LIGMA_CONFIG_TYPES_H__ */
