/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfig typedefs
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__


#include "libgimpconfig/gimpconfigtypes.h"


typedef struct _GimpBaseConfig       GimpBaseConfig;
typedef struct _GimpCoreConfig       GimpCoreConfig;
typedef struct _GimpDisplayConfig    GimpDisplayConfig;
typedef struct _GimpGuiConfig        GimpGuiConfig;
typedef struct _GimpPluginConfig     GimpPluginConfig;
typedef struct _GimpRc               GimpRc;

typedef struct _GimpXmlParser        GimpXmlParser;

/* should be in display/display-types.h */
typedef struct _GimpDisplayOptions   GimpDisplayOptions;

/* should be in core/core-types.h */
typedef struct _GimpGrid             GimpGrid;
typedef struct _GimpTemplate         GimpTemplate;


#endif /* __CONFIG_TYPES_H__ */
