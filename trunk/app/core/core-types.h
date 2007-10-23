/* GIMP - The GNU Image Manipulation Program
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


#include "libgimpmodule/gimpmoduletypes.h"
#include "libgimpthumb/gimpthumb-types.h"

#include "base/base-types.h"

#include "core/core-enums.h"


/*  defines  */

#define GIMP_OPACITY_TRANSPARENT      0.0
#define GIMP_OPACITY_OPAQUE           1.0

#define GIMP_COORDS_MIN_PRESSURE      0.0
#define GIMP_COORDS_MAX_PRESSURE      1.0
#define GIMP_COORDS_DEFAULT_PRESSURE  1.0

#define GIMP_COORDS_MIN_TILT         -1.0
#define GIMP_COORDS_MAX_TILT          1.0
#define GIMP_COORDS_DEFAULT_TILT      0.0

#define GIMP_COORDS_MIN_WHEEL         0.0
#define GIMP_COORDS_MAX_WHEEL         1.0
#define GIMP_COORDS_DEFAULT_WHEEL     0.5

#define GIMP_COORDS_DEFAULT_VALUES  { 0.0, 0.0, \
                                      GIMP_COORDS_DEFAULT_PRESSURE, \
                                      GIMP_COORDS_DEFAULT_TILT,     \
                                      GIMP_COORDS_DEFAULT_TILT,     \
                                      GIMP_COORDS_DEFAULT_WHEEL }


/*  base classes  */

typedef struct _GimpObject          GimpObject;
typedef struct _GimpViewable        GimpViewable;
typedef struct _GimpItem            GimpItem;

typedef struct _Gimp                Gimp;
typedef struct _GimpImage           GimpImage;


/*  containers  */

typedef struct _GimpContainer       GimpContainer;
typedef struct _GimpList            GimpList;
typedef struct _GimpDocumentList    GimpDocumentList;
typedef struct _GimpToolPresets     GimpToolPresets;


/*  context objects  */

typedef struct _GimpContext         GimpContext;
typedef struct _GimpStrokeOptions   GimpStrokeOptions;
typedef struct _GimpToolOptions     GimpToolOptions;


/*  info objects  */

typedef struct _GimpPaintInfo       GimpPaintInfo;
typedef struct _GimpToolInfo        GimpToolInfo;


/*  data objects  */

typedef struct _GimpDataFactory      GimpDataFactory;
typedef struct _GimpData             GimpData;
typedef struct _GimpBrush            GimpBrush;
typedef struct _GimpBrushClipboard   GimpBrushClipboard;
typedef struct _GimpBrushGenerated   GimpBrushGenerated;
typedef struct _GimpBrushPipe        GimpBrushPipe;
typedef struct _GimpGradient         GimpGradient;
typedef struct _GimpPattern          GimpPattern;
typedef struct _GimpPatternClipboard GimpPatternClipboard;
typedef struct _GimpPalette          GimpPalette;


/*  drawable objects  */

typedef struct _GimpDrawable        GimpDrawable;
typedef struct _GimpChannel         GimpChannel;
typedef struct _GimpSelection       GimpSelection;
typedef struct _GimpLayer           GimpLayer;
typedef struct _GimpLayerMask       GimpLayerMask;


/*  undo objects  */

typedef struct _GimpUndo              GimpUndo;
typedef struct _GimpImageUndo         GimpImageUndo;
typedef struct _GimpItemUndo          GimpItemUndo;
typedef struct _GimpItemPropUndo      GimpItemPropUndo;
typedef struct _GimpChannelUndo       GimpChannelUndo;
typedef struct _GimpChannelPropUndo   GimpChannelPropUndo;
typedef struct _GimpDrawableUndo      GimpDrawableUndo;
typedef struct _GimpDrawableModUndo   GimpDrawableModUndo;
typedef struct _GimpLayerMaskUndo     GimpLayerMaskUndo;
typedef struct _GimpLayerMaskPropUndo GimpLayerMaskPropUndo;
typedef struct _GimpLayerUndo         GimpLayerUndo;
typedef struct _GimpLayerPropUndo     GimpLayerPropUndo;
typedef struct _GimpMaskUndo          GimpMaskUndo;
typedef struct _GimpGuideUndo         GimpGuideUndo;
typedef struct _GimpSamplePointUndo   GimpSamplePointUndo;
typedef struct _GimpFloatingSelUndo   GimpFloatingSelUndo;
typedef struct _GimpUndoStack         GimpUndoStack;
typedef struct _GimpUndoAccumulator   GimpUndoAccumulator;


/*  misc objects  */

typedef struct _GimpBuffer          GimpBuffer;
typedef struct _GimpEnvironTable    GimpEnvironTable;
/* typedef struct _GimpGrid         GimpGrid; in config-types.h */
typedef struct _GimpGuide           GimpGuide;
typedef struct _GimpImagefile       GimpImagefile;
typedef struct _GimpImageMap        GimpImageMap;
typedef struct _GimpInterpreterDB   GimpInterpreterDB;
typedef struct _GimpParasiteList    GimpParasiteList;
typedef struct _GimpPdbProgress     GimpPdbProgress;
typedef struct _GimpProjection      GimpProjection;
typedef struct _GimpSubProgress     GimpSubProgress;
typedef struct _GimpStrokeDesc      GimpStrokeDesc;
/* typedef struct _GimpTemplate     GimpTemplate; in config-types.h */


/*  interfaces  */

typedef struct _GimpPickable        GimpPickable; /* dummy typedef */
typedef struct _GimpProgress        GimpProgress; /* dummy typedef */


/*  non-object types  */

typedef struct _GimpArea            GimpArea;
typedef struct _GimpCoords          GimpCoords;
typedef struct _GimpGradientSegment GimpGradientSegment;
typedef struct _GimpSamplePoint     GimpSamplePoint;
typedef         guint32             GimpTattoo;
typedef struct _GimpPaletteEntry    GimpPaletteEntry;
typedef struct _GimpScanConvert     GimpScanConvert;


/*  functions  */

typedef void     (* GimpInitStatusFunc)   (const gchar      *text1,
                                           const gchar      *text2,
                                           gdouble           percentage);

typedef gboolean (* GimpObjectFilterFunc) (const GimpObject *object,
                                           gpointer          user_data);

typedef gint64   (* GimpMemsizeFunc)      (gpointer          instance,
                                           gint64           *gui_size);


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
