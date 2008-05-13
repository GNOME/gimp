/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * vectors-types.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#ifndef __VECTORS_TYPES_H__
#define __VECTORS_TYPES_H__

#include "core/core-types.h"

#include "vectors/vectors-enums.h"

typedef struct _GimpAnchor          GimpAnchor;

typedef struct _GimpVectors         GimpVectors;
typedef struct _GimpVectorsUndo     GimpVectorsUndo;
typedef struct _GimpVectorsModUndo  GimpVectorsModUndo;
typedef struct _GimpVectorsPropUndo GimpVectorsPropUndo;
typedef struct _GimpStroke          GimpStroke;
typedef struct _GimpBezierStroke    GimpBezierStroke;

/*
 * The following hack is made so that we can reuse the definition
 * the cairo definition of cairo_path_t without having to translate
 * between our own version of a bezier description and cairos version.
 *
 * to avoid having to include <cairo/cairo.h> in each and every file
 * including this file we only use the "real" definition when cairo.h
 * already has been included and use something else.
 *
 * Note that if you really want to work with GimpBezierDesc (except just
 * passing pointers to it around) you also need to include <cairo/cairo.h>.
 */

#ifdef CAIRO_VERSION
typedef cairo_path_t GimpBezierDesc;
#else
typedef void * GimpBezierDesc;
#endif

#endif /* __VECTORS_TYPES_H__ */
