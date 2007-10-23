/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_VECTOR_TOOL_H__
#define __GIMP_VECTOR_TOOL_H__


#include "gimpdrawtool.h"


/*  possible vector functions  */
typedef enum
{
  VECTORS_SELECT_VECTOR,
  VECTORS_CREATE_VECTOR,
  VECTORS_CREATE_STROKE,
  VECTORS_ADD_ANCHOR,
  VECTORS_MOVE_ANCHOR,
  VECTORS_MOVE_ANCHORSET,
  VECTORS_MOVE_HANDLE,
  VECTORS_MOVE_CURVE,
  VECTORS_MOVE_STROKE,
  VECTORS_MOVE_VECTORS,
  VECTORS_INSERT_ANCHOR,
  VECTORS_DELETE_ANCHOR,
  VECTORS_CONNECT_STROKES,
  VECTORS_DELETE_SEGMENT,
  VECTORS_CONVERT_EDGE,
  VECTORS_FINISHED
} GimpVectorFunction;


#define GIMP_TYPE_VECTOR_TOOL            (gimp_vector_tool_get_type ())
#define GIMP_VECTOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTOR_TOOL, GimpVectorTool))
#define GIMP_VECTOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTOR_TOOL, GimpVectorToolClass))
#define GIMP_IS_VECTOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTOR_TOOL))
#define GIMP_IS_VECTOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTOR_TOOL))
#define GIMP_VECTOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTOR_TOOL, GimpVectorToolClass))

#define GIMP_VECTOR_TOOL_GET_OPTIONS(t)  (GIMP_VECTOR_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpVectorTool      GimpVectorTool;
typedef struct _GimpVectorToolClass GimpVectorToolClass;

struct _GimpVectorTool
{
  GimpDrawTool          parent_instance;

  GimpVectorFunction    function;       /* function we're performing         */
  GimpAnchorFeatureType restriction;    /* movement restriction              */
  gboolean              modifier_lock;  /* can we toggle the Shift key?      */
  GdkModifierType       saved_state;    /* modifier state at button_press    */
  gdouble               last_x;         /* last x coordinate                 */
  gdouble               last_y;         /* last y coordinate                 */
  gboolean              undo_motion;    /* we need a motion to have an undo  */
  gboolean              have_undo;      /* did we push an undo at            */
                                        /* ..._button_press?                 */

  GimpAnchor           *cur_anchor;     /* the current Anchor                */
  GimpAnchor           *cur_anchor2;    /* secondary Anchor (end on_curve)   */
  GimpStroke           *cur_stroke;     /* the current Stroke                */
  gdouble               cur_position;   /* the current Position on a segment */
  GimpVectors          *cur_vectors;    /* the vectors the tool is hovering  */
                                        /* over (if different from ->vectors */
  GimpVectors          *vectors;        /* the current Vector data           */

  gint                  sel_count;      /* number of selected anchors        */
  GimpAnchor           *sel_anchor;     /* currently selected anchor, NULL   */
                                        /* if multiple anchors are selected  */
  GimpStroke           *sel_stroke;     /* selected stroke                   */

  GimpVectorMode        saved_mode;     /* used by modifier_key()            */
};

struct _GimpVectorToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_vector_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data);

GType   gimp_vector_tool_get_type (void) G_GNUC_CONST;


void    gimp_vector_tool_set_vectors (GimpVectorTool *vector_tool,
                                      GimpVectors    *vectors);


#endif  /*  __GIMP_VECTOR_TOOL_H__  */
