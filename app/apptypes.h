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

#ifndef __APPTYPES_H__
#define __APPTYPES_H__


#include "libgimpcolor/gimpcolortypes.h"
#include "libgimpmath/gimpmathtypes.h"
#include "libgimpbase/gimpbasetypes.h"

#include "base/base-types.h"

#include "undo_types.h"

#include "appenums.h"


/*  other stuff  */

typedef struct _Argument            Argument;

typedef struct _ColorNotebook       ColorNotebook;

typedef struct _GDisplay            GDisplay;

typedef struct _GimpImageNewValues  GimpImageNewValues;

typedef struct _GimpProgress        GimpProgress;

typedef struct _ImageMap            ImageMap;

typedef struct _InfoDialog          InfoDialog;

typedef struct _NavigationDialog    NavigationDialog;

typedef struct _Path                Path;
typedef struct _PathPoint           PathPoint;
typedef struct _PathList            PathList;

typedef struct _PlugIn              PlugIn;
typedef struct _PlugInDef           PlugInDef;
typedef struct _PlugInProcDef       PlugInProcDef;

typedef struct _ParasiteList        ParasiteList;

typedef struct _ProcArg             ProcArg;
typedef struct _ProcRecord          ProcRecord;

typedef         guint32             Tattoo;

typedef struct _Selection           Selection;


/*  some undo stuff  */

typedef struct _LayerUndo           LayerUndo;

typedef struct _LayerMaskUndo       LayerMaskUndo;

typedef struct _FStoLayerUndo       FStoLayerUndo;

typedef         GSList              PathUndo;


/*  functions  */

typedef void   (* ImageMapApplyFunc) (PixelRegion *srcPR,
				      PixelRegion *destPR,
				      gpointer     data);


#endif /* __APPTYPES_H__ */
