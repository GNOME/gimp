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

/*< proxy-skip >*/

#ifndef __CORE_TYPES_H__
#define __CORE_TYPES_H__


#include "libgimpmath/gimpmath.h"

#include "base/base-types.h"

#include "core/core-enums.h"


/*  defines  */

#define GIMP_OPACITY_TRANSPARENT 0.0
#define GIMP_OPACITY_OPAQUE      1.0


/*  enums  */

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


typedef enum /*< pdb-skip >*/ /*< skip >*/
{
  /* NOTE: If you change this list, please update the textual mapping at
   *  the bottom of undo.c as well.
   */

  /* Type NO_UNDO_GROUP (0) is special - in the gimpimage structure it
   * means there is no undo group currently being added to.
   */
  NO_UNDO_GROUP = 0,

  FIRST_UNDO_GROUP = NO_UNDO_GROUP,

  IMAGE_SCALE_UNDO_GROUP,
  IMAGE_RESIZE_UNDO_GROUP,
  IMAGE_CONVERT_UNDO_GROUP,
  IMAGE_CROP_UNDO_GROUP,
  IMAGE_LAYERS_MERGE_UNDO_GROUP,
  IMAGE_QMASK_UNDO_GROUP,
  IMAGE_GUIDE_UNDO_GROUP,
  LAYER_PROPERTIES_UNDO_GROUP,
  LAYER_SCALE_UNDO_GROUP,
  LAYER_RESIZE_UNDO_GROUP,
  LAYER_DISPLACE_UNDO_GROUP,
  LAYER_LINKED_UNDO_GROUP,
  LAYER_APPLY_MASK_UNDO_GROUP,
  FS_FLOAT_UNDO_GROUP,
  FS_ANCHOR_UNDO_GROUP,
  EDIT_PASTE_UNDO_GROUP,
  EDIT_CUT_UNDO_GROUP,
  TEXT_UNDO_GROUP,
  TRANSFORM_UNDO_GROUP,
  PAINT_UNDO_GROUP,
  PARASITE_ATTACH_UNDO_GROUP,
  PARASITE_REMOVE_UNDO_GROUP,
  MISC_UNDO_GROUP,

  LAST_UNDO_GROUP = MISC_UNDO_GROUP
} UndoType;


/*  base objects  */

typedef struct _GimpObject          GimpObject; /*< proxy-include >*/

typedef struct _Gimp                Gimp;

typedef struct _GimpParasiteList    GimpParasiteList;

typedef struct _GimpModuleInfoObj   GimpModuleInfoObj;

typedef struct _GimpContainer       GimpContainer;
typedef struct _GimpList            GimpList;
typedef struct _GimpDataList        GimpDataList;

typedef struct _GimpDataFactory     GimpDataFactory;

typedef struct _GimpContext         GimpContext;

typedef struct _GimpViewable        GimpViewable;
typedef struct _GimpItem            GimpItem;

typedef struct _GimpBuffer          GimpBuffer;

typedef struct _GimpPaintInfo       GimpPaintInfo;
typedef struct _GimpToolInfo        GimpToolInfo; /*< proxy-include >*/

typedef struct _GimpImagefile       GimpImagefile;
typedef struct _GimpList            GimpDocumentList;

typedef struct _GimpImageMap        GimpImageMap;

/*  drawable objects  */

typedef struct _GimpDrawable        GimpDrawable; /*< proxy-resume >*/

typedef struct _GimpChannel         GimpChannel;

typedef struct _GimpLayer           GimpLayer;
typedef struct _GimpLayerMask       GimpLayerMask;

typedef struct _GimpImage           GimpImage;


/*  data objects  */

typedef struct _GimpData            GimpData; /*< proxy-skip >*/

typedef struct _GimpBrush	    GimpBrush;
typedef struct _GimpBrushGenerated  GimpBrushGenerated;
typedef struct _GimpBrushPipe       GimpBrushPipe;

typedef struct _GimpGradient        GimpGradient;

typedef struct _GimpPattern         GimpPattern;

typedef struct _GimpPalette         GimpPalette;


/*  undo objects  */

typedef struct _GimpUndo            GimpUndo;
typedef struct _GimpUndoStack       GimpUndoStack;


/*  non-object types  */

typedef struct _GimpCoords          GimpCoords; /*< proxy-include >*/

typedef struct _GimpCoreConfig      GimpCoreConfig;

typedef struct _GimpGuide           GimpGuide;

typedef struct _GimpImageNewValues  GimpImageNewValues;

typedef struct _GimpProgress        GimpProgress;

typedef         guint32             GimpTattoo;

typedef struct _GimpPaletteEntry    GimpPaletteEntry;


/*  EEK stuff  */

typedef struct _Path                Path;
typedef struct _PathPoint           PathPoint;
typedef struct _PathList            PathList;

/*  stuff which is forward declared here so we don't need to cross-include it
 */

typedef struct _GimpToolOptions     GimpToolOptions;  /*< proxy-include >*/

typedef GimpToolOptions * (* GimpToolOptionsNewFunc) (GimpToolInfo *tool_info);  /*< proxy-include >*/


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


#include "paint/paint-types.h"
#include "vectors/vectors-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"


#endif /* __CORE_TYPES_H__ */
