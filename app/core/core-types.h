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


#include "libgimpbase/gimpbasetypes.h"
#include "libgimpmath/gimpmath.h"

#include "base/base-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"
#include "vectors/vectors-types.h"

#include "undo_types.h"  /* EEK */

#include "core/core-enums.h"


/*  enums  */

typedef enum  /*< chop=ADD_ >*/
{
  ADD_WHITE_MASK,
  ADD_BLACK_MASK,
  ADD_ALPHA_MASK,
  ADD_SELECTION_MASK,
  ADD_INV_SELECTION_MASK,
  ADD_COPY_MASK,
  ADD_INV_COPY_MASK
} AddMaskType;

typedef enum
{
  APPLY,
  DISCARD
} MaskApplyMode;

typedef enum
{
  HORIZONTAL,
  VERTICAL,
  UNKNOWN
} OrientationType;

typedef enum /*< pdb-skip >*/
{
  ORIENTATION_UNKNOWN,
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
} InternalOrientationType;

/*  Selection Boolean operations  */
typedef enum /*< chop=CHANNEL_OP_ >*/
{
  CHANNEL_OP_ADD,
  CHANNEL_OP_SUB,
  CHANNEL_OP_REPLACE,
  CHANNEL_OP_INTERSECT
} ChannelOps;

typedef enum
{
  FOREGROUND_FILL,	/*< nick=FG_IMAGE_FILL >*/
  BACKGROUND_FILL,	/*< nick=BG_IMAGE_FILL >*/
  WHITE_FILL,		/*< nick=WHITE_IMAGE_FILL >*/
  TRANSPARENT_FILL,	/*< nick=TRANS_IMAGE_FILL >*/
  NO_FILL		/*< nick=NO_IMAGE_FILL >*/
} GimpFillType;

typedef enum
{
  OFFSET_BACKGROUND,
  OFFSET_TRANSPARENT
} GimpOffsetType;

typedef enum
{
  EXPAND_AS_NECESSARY,
  CLIP_TO_IMAGE,
  CLIP_TO_BOTTOM_LAYER,
  FLATTEN_IMAGE
} MergeType;

typedef enum
{
  MAKE_PALETTE   = 0,
  REUSE_PALETTE  = 1,
  WEB_PALETTE    = 2,
  MONO_PALETTE   = 3,
  CUSTOM_PALETTE = 4
} ConvertPaletteType;

typedef enum
{
  NO_DITHER         = 0,
  FS_DITHER         = 1,
  FSLOWBLEED_DITHER = 2,
  FIXED_DITHER      = 3,

  NODESTRUCT_DITHER = 4 /* NEVER USE NODESTRUCT_DITHER EXPLICITLY */
} ConvertDitherType;

typedef enum
{
  FG_BUCKET_FILL,
  BG_BUCKET_FILL,
  PATTERN_BUCKET_FILL
} BucketFillMode;


/*  base objects  */

typedef struct _GimpObject          GimpObject;

typedef struct _Gimp                Gimp;

typedef struct _GimpParasiteList    GimpParasiteList;

typedef struct _GimpModuleInfoObj   GimpModuleInfoObj;

typedef struct _GimpContainer       GimpContainer;
typedef struct _GimpList            GimpList;
typedef struct _GimpDataList        GimpDataList;

typedef struct _GimpDataFactory     GimpDataFactory;

typedef struct _GimpContext         GimpContext;

typedef struct _GimpViewable        GimpViewable;

typedef struct _GimpBuffer          GimpBuffer;

typedef struct _GimpToolInfo        GimpToolInfo;

typedef struct _GimpImagefile       GimpImagefile;

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


/*  other objects  */

typedef struct _ImageMap            ImageMap; /* temp_hack, will be an object */


/*  non-object types  */

typedef struct _GimpCoords          GimpCoords;

typedef struct _GimpCoreConfig      GimpCoreConfig;

typedef struct _GimpGuide           GimpGuide;

typedef struct _GimpImageNewValues  GimpImageNewValues;

typedef struct _GimpProgress        GimpProgress;

typedef         guint32             GimpTattoo;


/*  EEK stuff  */

typedef struct _Path                Path;
typedef struct _PathPoint           PathPoint;
typedef struct _PathList            PathList;


/*  stuff which is forward declared here so we don't need to cross-include it  */

typedef struct _GimpToolOptions     GimpToolOptions;


/*  functions  */

typedef void       (* GimpInitStatusFunc)       (const gchar *text1,
                                                 const gchar *text2,
                                                 gdouble      percentage);
typedef void       (* GimpDataFileLoaderFunc)   (const gchar *filename,
						 gpointer     loader_data);
typedef GimpData * (* GimpDataObjectLoaderFunc) (const gchar *filename);


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


#endif /* __CORE_TYPES_H__ */
