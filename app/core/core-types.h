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


#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmoduletypes.h"
#include "libgimpthumb/gimpthumb-types.h"

#include "base/base-types.h"

#include "core/core-enums.h"


/*  defines  */

#define GIMP_OPACITY_TRANSPARENT     0.0
#define GIMP_OPACITY_OPAQUE          1.0

#define GIMP_COORDS_DEFAULT_PRESSURE 1.0
#define GIMP_COORDS_DEFAULT_TILT     0.0
#define GIMP_COORDS_DEFAULT_WHEEL    0.5


/*  enums  */

typedef enum
{
  GIMP_PIXELS,
  GIMP_POINTS
} SizeType;


/*  base objects  */

typedef struct _GimpObject          GimpObject;

typedef struct _Gimp                Gimp;

typedef struct _GimpParasiteList    GimpParasiteList;

typedef struct _GimpContainer       GimpContainer;
typedef struct _GimpList            GimpList;

typedef struct _GimpDataFactory     GimpDataFactory;

typedef struct _GimpContext         GimpContext;

typedef struct _GimpViewable        GimpViewable;
typedef struct _GimpItem            GimpItem;

typedef struct _GimpBuffer          GimpBuffer;

typedef struct _GimpPaintInfo       GimpPaintInfo;
typedef struct _GimpStrokeOptions   GimpStrokeOptions;
typedef struct _GimpToolInfo        GimpToolInfo;
typedef struct _GimpToolOptions     GimpToolOptions;

typedef struct _GimpImagefile       GimpImagefile;
typedef struct _GimpDocumentList    GimpDocumentList;

/*  drawable objects  */

typedef struct _GimpDrawable        GimpDrawable;

typedef struct _GimpChannel         GimpChannel;
typedef struct _GimpSelection       GimpSelection;

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


/*  misc objects  */

typedef struct _GimpImageMap        GimpImageMap;

typedef struct _GimpEnvironTable    GimpEnvironTable;


/*  undo objects  */

typedef struct _GimpUndo            GimpUndo;
typedef struct _GimpItemUndo        GimpItemUndo;
typedef struct _GimpUndoStack       GimpUndoStack;
typedef struct _GimpUndoAccumulator GimpUndoAccumulator;


/*  non-object types  */

typedef struct _GimpCoords          GimpCoords;
typedef struct _GimpGradientSegment GimpGradientSegment;
typedef struct _GimpGuide           GimpGuide;
typedef struct _GimpProgress        GimpProgress;
typedef         guint32             GimpTattoo;
typedef struct _GimpPaletteEntry    GimpPaletteEntry;
typedef struct _GimpPlugInDebug     GimpPlugInDebug;
typedef struct _GimpScanConvert     GimpScanConvert;


/*  functions  */

typedef void       (* GimpInitStatusFunc)       (const gchar         *text1,
                                                 const gchar         *text2,
                                                 gdouble              percentage);

typedef GimpData * (* GimpDataObjectLoaderFunc) (const gchar         *filename);

typedef gboolean   (* GimpObjectFilterFunc)     (const GimpObject    *object,
                                                 gpointer             user_data);

typedef gboolean   (* GimpUndoPopFunc)          (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode,
                                                 GimpUndoAccumulator *accum);
typedef void       (* GimpUndoFreeFunc)         (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode);


/*  structs  */

struct _GimpCoords
{
  gdouble x;
  gdouble y;
  gdouble pressure;
  gdouble xtilt;
  gdouble ytilt;
  gdouble wheel;
};


#include "paint/paint-types.h"
#include "text/text-types.h"
#include "vectors/vectors-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"


#endif /* __CORE_TYPES_H__ */
