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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TYPES_H__
#define __GIMP_TYPES_H__

#include <libgimpbase/gimpbasetypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpPDB                 GimpPDB;
typedef struct _GimpPlugIn              GimpPlugIn;
typedef struct _GimpProcedure           GimpProcedure;
typedef struct _GimpBatchProcedure      GimpBatchProcedure;
typedef struct _GimpImageProcedure      GimpImageProcedure;
typedef struct _GimpFileProcedure       GimpFileProcedure;
typedef struct _GimpVectorLoadProcedure GimpVectorLoadProcedure;
typedef struct _GimpLoadProcedure       GimpLoadProcedure;
typedef struct _GimpExportProcedure     GimpExportProcedure;
typedef struct _GimpThumbnailProcedure  GimpThumbnailProcedure;
typedef struct _GimpProcedureConfig     GimpProcedureConfig;

typedef struct _GimpImage               GimpImage;
typedef struct _GimpItem                GimpItem;
typedef struct _GimpDrawable            GimpDrawable;
typedef struct _GimpGroupLayer          GimpGroupLayer;
typedef struct _GimpLayer               GimpLayer;
typedef struct _GimpChannel             GimpChannel;
typedef struct _GimpLayerMask           GimpLayerMask;
typedef struct _GimpSelection           GimpSelection;
typedef struct _GimpTextLayer           GimpTextLayer;
typedef struct _GimpPath                GimpPath;

typedef struct _GimpDisplay             GimpDisplay;

typedef struct _GimpResource            GimpResource;
typedef struct _GimpBrush               GimpBrush;
typedef struct _GimpFont                GimpFont;
typedef struct _GimpGradient            GimpGradient;
typedef struct _GimpPattern             GimpPattern;
typedef struct _GimpPalette             GimpPalette;


/* FIXME move somewhere else */

/**
 * GimpPixbufTransparency:
 * @GIMP_PIXBUF_KEEP_ALPHA:   Create a pixbuf with alpha
 * @GIMP_PIXBUF_SMALL_CHECKS: Show transparency as small checks
 * @GIMP_PIXBUF_LARGE_CHECKS: Show transparency as large checks
 *
 * How to deal with transparency when creating thubnail pixbufs from
 * images and drawables.
 **/
typedef enum
{
  GIMP_PIXBUF_KEEP_ALPHA,
  GIMP_PIXBUF_SMALL_CHECKS,
  GIMP_PIXBUF_LARGE_CHECKS
} GimpPixbufTransparency;


G_END_DECLS

#endif /* __GIMP_TYPES_H__ */
