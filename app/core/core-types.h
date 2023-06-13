/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __CORE_TYPES_H__
#define __CORE_TYPES_H__


#include "libgimpbase/gimpbasetypes.h"
#include "libgimpmath/gimpmathtypes.h"
#include "libgimpcolor/gimpcolortypes.h"
#include "libgimpmodule/gimpmoduletypes.h"
#include "libgimpthumb/gimpthumb-types.h"

#include "config/config-types.h"

#include "core/core-enums.h"


/*  former base/ defines  */

#define MAX_CHANNELS  4

#define RED           0
#define GREEN         1
#define BLUE          2
#define ALPHA         3

#define GRAY          0
#define ALPHA_G       1

#define INDEXED       0
#define ALPHA_I       1


/*  defines  */

#define GIMP_COORDS_MIN_PRESSURE      0.0
#define GIMP_COORDS_MAX_PRESSURE      1.0
#define GIMP_COORDS_DEFAULT_PRESSURE  1.0

#define GIMP_COORDS_MIN_TILT         -1.0
#define GIMP_COORDS_MAX_TILT          1.0
#define GIMP_COORDS_DEFAULT_TILT      0.0

#define GIMP_COORDS_MIN_WHEEL         0.0
#define GIMP_COORDS_MAX_WHEEL         1.0
#define GIMP_COORDS_DEFAULT_WHEEL     0.5

#define GIMP_COORDS_DEFAULT_DISTANCE  0.0
#define GIMP_COORDS_DEFAULT_ROTATION  0.0
#define GIMP_COORDS_DEFAULT_SLIDER    0.0

#define GIMP_COORDS_DEFAULT_VELOCITY  0.0

#define GIMP_COORDS_DEFAULT_DIRECTION 0.0

#define GIMP_COORDS_DEFAULT_XSCALE    1.0
#define GIMP_COORDS_DEFAULT_YSCALE    1.0

#define GIMP_COORDS_DEFAULT_VALUES    { 0.0, 0.0, \
                                        GIMP_COORDS_DEFAULT_PRESSURE, \
                                        GIMP_COORDS_DEFAULT_TILT,     \
                                        GIMP_COORDS_DEFAULT_TILT,     \
                                        GIMP_COORDS_DEFAULT_WHEEL,    \
                                        GIMP_COORDS_DEFAULT_DISTANCE, \
                                        GIMP_COORDS_DEFAULT_ROTATION, \
                                        GIMP_COORDS_DEFAULT_SLIDER,   \
                                        GIMP_COORDS_DEFAULT_VELOCITY, \
                                        GIMP_COORDS_DEFAULT_DIRECTION,\
                                        GIMP_COORDS_DEFAULT_XSCALE,   \
                                        GIMP_COORDS_DEFAULT_YSCALE }


/*  base classes  */

typedef struct _GimpObject                      GimpObject;
typedef struct _GimpViewable                    GimpViewable;
typedef struct _GimpFilter                      GimpFilter;
typedef struct _GimpItem                        GimpItem;
typedef struct _GimpAuxItem                     GimpAuxItem;

typedef struct _Gimp                            Gimp;
typedef struct _GimpImage                       GimpImage;

typedef struct _GimpDisplay                     GimpDisplay;


/*  containers  */

typedef struct _GimpContainer                   GimpContainer;
typedef struct _GimpList                        GimpList;
typedef struct _GimpDocumentList                GimpDocumentList;
typedef struct _GimpDrawableStack               GimpDrawableStack;
typedef struct _GimpFilteredContainer           GimpFilteredContainer;
typedef struct _GimpFilterStack                 GimpFilterStack;
typedef struct _GimpItemList                    GimpItemList;
typedef struct _GimpItemStack                   GimpItemStack;
typedef struct _GimpLayerStack                  GimpLayerStack;
typedef struct _GimpTaggedContainer             GimpTaggedContainer;
typedef struct _GimpTreeProxy                   GimpTreeProxy;


/*  not really a container  */

typedef struct _GimpItemTree                    GimpItemTree;


/*  context objects  */

typedef struct _GimpContext                     GimpContext;
typedef struct _GimpFillOptions                 GimpFillOptions;
typedef struct _GimpStrokeOptions               GimpStrokeOptions;
typedef struct _GimpToolOptions                 GimpToolOptions;


/*  info objects  */

typedef struct _GimpPaintInfo                   GimpPaintInfo;
typedef struct _GimpToolGroup                   GimpToolGroup;
typedef struct _GimpToolInfo                    GimpToolInfo;
typedef struct _GimpToolItem                    GimpToolItem;


/*  data objects  */

typedef struct _GimpDataFactory                 GimpDataFactory;
typedef struct _GimpDataLoaderFactory           GimpDataLoaderFactory;
typedef struct _GimpResource                    GimpResource;
typedef struct _GimpData                        GimpData;
typedef struct _GimpBrush                       GimpBrush;
typedef struct _GimpBrushCache                  GimpBrushCache;
typedef struct _GimpBrushClipboard              GimpBrushClipboard;
typedef struct _GimpBrushGenerated              GimpBrushGenerated;
typedef struct _GimpBrushPipe                   GimpBrushPipe;
typedef struct _GimpCurve                       GimpCurve;
typedef struct _GimpDynamics                    GimpDynamics;
typedef struct _GimpDynamicsOutput              GimpDynamicsOutput;
typedef struct _GimpGradient                    GimpGradient;
typedef struct _GimpMybrush                     GimpMybrush;
typedef struct _GimpPadActions                  GimpPadActions;
typedef struct _GimpPalette                     GimpPalette;
typedef struct _GimpPaletteMru                  GimpPaletteMru;
typedef struct _GimpPattern                     GimpPattern;
typedef struct _GimpPatternClipboard            GimpPatternClipboard;
typedef struct _GimpToolPreset                  GimpToolPreset;
typedef struct _GimpTagCache                    GimpTagCache;


/*  drawable objects  */

typedef struct _GimpDrawable                    GimpDrawable;
typedef struct _GimpChannel                     GimpChannel;
typedef struct _GimpLayerMask                   GimpLayerMask;
typedef struct _GimpSelection                   GimpSelection;
typedef struct _GimpLayer                       GimpLayer;
typedef struct _GimpGroupLayer                  GimpGroupLayer;


/*  auxiliary image items  */

typedef struct _GimpGuide                       GimpGuide;
typedef struct _GimpSamplePoint                 GimpSamplePoint;


/*  undo objects  */

typedef struct _GimpUndo                        GimpUndo;
typedef struct _GimpUndoStack                   GimpUndoStack;
typedef struct _GimpUndoAccumulator             GimpUndoAccumulator;


/* Symmetry transformations */

typedef struct _GimpSymmetry                    GimpSymmetry;
typedef struct _GimpMirror                      GimpMirror;
typedef struct _GimpTiling                      GimpTiling;
typedef struct _GimpMandala                     GimpMandala;


/*  misc objects  */

typedef struct _GimpAsync                       GimpAsync;
typedef struct _GimpAsyncSet                    GimpAsyncSet;
typedef struct _GimpBuffer                      GimpBuffer;
typedef struct _GimpDrawableFilter              GimpDrawableFilter;
typedef struct _GimpEnvironTable                GimpEnvironTable;
typedef struct _GimpExtension                   GimpExtension;
typedef struct _GimpExtensionManager            GimpExtensionManager;
typedef struct _GimpHistogram                   GimpHistogram;
typedef struct _GimpIdTable                     GimpIdTable;
typedef struct _GimpImagefile                   GimpImagefile;
typedef struct _GimpImageProxy                  GimpImageProxy;
typedef struct _GimpInterpreterDB               GimpInterpreterDB;
typedef struct _GimpLineArt                     GimpLineArt;
typedef struct _GimpObjectQueue                 GimpObjectQueue;
typedef struct _GimpParasiteList                GimpParasiteList;
typedef struct _GimpPdbProgress                 GimpPdbProgress;
typedef struct _GimpProjection                  GimpProjection;
typedef struct _GimpSettings                    GimpSettings;
typedef struct _GimpSubProgress                 GimpSubProgress;
typedef struct _GimpTag                         GimpTag;
typedef struct _GimpTreeHandler                 GimpTreeHandler;
typedef struct _GimpTriviallyCancelableWaitable GimpTriviallyCancelableWaitable;
typedef struct _GimpUncancelableWaitable        GimpUncancelableWaitable;


/*  interfaces  */

typedef struct _GimpCancelable                  GimpCancelable;  /* dummy typedef */
typedef struct _GimpPickable                    GimpPickable;    /* dummy typedef */
typedef struct _GimpProgress                    GimpProgress;    /* dummy typedef */
typedef struct _GimpProjectable                 GimpProjectable; /* dummy typedef */
typedef struct _GimpTagged                      GimpTagged;      /* dummy typedef */
typedef struct _GimpWaitable                    GimpWaitable;    /* dummy typedef */


/*  non-object types  */

typedef struct _GimpBacktrace                   GimpBacktrace;
typedef struct _GimpBoundSeg                    GimpBoundSeg;
typedef struct _GimpChunkIterator               GimpChunkIterator;
typedef struct _GimpCoords                      GimpCoords;
typedef struct _GimpGradientSegment             GimpGradientSegment;
typedef struct _GimpPaletteEntry                GimpPaletteEntry;
typedef struct _GimpScanConvert                 GimpScanConvert;
typedef struct _GimpTempBuf                     GimpTempBuf;
typedef         guint32                         GimpTattoo;

/* The following hack is made so that we can reuse the definition
 * the cairo definition of cairo_path_t without having to translate
 * between our own version of a bezier description and cairos version.
 *
 * to avoid having to include <cairo.h> in each and every file
 * including this file we only use the "real" definition when cairo.h
 * already has been included and use something else.
 *
 * Note that if you really want to work with GimpBezierDesc (except just
 * passing pointers to it around) you also need to include <cairo.h>.
 */
#ifdef CAIRO_VERSION
typedef cairo_path_t GimpBezierDesc;
#else
typedef void * GimpBezierDesc;
#endif


/*  functions  */

typedef void     (* GimpInitStatusFunc)    (const gchar *text1,
                                            const gchar *text2,
                                            gdouble      percentage);

typedef gboolean (* GimpObjectFilterFunc)  (GimpObject  *object,
                                            gpointer     user_data);

typedef gint64   (* GimpMemsizeFunc)       (gpointer     instance,
                                            gint64      *gui_size);

typedef void     (* GimpRunAsyncFunc)      (GimpAsync   *async,
                                            gpointer     user_data);


/*  structs  */

struct _GimpCoords
{
  /* axes as reported by the device */
  gdouble  x;
  gdouble  y;
  gdouble  pressure;
  gdouble  xtilt;
  gdouble  ytilt;
  gdouble  wheel;
  gdouble  distance;
  gdouble  rotation;
  gdouble  slider;

  /* synthetic axes */
  gdouble  velocity;
  gdouble  direction;

  /* view transform */
  gdouble  xscale;  /* the view scale                */
  gdouble  yscale;
  gdouble  angle;   /* the view rotation angle       */
  gboolean reflect; /* whether the view is reflected */
};

/*  temp hack as replacement for GdkSegment  */

typedef struct _GimpSegment GimpSegment;

struct _GimpSegment
{
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};


#include "gegl/gimp-gegl-types.h"
#include "paint/paint-types.h"
#include "text/text-types.h"
#include "vectors/vectors-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"


#endif /* __CORE_TYPES_H__ */
