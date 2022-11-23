/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmatypes.h
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

#ifndef __LIGMA_TYPES_H__
#define __LIGMA_TYPES_H__

#include <libligmabase/ligmabasetypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _LigmaPDB             LigmaPDB;
typedef struct _LigmaPlugIn          LigmaPlugIn;
typedef struct _LigmaProcedure       LigmaProcedure;
typedef struct _LigmaProcedureConfig LigmaProcedureConfig;

typedef struct _LigmaImage           LigmaImage;
typedef struct _LigmaItem            LigmaItem;
typedef struct _LigmaDrawable        LigmaDrawable;
typedef struct _LigmaLayer           LigmaLayer;
typedef struct _LigmaChannel         LigmaChannel;
typedef struct _LigmaLayerMask       LigmaLayerMask;
typedef struct _LigmaSelection       LigmaSelection;
typedef struct _LigmaTextLayer       LigmaTextLayer;
typedef struct _LigmaVectors         LigmaVectors;

typedef struct _LigmaDisplay         LigmaDisplay;


/* FIXME move somewhere else */

/**
 * LigmaPixbufTransparency:
 * @LIGMA_PIXBUF_KEEP_ALPHA:   Create a pixbuf with alpha
 * @LIGMA_PIXBUF_SMALL_CHECKS: Show transparency as small checks
 * @LIGMA_PIXBUF_LARGE_CHECKS: Show transparency as large checks
 *
 * How to deal with transparency when creating thubnail pixbufs from
 * images and drawables.
 **/
typedef enum
{
  LIGMA_PIXBUF_KEEP_ALPHA,
  LIGMA_PIXBUF_SMALL_CHECKS,
  LIGMA_PIXBUF_LARGE_CHECKS
} LigmaPixbufTransparency;


G_END_DECLS

#endif /* __LIGMA_TYPES_H__ */
