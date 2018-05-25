/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptypes.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TYPES_H__
#define __GIMP_TYPES_H__

#include <libgimpbase/gimpbasetypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpPlugInInfo  GimpPlugInInfo;
typedef struct _GimpTile        GimpTile;
typedef struct _GimpDrawable    GimpDrawable;
typedef struct _GimpPixelRgn    GimpPixelRgn;
typedef struct _GimpParamDef    GimpParamDef;
typedef struct _GimpParamRegion GimpParamRegion;
typedef union  _GimpParamData   GimpParamData;
typedef struct _GimpParam       GimpParam;


G_END_DECLS

#endif /* __GIMP_TYPES_H__ */
