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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"

#include "floating_sel.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "path_transform.h"
#include "selection.h"
#include "undo.h"

#include "gimpeditselectiontool.h"
#include "gimpdrawtool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


#define EDIT_SELECT_SCROLL_LOCK FALSE
#define ARROW_VELOCITY          25
#define STATUSBAR_SIZE          128


#define GIMP_TYPE_EDIT_SELECTION_TOOL            (gimp_edit_selection_tool_get_type ())
#define GIMP_EDIT_SELECTION_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionTool))
#define GIMP_IS_EDIT_SELECTION_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL))
#define GIMP_EDIT_SELECTION_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionToolClass))
#define GIMP_IS_EDIT_SELECTION_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL))


typedef struct _GimpEditSelectionTool      GimpEditSelectionTool;
typedef struct _GimpEditSelectionToolClass GimpEditSelectionToolClass;

struct _GimpEditSelectionTool
{
  GimpDrawTool        parent_instance;

  gint                origx, origy;      /*  last x and y coords             */
  gint                cumlx, cumly;      /*  cumulative changes to x and yed */
  gint                x, y;              /*  current x and y coords          */
  gint                num_segs_in;       /* Num seg in selection boundary    */
  gint                num_segs_out;      /* Num seg in selection boundary    */
  BoundSeg           *segs_in;           /* Pointer to the channel sel. segs */
  BoundSeg           *segs_out;          /* Pointer to the channel sel. segs */

  gint                x1, y1;            /*  bounding box of selection mask  */
  gint                x2, y2;

  EditType            edit_type;         /*  translate the mask or layer?    */

  gboolean            first_move;        /*  we undo_freeze after the first  */

  guint               context_id;        /*  for the statusbar               */
};

struct _GimpEditSelectionToolClass
{
  GimpDrawToolClass parent_class;
};


static GtkType   gimp_edit_selection_tool_get_type   (void);
static void 	 gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass);
static void      gimp_edit_selection_tool_init       (GimpEditSelectionTool *edit_selection_tool);

static void      gimp_edit_selection_tool_destroy        (GtkObject      *object);

static void      gimp_edit_selection_tool_button_release (GimpTool       *tool,
							  GdkEventButton *bevent,
							  GDisplay       *gdisp);
static void      gimp_edit_selection_tool_motion         (GimpTool       *tool,
							  GdkEventMotion *mevent,
							  GDisplay       *gdisp);
static void      gimp_edit_selection_tool_control        (GimpTool       *tool,
							  ToolAction      action,
							  GDisplay       *gdisp);
static void      gimp_edit_selection_tool_cursor_update  (GimpTool       *tool,
							  GdkEventMotion *mevent,
							  GDisplay       *gdisp);
void             gimp_edit_selection_tool_arrow_key      (GimpTool       *tool,
							  GdkEventKey    *kevent,
							  GDisplay       *gdisp);

static void      gimp_edit_selection_tool_draw           (GimpDrawTool   *tool);


static GimpDrawToolClass *parent_class = NULL;


GtkType
gimp_edit_selection_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpEditSelectionTool",
        sizeof (GimpEditSelectionTool),
        sizeof (GimpEditSelectionToolClass),
        (GtkClassInitFunc) gimp_edit_selection_tool_class_init,
        (GtkObjectInitFunc) gimp_edit_selection_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_DRAW_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;
  draw_class   = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAW_TOOL);

  object_class->destroy      = gimp_edit_selection_tool_destroy;

  tool_class->control        = gimp_edit_selection_tool_control;
  tool_class->button_release = gimp_edit_selection_tool_button_release;
  tool_class->motion         = gimp_edit_selection_tool_motion;
  tool_class->cursor_update  = gimp_edit_selection_tool_cursor_update;

  draw_class->draw	     = gimp_edit_selection_tool_draw;
}

static void
gimp_edit_selection_tool_init (GimpEditSelectionTool *edit_selection_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (edit_selection_tool);

  tool->scroll_lock               = EDIT_SELECT_SCROLL_LOCK;
  tool->auto_snap_to              = FALSE;

  edit_selection_tool->origx      = 0;
  edit_selection_tool->origy      = 0;

  edit_selection_tool->cumlx      = 0;
  edit_selection_tool->cumly      = 0;

  edit_selection_tool->first_move = TRUE;
}

static void
gimp_edit_selection_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_edit_selection_tool_snap (GimpEditSelectionTool *edit_select,
			       GDisplay              *gdisp,
			       gdouble                x,
			       gdouble                y)
{
  gdouble x1, y1;
  gdouble x2, y2;
  gdouble dx, dy;

  gdisplay_untransform_coords_f (gdisp, x, y, &x, &y, TRUE);

  dx = x - edit_select->origx;
  dy = y - edit_select->origy;
  
  x1 = edit_select->x1 + dx;
  y1 = edit_select->y1 + dy;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      x2 = edit_select->x2 + dx;
      y2 = edit_select->y2 + dy;
  
      gdisplay_transform_coords_f (gdisp, x1, y1, &x1, &y1, TRUE);
      gdisplay_transform_coords_f (gdisp, x2, y2, &x2, &y2, TRUE);
      
      gdisplay_snap_rectangle (gdisp, x1, y1, x2, y2, &x1, &y1);
      
      gdisplay_untransform_coords_f (gdisp, x1, y1, &x1, &y1, TRUE);
    }
  
  edit_select->x = (gint) RINT (x1) - (edit_select->x1 - edit_select->origx);
  edit_select->y = (gint) RINT (y1) - (edit_select->y1 - edit_select->origy);
}

void
init_edit_selection (GimpTool       *tool,
		     GDisplay       *gdisp,
		     GdkEventButton *bevent,
		     EditType        edit_type)
{
  GimpEditSelectionTool *edit_select;
  gint                   x, y;

  edit_select = gtk_type_new (GIMP_TYPE_EDIT_SELECTION_TOOL);

  undo_push_group_start (gdisp->gimage, LAYER_DISPLACE_UNDO);

  /*  Move the (x, y) point from screen to image space  */
  gdisplay_untransform_coords (gdisp, 
			       bevent->x, bevent->y, &x, &y, FALSE, TRUE);

  edit_select->x = edit_select->origx = x;
  edit_select->y = edit_select->origy = y;

  gimage_mask_boundary (gdisp->gimage,
			&edit_select->segs_in, 
			&edit_select->segs_out,
			&edit_select->num_segs_in, 
			&edit_select->num_segs_out);

  /*  Make a check to see if it should be a floating selection translation  */
  if (edit_type == EDIT_MASK_TO_LAYER_TRANSLATE &&
      gimp_image_floating_sel (gdisp->gimage))
    {
      edit_type = EDIT_FLOATING_SEL_TRANSLATE;
    }

  if (edit_type == EDIT_LAYER_TRANSLATE)
    {
      GimpLayer *layer;

      layer = gimp_image_get_active_layer (gdisp->gimage);

      if (gimp_layer_is_floating_sel (layer))
	edit_type = EDIT_FLOATING_SEL_TRANSLATE;
    }

  edit_select->edit_type = edit_type;

  /*  find the bounding box of the selection mask -
   *  this is used for the case of a EDIT_MASK_TO_LAYER_TRANSLATE,
   *  where the translation will result in floating the selection
   *  mask and translating the resulting layer
   */
  gimp_drawable_mask_bounds (gimp_image_active_drawable (gdisp->gimage),
			     &edit_select->x1, &edit_select->y1,
			     &edit_select->x2, &edit_select->y2);

  gimp_edit_selection_tool_snap (edit_select, gdisp, bevent->x, bevent->y);

  GIMP_TOOL (edit_select)->gdisp = gdisp;
  GIMP_TOOL (edit_select)->state = ACTIVE;

  gtk_object_ref (GTK_OBJECT (edit_select));

  tool_manager_push_tool (gdisp->gimage->gimp,
			  GIMP_TOOL (edit_select));

  /*  pause the current selection  */
  gdisplay_selection_visibility (gdisp, SELECTION_PAUSE);

  /* initialize the statusbar display */
  edit_select->context_id 
    = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), 
				    "edit_select");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), 
		      edit_select->context_id, 
		      _("Move: 0, 0"));

  gimp_draw_tool_start (GIMP_DRAW_TOOL (edit_select),
			gdisp->canvas->window);
}


static void
gimp_edit_selection_tool_button_release (GimpTool       *tool,
					 GdkEventButton *bevent,
					 GDisplay       *gdisp)
{
  GimpEditSelectionTool *edit_select;
  gint                   x;
  gint                   y;
  GimpLayer             *layer;

  edit_select = GIMP_EDIT_SELECTION_TOOL (tool);

  /*  resume the current selection and ungrab the pointer  */
  gdisplay_selection_visibility (gdisp, SELECTION_RESUME);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), edit_select->context_id);

  /*  Stop and free the selection core  */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (gdisp->gimage->gimp);

  tool_manager_get_active (gdisp->gimage->gimp)->state = INACTIVE;

  /* EDIT_MASK_TRANSLATE is performed here at movement end, not 'live' like
   *  the other translation types.
   */
  if (edit_select->edit_type == EDIT_MASK_TRANSLATE)
    {
      gimp_edit_selection_tool_snap (edit_select, gdisp, bevent->x, bevent->y);

      x = edit_select->x;
      y = edit_select->y;

      /* move the selection -- whether there has been net movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimage_mask_translate (gdisp->gimage, 
			     edit_select->cumlx,
			     edit_select->cumly);

      if (edit_select->first_move)
	{
	  gimp_image_undo_freeze (gdisp->gimage);
	  edit_select->first_move = FALSE;
	}
    }

  /* thaw the undo again */
  gimp_image_undo_thaw (gdisp->gimage);

  if (edit_select->cumlx == 0 && edit_select->cumly == 0)
    {
      /* The user either didn't actually move the selection,
	 or moved it around and eventually just put it back in
	 exactly the same spot. */

     /*  If no movement occured and the type is EDIT_FLOATING_SEL_TRANSLATE,
	  check if the layer is a floating selection.  If so, anchor. */
      if (edit_select->edit_type == EDIT_FLOATING_SEL_TRANSLATE)
	{
	  layer = gimp_image_get_active_layer (gdisp->gimage);

	  if (gimp_layer_is_floating_sel (layer))
	    floating_sel_anchor (layer);
	}
    }
  else
    {
      path_transform_xy (gdisp->gimage, edit_select->cumlx, edit_select->cumly);

      layer = gimp_image_get_active_layer (gdisp->gimage);

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
    }

  undo_push_group_end (gdisp->gimage);

  if (bevent->state & GDK_BUTTON3_MASK) /* OPERATION CANCELLED */
    {
      /* Operation cancelled - undo the undo-group! */
      undo_pop (gdisp->gimage);
    }

  gdisplays_flush ();

  gtk_object_unref (GTK_OBJECT (edit_select));
}


static void
gimp_edit_selection_tool_motion (GimpTool       *tool,
				 GdkEventMotion *mevent,
				 GDisplay       *gdisp)
{
  GimpEditSelectionTool *edit_select;
  gchar                  offset[STATUSBAR_SIZE];
  gdouble                lastmotion_x, lastmotion_y;

  edit_select = GIMP_EDIT_SELECTION_TOOL (tool);

  if (tool->state != ACTIVE)
    {
      g_warning ("BUG: Tracking motion while !ACTIVE");
      return;
    }

  gdk_flush ();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* Perform motion compression so that we don't lag and/or waste time. */

  if (! gtkutil_compress_motion (gtk_get_event_widget ((GdkEvent *) mevent),
				 &lastmotion_x,
				 &lastmotion_y))
    {
      lastmotion_x = mevent->x;
      lastmotion_y = mevent->y;
    }

  /* now do the actual move. */

  gimp_edit_selection_tool_snap (edit_select,
				 gdisp,
				 RINT (lastmotion_x),
				 RINT (lastmotion_y));

  /******************************************* adam's live move *******/
  /********************************************************************/
  {
    gint       x, y;
    GimpLayer *layer;
    GimpLayer *floating_layer;
    GList     *layer_list;

    x = edit_select->x;
    y = edit_select->y;

    /* if there has been movement, move the selection  */
    if (edit_select->origx != x || edit_select->origy != y)
      {
	gint xoffset, yoffset;
	
	xoffset = x - edit_select->origx;
	yoffset = y - edit_select->origy;

	edit_select->cumlx += xoffset;
	edit_select->cumly += yoffset;

	switch (edit_select->edit_type)
	  {
	  case EDIT_MASK_TRANSLATE:
	    /*  we don't do the actual edit selection move here.  */
	    edit_select->origx = x;
	    edit_select->origy = y;
	    break;
	
	  case EDIT_LAYER_TRANSLATE:
	    if ((floating_layer = gimp_image_floating_sel (gdisp->gimage)))
	      floating_sel_relax (floating_layer, TRUE);
      
	    /*  translate the layer--and any "linked" layers as well  */
	    for (layer_list = GIMP_LIST (gdisp->gimage->layers)->list;
		 layer_list;
		 layer_list = g_list_next (layer_list))
	      {
		layer = (GimpLayer *) layer_list->data;

		if (layer == gdisp->gimage->active_layer || 
		    gimp_layer_get_linked (layer))
		  {
		    gimp_layer_translate (layer, xoffset, yoffset);
		  }
	      }
      
	    if (floating_layer)
	      floating_sel_rigor (floating_layer, TRUE);

	    if (edit_select->first_move)
	      {
		gimp_image_undo_freeze (gdisp->gimage);
		edit_select->first_move = FALSE;
	      }
	    break;
	
	  case EDIT_MASK_TO_LAYER_TRANSLATE:
	    if (! gimage_mask_float (gdisp->gimage, 
				     gimp_image_active_drawable (gdisp->gimage),
				     0, 0))
	      {
		/* no region to float, abort safely */
		gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

		return;
	      }

	    /* this is always the first move, since we switch to 
	       EDIT_FLOATING_SEL_TRANSLATE when finished here */
	    gimp_image_undo_freeze (gdisp->gimage);
	    edit_select->first_move = FALSE; 

	    edit_select->origx -= edit_select->x1;
	    edit_select->origy -= edit_select->y1;
	    edit_select->x2    -= edit_select->x1;
	    edit_select->y2    -= edit_select->y1;
	    edit_select->x1     = 0;
	    edit_select->y1     = 0;

	    edit_select->edit_type = EDIT_FLOATING_SEL_TRANSLATE;
	    break;
      
	  case EDIT_FLOATING_SEL_TRANSLATE:
	    layer = gimp_image_get_active_layer (gdisp->gimage);
      
	    floating_sel_relax (layer, TRUE);
	    gimp_layer_translate (layer, xoffset, yoffset);
	    floating_sel_rigor (layer, TRUE);

	    if (edit_select->first_move)
	      {
		gimp_image_undo_freeze (gdisp->gimage);

		edit_select->first_move = FALSE;
	      }
	    break;

	  default:
	    g_warning ("esm / BAD FALLTHROUGH");
	  }
      }

    gdisplay_flush (gdisp);
  }
  /********************************************************************/
  /********************************************************************/
  
  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), edit_select->context_id);

  if (gdisp->dot_for_dot)
    {
      g_snprintf (offset, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Move: "),
		  edit_select->cumlx,
		  ", ",
		  edit_select->cumly);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (offset, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Move: "), 
		  (edit_select->cumlx) * unit_factor / 
		  gdisp->gimage->xresolution,
		  ", ",
		  (edit_select->cumly) * unit_factor /
		  gdisp->gimage->yresolution);
    }

  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), edit_select->context_id,
		      offset);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
selection_transform_segs (GimpEditSelectionTool *edit_select,
			  GDisplay              *gdisp,
			  BoundSeg              *src_segs,
			  GdkSegment            *dest_segs,
			  gint                   num_segs)
{
  gint x, y;
  gint i;

  for (i = 0; i < num_segs; i++)
    {
      gdisplay_transform_coords (gdisp, 
				 src_segs[i].x1 + edit_select->cumlx, 
				 src_segs[i].y1 + edit_select->cumly,
				 &x, &y, FALSE);

      dest_segs[i].x1 = x;
      dest_segs[i].y1 = y;

      gdisplay_transform_coords (gdisp, 
				 src_segs[i].x2 + edit_select->cumlx, 
				 src_segs[i].y2 + edit_select->cumly,
				 &x, &y, FALSE);

      dest_segs[i].x2 = x;
      dest_segs[i].y2 = y;
    }
}

static void
gimp_edit_selection_tool_draw (GimpDrawTool *draw_tool)
{
  GimpEditSelectionTool *edit_select;
  GimpTool              *tool;
  GDisplay              *gdisp;
  Selection             *select;
  GimpLayer             *layer;
  GList                 *layer_list;
  gboolean               floating_sel;
  gint                   x1, y1, x2, y2;
  gint                   x3, y3, x4, y4;
  gint                   off_x, off_y;
  GdkSegment            *segs_copy;

  edit_select = GIMP_EDIT_SELECTION_TOOL (draw_tool);
  tool        = GIMP_TOOL (draw_tool);

  gdisp  = tool->gdisp;
  select = gdisp->select;

  switch (edit_select->edit_type)
    {
    case EDIT_MASK_TRANSLATE:
      layer        = gimp_image_get_active_layer (gdisp->gimage);
      floating_sel = gimp_layer_is_floating_sel (layer);

      if (! floating_sel)
	{
	   segs_copy = g_new (GdkSegment, edit_select->num_segs_in);

	   selection_transform_segs (edit_select,
				     gdisp,
				     edit_select->segs_in,
				     segs_copy,
				     edit_select->num_segs_in);
      
	   /*  Draw the items  */
	   gdk_draw_segments (draw_tool->win, draw_tool->gc,
			      segs_copy, select->num_segs_in);

	   g_free (segs_copy);
	}

      segs_copy = g_new (GdkSegment, edit_select->num_segs_out);
      
      selection_transform_segs (edit_select,
				gdisp,
				edit_select->segs_out,
				segs_copy,
				edit_select->num_segs_out);

      /*  Draw the items  */
      gdk_draw_segments (draw_tool->win, draw_tool->gc,
			 segs_copy, select->num_segs_out);

      g_free (segs_copy);
      break;

    case EDIT_MASK_TO_LAYER_TRANSLATE:
      gdisplay_transform_coords (gdisp, 
				 edit_select->x1, edit_select->y1, 
				 &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp, 
				 edit_select->x2, edit_select->y2, 
				 &x2, &y2, TRUE);
      gdk_draw_rectangle (draw_tool->win,
			  draw_tool->gc, 0,
			  x1, y1,
			  x2 - x1 + 1, y2 - y1 + 1);
      break;

    case EDIT_LAYER_TRANSLATE:
      gdisplay_transform_coords (gdisp, 0, 0, &x1, &y1, TRUE);
      gdisplay_transform_coords
	(gdisp,
	 gimp_drawable_width (GIMP_DRAWABLE (gdisp->gimage->active_layer)),
	 gimp_drawable_height (GIMP_DRAWABLE (gdisp->gimage->active_layer)),
	 &x2, &y2, TRUE);

      /*  Now, expand the rectangle to include all linked layers as well  */
      for (layer_list= GIMP_LIST (gdisp->gimage->layers)->list;
	   layer_list;
	   layer_list = g_list_next (layer_list))
	{
	  layer = (GimpLayer *) layer_list->data;

	  if (((layer) != gdisp->gimage->active_layer) &&
	      gimp_layer_get_linked (layer))
	    {
	      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
	      gdisplay_transform_coords (gdisp, off_x, off_y, &x3, &y3, FALSE);
	      gdisplay_transform_coords
		(gdisp,
		 off_x + gimp_drawable_width (GIMP_DRAWABLE (layer)),
		 off_y + gimp_drawable_height (GIMP_DRAWABLE (layer)),
		 &x4, &y4, FALSE);

	      if (x3 < x1)
		x1 = x3;
	      if (y3 < y1)
		y1 = y3;
	      if (x4 > x2)
		x2 = x4;
	      if (y4 > y2)
		y2 = y4;
	    }
	}

      gdk_draw_rectangle (draw_tool->win,
			  draw_tool->gc, 0,
			  x1, y1,
			  x2 - x1, y2 - y1);
      break;

    case EDIT_FLOATING_SEL_TRANSLATE:
      segs_copy = g_new (GdkSegment, edit_select->num_segs_in);

      /* The selection segs are in image space convert these 
       * to display space.
       * Takes care of offset/zoom etc etc.
       */

      selection_transform_segs (edit_select,
				gdisp,
				edit_select->segs_in,
				segs_copy,
				edit_select->num_segs_in);

      /*  Draw the items  */
      gdk_draw_segments (draw_tool->win, draw_tool->gc,
			 segs_copy, select->num_segs_in);

      g_free (segs_copy);
      break;
    }
}


static void
gimp_edit_selection_tool_control (GimpTool   *tool,
				  ToolAction  action,
				  GDisplay   *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}


static void
gimp_edit_selection_tool_cursor_update (GimpTool       *tool,
					GdkEventMotion *mevent,
					GDisplay       *gdisp)
{
  gdisplay_install_tool_cursor (gdisp,
				GIMP_MOUSE_CURSOR,
				GIMP_TOOL_CURSOR_NONE,
				GIMP_CURSOR_MODIFIER_MOVE);
}

static gint
process_event_queue_keys (GdkEventKey *kevent, 
			  ...)
/* GdkKeyType, GdkModifierType, value ... 0 
 * could move this function to a more central location so it can be used
 * by other tools? */
{
#define FILTER_MAX_KEYS 50
  va_list          argp;
  GdkEvent        *event;
  GList           *event_list = NULL;
  GList           *list;
  guint            keys[FILTER_MAX_KEYS];
  GdkModifierType  modifiers[FILTER_MAX_KEYS];
  gint             values[FILTER_MAX_KEYS];
  gint             i = 0, nkeys = 0, value = 0;
  gboolean         done = FALSE;
  gboolean         discard_event;
  GtkWidget        *orig_widget;

  va_start (argp, kevent);
  while (nkeys <FILTER_MAX_KEYS && (keys[nkeys] = va_arg (argp, guint)) != 0)
    {
      modifiers[nkeys] = va_arg (argp, GdkModifierType);
      values[nkeys]    = va_arg (argp, gint);
      nkeys++;
    }
  va_end(argp);

  for (i = 0; i < nkeys; i++)
    {
      if (kevent->keyval == keys[i] && kevent->state == modifiers[i])
	value += values[i];
    }

  orig_widget = gtk_get_event_widget ((GdkEvent*) kevent);

  while (gdk_events_pending () > 0 && !done)
  {
    discard_event = FALSE;
    event = gdk_event_get ();

    if (!event || orig_widget != gtk_get_event_widget (event))
      {
	done = TRUE;
      }
    else
      {
	if (event->any.type == GDK_KEY_PRESS)
	  {
	    for (i = 0; i < nkeys; i++)
	      if (event->key.keyval == keys[i] &&
		  event->key.state  == modifiers[i])
		{
		  discard_event = TRUE;
		  value += values[i];
		}

	    if (!discard_event)
	      done = TRUE;
	  }
	     /* should there be more types here? */
	else if (event->any.type != GDK_KEY_RELEASE &&
		 event->any.type != GDK_MOTION_NOTIFY &&
		 event->any.type != GDK_EXPOSE)
	  done = FALSE;
      }

    if (!event)
      ; /* Do nothing */
    else if (!discard_event)
      event_list = g_list_prepend (event_list, event);
    else
      gdk_event_free (event);
  }

  event_list = g_list_reverse (event_list);

  /* unget the unused events and free the list */
  for (list = event_list;
       list;
       list = g_list_next (list))
    {
      gdk_event_put ((GdkEvent*)list->data);
      gdk_event_free ((GdkEvent*)list->data);
    }

  g_list_free (event_list);

  return value;
#undef FILTER_MAX_KEYS
}

void
gimp_edit_selection_tool_arrow_key (GimpTool    *tool,
				    GdkEventKey *kevent,
				    GDisplay    *gdisp)
{
  gint       inc_x, inc_y, mask_inc_x, mask_inc_y;
  GimpLayer *layer;
  GimpLayer *floating_layer;
  GList     *layer_list;
  EditType   edit_type;

  layer = NULL;

  inc_x = 
    process_event_queue_keys (kevent,
			      GDK_Left,  0,              -1, 
			      GDK_Left,  GDK_SHIFT_MASK, -1 * ARROW_VELOCITY, 
			      GDK_Right, 0,               1, 
			      GDK_Right, GDK_SHIFT_MASK,  1 * ARROW_VELOCITY, 
			      0);
  inc_y = 
    process_event_queue_keys (kevent,
			      GDK_Up,   0,              -1, 
			      GDK_Up,   GDK_SHIFT_MASK, -1 * ARROW_VELOCITY, 
			      GDK_Down, 0,               1, 
			      GDK_Down, GDK_SHIFT_MASK,  1 * ARROW_VELOCITY, 
			      0);
  
  mask_inc_x = 
    process_event_queue_keys (kevent,
			      GDK_Left,  GDK_MOD1_MASK,                    -1, 
			      GDK_Left,  (GDK_MOD1_MASK | GDK_SHIFT_MASK), -1 * ARROW_VELOCITY, 
			      GDK_Right, GDK_MOD1_MASK,                     1, 
			      GDK_Right, (GDK_MOD1_MASK | GDK_SHIFT_MASK),  1 * ARROW_VELOCITY, 
			      0);
  mask_inc_y = 
    process_event_queue_keys (kevent,
			      GDK_Up,   GDK_MOD1_MASK,                    -1, 
			      GDK_Up,   (GDK_MOD1_MASK | GDK_SHIFT_MASK), -1 * ARROW_VELOCITY, 
			      GDK_Down, GDK_MOD1_MASK,                     1, 
			      GDK_Down, (GDK_MOD1_MASK | GDK_SHIFT_MASK),  1 * ARROW_VELOCITY, 
			      0);

  if (inc_x == 0 && inc_y == 0  &&  mask_inc_x == 0 && mask_inc_y == 0)
    return;

  undo_push_group_start (gdisp->gimage, LAYER_DISPLACE_UNDO);

  if (mask_inc_x != 0 || mask_inc_y != 0)
    gimage_mask_translate (gdisp->gimage, mask_inc_x, mask_inc_y);

  if (inc_x != 0 || inc_y != 0)
    {
      layer = gimp_image_get_active_layer (gdisp->gimage);
 
      if (gimp_layer_is_floating_sel (layer))
	edit_type = EDIT_FLOATING_SEL_TRANSLATE;
      else
	edit_type = EDIT_LAYER_TRANSLATE;
      
      switch (edit_type)
	{
	case EDIT_MASK_TRANSLATE:
	case EDIT_MASK_TO_LAYER_TRANSLATE:
	  /*  this won't happen  */
	  break;

	case EDIT_LAYER_TRANSLATE:
	  
	  if ((floating_layer = gimp_image_floating_sel (gdisp->gimage)))
	    floating_sel_relax (floating_layer, TRUE);
	  
	  /*  translate the layer -- and any "linked" layers as well  */
	  for (layer_list = GIMP_LIST (gdisp->gimage->layers)->list;
	       layer_list; 
	       layer_list = g_list_next (layer_list))
	    {
	      layer = (GimpLayer *) layer_list->data;

	      if (((layer) == gdisp->gimage->active_layer) || 
		  gimp_layer_get_linked (layer))
		{
		  gimp_layer_translate (layer, inc_x, inc_y);
		}
	    }
	  
	  if (floating_layer)
	    floating_sel_rigor (floating_layer, TRUE);
	  
	  break;
	  
	case EDIT_FLOATING_SEL_TRANSLATE:
	  
	  floating_sel_relax (layer, TRUE);
	  
	  gimp_layer_translate (layer, inc_x, inc_y);
	  
	  floating_sel_rigor (layer, TRUE);
	  
	  break;
	}
    }

  undo_push_group_end (gdisp->gimage);
  gdisplays_flush ();
}

/***************************************************************/
/* gtkutil_compress_motion:

   This function walks the whole GDK event queue seeking motion events
   corresponding to the widget 'widget'.  If it finds any it will
   remove them from the queue, write the most recent motion offset
   to 'lastmotion_x' and 'lastmotion_y', then return TRUE.  Otherwise
   it will return FALSE and 'lastmotion_x' / 'lastmotion_y' will be
   untouched.
 */
/* The gtkutil_compress_motion function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
gboolean
gtkutil_compress_motion (GtkWidget *widget,
			 gdouble   *lastmotion_x,
			 gdouble   *lastmotion_y)
{
  GdkEvent *event;
  GList    *requeued_events = NULL;
  GList    *list;
  gboolean  success = FALSE;

  /* Move the entire GDK event queue to a private list, filtering
     out any motion events for the desired widget. */
  while (gdk_events_pending ())
    {
      event = gdk_event_get ();

      if (!event)
	{
	  /* Do nothing */
	}
      else if ((gtk_get_event_widget (event) == widget) &&
	       (event->any.type == GDK_MOTION_NOTIFY))
	{
	  *lastmotion_x = event->motion.x;
	  *lastmotion_y = event->motion.y;
	  
	  gdk_event_free (event);
	  success = TRUE;
	}
      else
	{
	  requeued_events = g_list_prepend (requeued_events, event);
	}
    }
  
  /* Replay the remains of our private event list back into the
     event queue in order. */

  requeued_events = g_list_reverse (requeued_events);

  for (list = requeued_events; list; list = g_list_next (list))
    {
      gdk_event_put ((GdkEvent*) list->data);
      gdk_event_free ((GdkEvent*) list->data);
    }
  
  g_list_free (requeued_events);

  return success;
}
