/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __CORE_TYPES_H__
#define __CORE_TYPES_H__


#include "apptypes.h"


/*  base objects  */

typedef struct _GimpObject          GimpObject;

typedef struct _GimpContainer       GimpContainer;
typedef struct _GimpList            GimpList;
typedef struct _GimpDataList        GimpDataList;

typedef struct _GimpDataFactory     GimpDataFactory;

typedef struct _GimpContext         GimpContext;

typedef struct _GimpViewable        GimpViewable;

typedef struct _GimpToolInfo        GimpToolInfo;


/*  drawable objects  */

typedef struct _GimpDrawable        GimpDrawable;

typedef struct _GimpChannel         GimpChannel;

typedef struct _GimpLayer           GimpLayer;
typedef struct _GimpLayerMask       GimpLayerMask;

typedef struct _GimpImage           GimpImage;


/*  data objects  */

typedef struct _GimpData            GimpData;

typedef struct _GimpBrush	    GimpBrush;
typedef struct _GimpBrushGenerated  GimpBrushGenerated;
typedef struct _GimpBrushPipe       GimpBrushPipe;

typedef struct _GimpGradient        GimpGradient;

typedef struct _GimpPattern         GimpPattern;

typedef struct _GimpPalette         GimpPalette;


/*  undo objects  */

typedef struct _GimpUndo            GimpUndo;
typedef struct _GimpUndoStack       GimpUndoStack;


/*  functions  */

typedef void       (* GimpDataFileLoaderFunc)   (const gchar *filename,
						 gpointer     loader_data);
typedef GimpData * (* GimpDataObjectLoaderFunc) (const gchar *filename);


#endif /* __CORE_TYPES_H__ */
