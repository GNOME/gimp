/* LIGMA - The GNU Image Manipulation Program
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


#include "libligmabase/ligmabasetypes.h"
#include "libligmamath/ligmamathtypes.h"
#include "libligmacolor/ligmacolortypes.h"
#include "libligmamodule/ligmamoduletypes.h"
#include "libligmathumb/ligmathumb-types.h"

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

#define LIGMA_COORDS_MIN_PRESSURE      0.0
#define LIGMA_COORDS_MAX_PRESSURE      1.0
#define LIGMA_COORDS_DEFAULT_PRESSURE  1.0

#define LIGMA_COORDS_MIN_TILT         -1.0
#define LIGMA_COORDS_MAX_TILT          1.0
#define LIGMA_COORDS_DEFAULT_TILT      0.0

#define LIGMA_COORDS_MIN_WHEEL         0.0
#define LIGMA_COORDS_MAX_WHEEL         1.0
#define LIGMA_COORDS_DEFAULT_WHEEL     0.5

#define LIGMA_COORDS_DEFAULT_DISTANCE  0.0
#define LIGMA_COORDS_DEFAULT_ROTATION  0.0
#define LIGMA_COORDS_DEFAULT_SLIDER    0.0

#define LIGMA_COORDS_DEFAULT_VELOCITY  0.0

#define LIGMA_COORDS_DEFAULT_DIRECTION 0.0

#define LIGMA_COORDS_DEFAULT_XSCALE    1.0
#define LIGMA_COORDS_DEFAULT_YSCALE    1.0

#define LIGMA_COORDS_DEFAULT_VALUES    { 0.0, 0.0, \
                                        LIGMA_COORDS_DEFAULT_PRESSURE, \
                                        LIGMA_COORDS_DEFAULT_TILT,     \
                                        LIGMA_COORDS_DEFAULT_TILT,     \
                                        LIGMA_COORDS_DEFAULT_WHEEL,    \
                                        LIGMA_COORDS_DEFAULT_DISTANCE, \
                                        LIGMA_COORDS_DEFAULT_ROTATION, \
                                        LIGMA_COORDS_DEFAULT_SLIDER,   \
                                        LIGMA_COORDS_DEFAULT_VELOCITY, \
                                        LIGMA_COORDS_DEFAULT_DIRECTION,\
                                        LIGMA_COORDS_DEFAULT_XSCALE,   \
                                        LIGMA_COORDS_DEFAULT_YSCALE }


/*  base classes  */

typedef struct _LigmaObject                      LigmaObject;
typedef struct _LigmaViewable                    LigmaViewable;
typedef struct _LigmaFilter                      LigmaFilter;
typedef struct _LigmaItem                        LigmaItem;
typedef struct _LigmaAuxItem                     LigmaAuxItem;

typedef struct _Ligma                            Ligma;
typedef struct _LigmaImage                       LigmaImage;

typedef struct _LigmaDisplay                     LigmaDisplay;


/*  containers  */

typedef struct _LigmaContainer                   LigmaContainer;
typedef struct _LigmaList                        LigmaList;
typedef struct _LigmaDocumentList                LigmaDocumentList;
typedef struct _LigmaDrawableStack               LigmaDrawableStack;
typedef struct _LigmaFilteredContainer           LigmaFilteredContainer;
typedef struct _LigmaFilterStack                 LigmaFilterStack;
typedef struct _LigmaItemList                    LigmaItemList;
typedef struct _LigmaItemStack                   LigmaItemStack;
typedef struct _LigmaLayerStack                  LigmaLayerStack;
typedef struct _LigmaTaggedContainer             LigmaTaggedContainer;
typedef struct _LigmaTreeProxy                   LigmaTreeProxy;


/*  not really a container  */

typedef struct _LigmaItemTree                    LigmaItemTree;


/*  context objects  */

typedef struct _LigmaContext                     LigmaContext;
typedef struct _LigmaFillOptions                 LigmaFillOptions;
typedef struct _LigmaStrokeOptions               LigmaStrokeOptions;
typedef struct _LigmaToolOptions                 LigmaToolOptions;


/*  info objects  */

typedef struct _LigmaPaintInfo                   LigmaPaintInfo;
typedef struct _LigmaToolGroup                   LigmaToolGroup;
typedef struct _LigmaToolInfo                    LigmaToolInfo;
typedef struct _LigmaToolItem                    LigmaToolItem;


/*  data objects  */

typedef struct _LigmaDataFactory                 LigmaDataFactory;
typedef struct _LigmaDataLoaderFactory           LigmaDataLoaderFactory;
typedef struct _LigmaData                        LigmaData;
typedef struct _LigmaBrush                       LigmaBrush;
typedef struct _LigmaBrushCache                  LigmaBrushCache;
typedef struct _LigmaBrushClipboard              LigmaBrushClipboard;
typedef struct _LigmaBrushGenerated              LigmaBrushGenerated;
typedef struct _LigmaBrushPipe                   LigmaBrushPipe;
typedef struct _LigmaCurve                       LigmaCurve;
typedef struct _LigmaDynamics                    LigmaDynamics;
typedef struct _LigmaDynamicsOutput              LigmaDynamicsOutput;
typedef struct _LigmaGradient                    LigmaGradient;
typedef struct _LigmaMybrush                     LigmaMybrush;
typedef struct _LigmaPalette                     LigmaPalette;
typedef struct _LigmaPaletteMru                  LigmaPaletteMru;
typedef struct _LigmaPattern                     LigmaPattern;
typedef struct _LigmaPatternClipboard            LigmaPatternClipboard;
typedef struct _LigmaToolPreset                  LigmaToolPreset;
typedef struct _LigmaTagCache                    LigmaTagCache;


/*  drawable objects  */

typedef struct _LigmaDrawable                    LigmaDrawable;
typedef struct _LigmaChannel                     LigmaChannel;
typedef struct _LigmaLayerMask                   LigmaLayerMask;
typedef struct _LigmaSelection                   LigmaSelection;
typedef struct _LigmaLayer                       LigmaLayer;
typedef struct _LigmaGroupLayer                  LigmaGroupLayer;


/*  auxiliary image items  */

typedef struct _LigmaGuide                       LigmaGuide;
typedef struct _LigmaSamplePoint                 LigmaSamplePoint;


/*  undo objects  */

typedef struct _LigmaUndo                        LigmaUndo;
typedef struct _LigmaUndoStack                   LigmaUndoStack;
typedef struct _LigmaUndoAccumulator             LigmaUndoAccumulator;


/* Symmetry transformations */

typedef struct _LigmaSymmetry                    LigmaSymmetry;
typedef struct _LigmaMirror                      LigmaMirror;
typedef struct _LigmaTiling                      LigmaTiling;
typedef struct _LigmaMandala                     LigmaMandala;


/*  misc objects  */

typedef struct _LigmaAsync                       LigmaAsync;
typedef struct _LigmaAsyncSet                    LigmaAsyncSet;
typedef struct _LigmaBuffer                      LigmaBuffer;
typedef struct _LigmaDrawableFilter              LigmaDrawableFilter;
typedef struct _LigmaEnvironTable                LigmaEnvironTable;
typedef struct _LigmaExtension                   LigmaExtension;
typedef struct _LigmaExtensionManager            LigmaExtensionManager;
typedef struct _LigmaHistogram                   LigmaHistogram;
typedef struct _LigmaIdTable                     LigmaIdTable;
typedef struct _LigmaImagefile                   LigmaImagefile;
typedef struct _LigmaImageProxy                  LigmaImageProxy;
typedef struct _LigmaInterpreterDB               LigmaInterpreterDB;
typedef struct _LigmaLineArt                     LigmaLineArt;
typedef struct _LigmaObjectQueue                 LigmaObjectQueue;
typedef struct _LigmaParasiteList                LigmaParasiteList;
typedef struct _LigmaPdbProgress                 LigmaPdbProgress;
typedef struct _LigmaProjection                  LigmaProjection;
typedef struct _LigmaSettings                    LigmaSettings;
typedef struct _LigmaSubProgress                 LigmaSubProgress;
typedef struct _LigmaTag                         LigmaTag;
typedef struct _LigmaTreeHandler                 LigmaTreeHandler;
typedef struct _LigmaTriviallyCancelableWaitable LigmaTriviallyCancelableWaitable;
typedef struct _LigmaUncancelableWaitable        LigmaUncancelableWaitable;


/*  interfaces  */

typedef struct _LigmaCancelable                  LigmaCancelable;  /* dummy typedef */
typedef struct _LigmaPickable                    LigmaPickable;    /* dummy typedef */
typedef struct _LigmaProgress                    LigmaProgress;    /* dummy typedef */
typedef struct _LigmaProjectable                 LigmaProjectable; /* dummy typedef */
typedef struct _LigmaTagged                      LigmaTagged;      /* dummy typedef */
typedef struct _LigmaWaitable                    LigmaWaitable;    /* dummy typedef */


/*  non-object types  */

typedef struct _LigmaBacktrace                   LigmaBacktrace;
typedef struct _LigmaBoundSeg                    LigmaBoundSeg;
typedef struct _LigmaChunkIterator               LigmaChunkIterator;
typedef struct _LigmaCoords                      LigmaCoords;
typedef struct _LigmaGradientSegment             LigmaGradientSegment;
typedef struct _LigmaPaletteEntry                LigmaPaletteEntry;
typedef struct _LigmaScanConvert                 LigmaScanConvert;
typedef struct _LigmaTempBuf                     LigmaTempBuf;
typedef         guint32                         LigmaTattoo;

/* The following hack is made so that we can reuse the definition
 * the cairo definition of cairo_path_t without having to translate
 * between our own version of a bezier description and cairos version.
 *
 * to avoid having to include <cairo.h> in each and every file
 * including this file we only use the "real" definition when cairo.h
 * already has been included and use something else.
 *
 * Note that if you really want to work with LigmaBezierDesc (except just
 * passing pointers to it around) you also need to include <cairo.h>.
 */
#ifdef CAIRO_VERSION
typedef cairo_path_t LigmaBezierDesc;
#else
typedef void * LigmaBezierDesc;
#endif


/*  functions  */

typedef void     (* LigmaInitStatusFunc)    (const gchar *text1,
                                            const gchar *text2,
                                            gdouble      percentage);

typedef gboolean (* LigmaObjectFilterFunc)  (LigmaObject  *object,
                                            gpointer     user_data);

typedef gint64   (* LigmaMemsizeFunc)       (gpointer     instance,
                                            gint64      *gui_size);

typedef void     (* LigmaRunAsyncFunc)      (LigmaAsync   *async,
                                            gpointer     user_data);


/*  structs  */

struct _LigmaCoords
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

typedef struct _LigmaSegment LigmaSegment;

struct _LigmaSegment
{
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};


#include "gegl/ligma-gegl-types.h"
#include "paint/paint-types.h"
#include "text/text-types.h"
#include "vectors/vectors-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"


#endif /* __CORE_TYPES_H__ */
