/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpbezierstroke.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpvectortool.h"
#include "selection_options.h"

#include "libgimp/gimpintl.h"


/*  definitions  */
#define  TARGET         9
#define  ARC_RADIUS     30
#define  STATUSBAR_SIZE 128


/*  the vector tool options  */
typedef struct _VectorOptions VectorOptions;

struct _VectorOptions
{
  SelectionOptions  selection_options;
};


/*  local function prototypes  */
static void   gimp_vector_tool_class_init      (GimpVectorToolClass *klass);
static void   gimp_vector_tool_init            (GimpVectorTool      *tool);
static void   gimp_vector_tool_finalize        (GObject             *object);

static void   gimp_vector_tool_control         (GimpTool        *tool,
                                                GimpToolAction   action,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_button_press    (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_button_release  (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_motion          (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_cursor_update   (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);

static void   gimp_vector_tool_draw            (GimpDrawTool    *draw_tool);



static GimpToolOptions * vector_tool_options_new     (GimpToolInfo    *tool_info);
static void              vector_tool_options_reset   (GimpToolOptions *tool_options);


static GimpSelectionToolClass *parent_class = NULL;


void
gimp_vector_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_VECTOR_TOOL,
                vector_tool_options_new,
                FALSE,
                "gimp-vector-tool",
                _("Vectors"),
                _("the most promising path tool prototype... :-)"),
                N_("/Tools/Vectors"), NULL,
                NULL, "tools/vector.html",
                GIMP_STOCK_TOOL_PATH,
                data);
}

GType
gimp_vector_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpVectorToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_vector_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpVectorTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_vector_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
					  "GimpVectorTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_vector_tool_class_init (GimpVectorToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_vector_tool_finalize;

  tool_class->control        = gimp_vector_tool_control;
  tool_class->button_press   = gimp_vector_tool_button_press;
  tool_class->button_release = gimp_vector_tool_button_release;
  tool_class->motion         = gimp_vector_tool_motion;
  tool_class->cursor_update  = gimp_vector_tool_cursor_update;

  draw_tool_class->draw      = gimp_vector_tool_draw;
}

static void
gimp_vector_tool_init (GimpVectorTool *vector_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (vector_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  
  vector_tool->function       = VECTORS_CREATING;
  vector_tool->last_x         = 0;
  vector_tool->last_y         = 0;

  vector_tool->cur_anchor     = NULL;
  vector_tool->cur_stroke     = NULL;
  vector_tool->vectors        = NULL;
  vector_tool->active_anchors = NULL;
}

static void
gimp_vector_tool_finalize (GObject *object)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (object);

  if (vector_tool->vectors)
    g_object_unref (vector_tool->vectors);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vector_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *gdisp)
{
  GimpVectorTool *vector_tool;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gimp_tool_pop_status (tool);
      gimp_tool_control_halt (tool->control);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}


static void
gimp_vector_tool_button_press (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpVectorTool *vector_tool;
  VectorOptions  *options;
  GimpAnchor     *anchor = NULL;
  GimpStroke     *stroke = NULL;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  options = (VectorOptions *) tool->tool_info->tool_options;

  /*  if we are changing displays, pop the statusbar of the old one  */ 
  if (gimp_tool_control_is_active (tool->control) && gdisp != tool->gdisp)
    {
      gimp_tool_pop_status (tool);
    }
  
  if (gimp_tool_control_is_active (tool->control) && gdisp == tool->gdisp)
    {
      /*  if the cursor is in one of the handles,
       *  the new function will be moving or adding a new point or guide
       */

      anchor = gimp_vectors_anchor_get (vector_tool->vectors, coords, &stroke);

      if (anchor && gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), gdisp,
                                              coords->x,
                                              coords->y,
                                              GIMP_HANDLE_CIRCLE,
                                              anchor->position.x,
                                              anchor->position.y,
                                              TARGET,
                                              TARGET,
                                              GTK_ANCHOR_CENTER,
                                              FALSE))
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (vector_tool));

          if (anchor->type == ANCHOR_HANDLE)
            gimp_stroke_anchor_select (stroke, anchor, TRUE);
          
          /* doublecheck if there are control handles at this anchor */
          anchor = gimp_vectors_anchor_get (vector_tool->vectors,
                                            coords, &stroke);

          vector_tool->function   = VECTORS_MOVING;
          vector_tool->cur_stroke = stroke;
          vector_tool->cur_anchor = anchor;

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (vector_tool));
        }
    }

  if (vector_tool->function == VECTORS_CREATING)
    {
      if (gimp_tool_control_is_active (tool->control))
	{
	  /* reset everything */
	  gimp_draw_tool_stop (GIMP_DRAW_TOOL (vector_tool));
	}

      if (! vector_tool->vectors)
        {
          GimpVectors *vectors;

          vectors = g_object_new (GIMP_TYPE_VECTORS, NULL);

          gimp_image_add_vectors (gdisp->gimage, vectors, -1);
          gimp_object_set_name (GIMP_OBJECT (vectors), _("Unnamed"));

          vector_tool->vectors = g_object_ref (vectors);
        }

      stroke = gimp_bezier_stroke_new (coords);
      anchor = gimp_stroke_anchor_get (stroke, coords);

      gimp_vectors_stroke_add (vector_tool->vectors, stroke);

      vector_tool->cur_stroke = stroke;
      vector_tool->cur_anchor = anchor;
      gimp_stroke_anchor_select (stroke, anchor, TRUE);

      vector_tool->function = VECTORS_MOVING;

      /*  set the gdisplay  */
      tool->gdisp = gdisp;

      if (gimp_tool_control_is_active (tool->control))
	{
	  gimp_tool_pop_status (tool);
	  gimp_tool_push_status (tool, "");
        }

      /*  start drawing the vector tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
    }

  if (vector_tool->function == VECTORS_ADDING)
    {
      if (gimp_tool_control_is_active (tool->control))
	{
	  /* reset everything */
	  gimp_draw_tool_stop (GIMP_DRAW_TOOL (vector_tool));
	}

      anchor = gimp_bezier_stroke_extend (GIMP_BEZIER_STROKE (vector_tool->cur_stroke), coords, vector_tool->cur_anchor, EXTEND_EDITABLE);
      if (anchor)
        vector_tool->cur_anchor = anchor;

      /*  set the gdisplay  */
      tool->gdisp = gdisp;

      if (gimp_tool_control_is_active (tool->control))
	{
	  gimp_tool_pop_status (tool);
	  gimp_tool_push_status (tool, "");
        }

      /*  start drawing the vector tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_vector_tool_button_release (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpVectorTool *vector_tool;
  GimpViewable   *viewable;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  vector_tool->function = VECTORS_ADDING;

  /* THIS DOES NOT BELONG HERE! */
  viewable = GIMP_VIEWABLE (vector_tool->vectors);
  gimp_viewable_invalidate_preview (viewable);

}

static void
gimp_vector_tool_motion (GimpTool        *tool,
                         GimpCoords      *coords,
                         guint32          time,
                         GdkModifierType  state,
                         GimpDisplay     *gdisp)
{
  GimpVectorTool *vector_tool;
  VectorOptions  *options;
  GimpAnchor     *anchor;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  options = (VectorOptions *) tool->tool_info->tool_options;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (vector_tool));

  switch (vector_tool->function)
    {
    case VECTORS_MOVING:
      /* if we are moving the start point and only have two,
       * make it the end point  */
      anchor = vector_tool->cur_anchor;

      if (anchor)
        gimp_stroke_anchor_move_absolute (vector_tool->cur_stroke,
                                          vector_tool->cur_anchor,
                                          coords, 0);

    default:
      break;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (vector_tool));
}

static void
gimp_vector_tool_cursor_update (GimpTool        *tool,
                                GimpCoords      *coords,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpVectorTool   *vector_tool;
  gboolean           in_handle = FALSE;
  GdkCursorType      ctype     = GIMP_MOUSE_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  GimpAnchor        *anchor;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  if (gimp_tool_control_is_active (tool->control) && tool->gdisp == gdisp)
    {
      anchor = gimp_vectors_anchor_get (vector_tool->vectors, coords, NULL);

      if (anchor && gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), gdisp,
                                              coords->x,
                                              coords->y,
                                              GIMP_HANDLE_CIRCLE,
                                              anchor->position.x,
                                              anchor->position.y,
                                              TARGET,
                                              TARGET,
                                              GTK_ANCHOR_CENTER,
                                              FALSE))
        {
          in_handle = TRUE;
          cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
        }
    }

  gimp_tool_control_set_cursor (tool->control, ctype);
  gimp_tool_control_set_cursor_modifier (tool->control, cmodifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_vector_tool_draw (GimpDrawTool *draw_tool)
{
  GimpVectorTool  *vector_tool;
  GimpTool        *tool;
  GimpAnchor      *cur_anchor = NULL;
  GimpStroke      *cur_stroke = NULL;
  GimpVectors     *vectors;
  GArray          *coords;
  gboolean         closed;
  GList           *draw_anchors, *ptr;

  vector_tool = GIMP_VECTOR_TOOL (draw_tool);
  tool        = GIMP_TOOL (draw_tool);

  vectors = vector_tool->vectors;

  while ((cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke)))
    {
      /* anchor handles */
      cur_anchor = NULL;
      draw_anchors = gimp_stroke_get_draw_anchors (cur_stroke);
      ptr = draw_anchors;

      while (ptr) 
        {
          cur_anchor = (GimpAnchor *) ptr->data;

          if (cur_anchor->type == ANCHOR_HANDLE)
            {
              gimp_draw_tool_draw_handle (draw_tool,
                                          cur_anchor->selected ?
                                            GIMP_HANDLE_CIRCLE:
                                            GIMP_HANDLE_FILLED_CIRCLE,
                                          cur_anchor->position.x,
                                          cur_anchor->position.y,
                                          TARGET,
                                          TARGET,
                                          GTK_ANCHOR_CENTER,
                                          FALSE);
            }
          ptr = ptr->next;
        }

      g_list_free (draw_anchors);

      /* control handles */
      draw_anchors = gimp_stroke_get_draw_controls (cur_stroke);
      ptr = draw_anchors;

      while (ptr) 
        {
          cur_anchor = (GimpAnchor *) ptr->data;

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_SQUARE,
                                      cur_anchor->position.x,
                                      cur_anchor->position.y,
                                      TARGET-2,
                                      TARGET-2,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
          ptr = ptr->next;
        }

      g_list_free (draw_anchors);

      /* the lines to the control handles */
      coords = gimp_stroke_get_draw_lines (cur_stroke);
      
      if (coords->len % 2 == 0)
        {
          gint i;
          for (i=0; i < coords->len; i += 2)
            gimp_draw_tool_draw_strokes (draw_tool,
                                       &(g_array_index (coords, GimpCoords, i)),
                                       2, FALSE, FALSE);
        }
      g_array_free (coords, TRUE);

      /* the stroke itself */
      coords = gimp_stroke_interpolate (cur_stroke, 1.0, &closed);

      if (coords->len)
        gimp_draw_tool_draw_strokes (draw_tool,
                                     & (g_array_index (coords, GimpCoords, 0)),
                                     coords->len, FALSE, FALSE);

      g_array_free (coords, TRUE);
    }
}

void
gimp_vector_tool_set_vectors (GimpVectorTool *vector_tool,
                              GimpVectors    *vectors)
{
  GimpDrawTool *draw_tool;
  GimpTool     *tool;
  GimpItem     *item;

  g_return_if_fail (GIMP_IS_VECTOR_TOOL (vector_tool));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  draw_tool = GIMP_DRAW_TOOL (vector_tool);
  tool      = GIMP_TOOL (vector_tool);
  item      = GIMP_ITEM (vectors);

  if (draw_tool->gdisp)
    {
      if (draw_tool->gdisp->gimage == item->gimage)
        gimp_draw_tool_pause (draw_tool);
      else
        gimp_draw_tool_stop (draw_tool);
    }

  if (vector_tool->vectors)
    g_object_unref (vector_tool->vectors);

  vector_tool->vectors        = vectors;
  vector_tool->cur_stroke     = NULL;
  vector_tool->cur_anchor     = NULL;
  vector_tool->active_anchors = NULL;
  vector_tool->function       = VECTORS_CREATING;

  if (vector_tool->vectors)
    g_object_ref (vector_tool->vectors);

  if (draw_tool->gdisp && draw_tool->gdisp->gimage == item->gimage)
    {
      gimp_draw_tool_resume (draw_tool);
    }
  else if (tool->gdisp && tool->gdisp->gimage == item->gimage)
    {
      gimp_draw_tool_start (draw_tool, tool->gdisp);
    }
  else
    {
      GimpContext *context;
      GimpDisplay *gdisp;

      context = gimp_get_current_context (tool->tool_info->gimp);

      gdisp = gimp_context_get_display (context);

      if (gdisp->gimage != item->gimage)
        {
          GList *list;

          gdisp = NULL;

          for (list = GIMP_LIST (item->gimage->gimp->displays)->list;
               list;
               list = g_list_next (list))
            {
              if (((GimpDisplay *) list->data)->gimage == item->gimage)
                {
                  gimp_context_set_display (context,
                                            (GimpDisplay *) list->data);

                  gdisp = gimp_context_get_display (context);
                  break;
                }
            }

          if (! gdisp)
            return;
        }

      tool->gdisp = gdisp;
      gimp_tool_control_activate (tool->control);

      gimp_draw_tool_start (draw_tool, tool->gdisp);
    }
}


/*  tool options stuff  */

static GimpToolOptions *
vector_tool_options_new (GimpToolInfo *tool_info)
{
  VectorOptions *options;
  GtkWidget      *vbox;

  options = g_new0 (VectorOptions, 1);

  selection_options_init ((SelectionOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = vector_tool_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  return (GimpToolOptions *) options;
}

static void
vector_tool_options_reset (GimpToolOptions *tool_options)
{
  VectorOptions *options;

  options = (VectorOptions *) tool_options;

  selection_options_reset (tool_options);
}
