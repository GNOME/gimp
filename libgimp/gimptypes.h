/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptypes.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_TYPES_H__
#define __GIMP_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the html documentation */


typedef struct _GimpPlugInInfo  GimpPlugInInfo;
typedef struct _GimpTile        GimpTile;
typedef struct _GimpDrawable    GimpDrawable;
typedef struct _GimpPixelRgn    GimpPixelRgn;
typedef struct _GimpParamDef    GimpParamDef;
typedef struct _GimpParamColor  GimpParamColor;
typedef struct _GimpParamRegion GimpParamRegion;
typedef union  _GimpParamData   GimpParamData;
typedef struct _GimpParam       GimpParam;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_TYPES_H__ */
