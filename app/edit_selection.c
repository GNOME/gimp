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
#include <stdlib.h>
#include <stdarg.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "draw_core.h"
#include "drawable.h"
#include "tools.h"
#include "edit_selection.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "undo.h"
#include "gimprc.h"
#include "paths_dialogP.h"

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

#define EDIT_SELECT_SCROLL_LOCK FALSE
#define ARROW_VELOCITY          25
#define STATUSBAR_SIZE          128

typedef struct _EditSelection EditSelection;
struct _EditSelection
{
  gint                origx, origy;      /*  last x and y coords             */
  gint                cumlx, cumly;      /*  cumulative changes to x and yed */
  gint                x, y;              /*  current x and y coords          */

  gint                x1, y1;            /*  bounding box of selection mask  */
  gint                x2, y2;

  EditType            edit_type;         /*  translate the mask or layer?    */

  DrawCore *          core;              /* selection core for drawing bounds*/

  ButtonReleaseFunc   old_button_release;/*  old button press member func    */
  MotionFunc          old_motion;        /*  old motion member function      */
  ToolCtlFunc         old_control;       /*  old control member function     */
  CursorUpdateFunc    old_cursor_update; /*  old cursor update function      */
  gboolean            old_scroll_lock;   /*  old value of scroll lock        */
  gboolean            old_auto_snap_to;  /*  old value of auto snap to       */

  gboolean            first_move;        /*  we undo_freeze after the first  */

  guint               context_id;        /*  for the statusbar               */
};


/*  static EditSelection structure--there is ever only one present  */
static EditSelection edit_select;


static void
edit_selection_snap (GDisplay *gdisp,
		     gdouble   x,
		     gdouble   y)
{
  gdouble x1, y1;
  gdouble x2, y2;
  gdouble dx, dy;

  gdisplay_untransform_coords_f (gdisp, x, y, &x, &y, TRUE);

  dx = x - edit_select.origx;
  dy = y - edit_select.origy;
  
  x1 = edit_select.x1 + dx;
  y1 = edit_select.y1 + dy;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      x2 = edit_select.x2 + dx;
      y2 = edit_select.y2 + dy;
  
      gdisplay_transform_coords_f (gdisp, x1, y1, &x1, &y1, TRUE);
      gdisplay_transform_coords_f (gdisp, x2, y2, &x2, &y2, TRUE);
      
      gdisplay_snap_rectangle (gdisp, x1, y1, x2, y2, &x1, &y1);
      
      gdisplay_untransform_coords_f (gdisp, x1, y1, &x1, &y1, TRUE);
    }
  
  edit_select.x = (int)x1 - (edit_select.x1 - edit_select.origx);
  edit_select.y = (int)y1 - (edit_select.y1 - edit_select.origy);
}

void
init_edit_selection (Tool           *tool,
		     gpointer        gdisp_ptr,
		     GdkEventButton *bevent,
		     EditType        edit_type)
{
  GDisplay *gdisp;
  Layer *layer;
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  undo_push_group_start (gdisp->gimage, LAYER_DISPLACE_UNDO);

  /*  Move the (x, y) point from screen to image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, TRUE);

  edit_select.x = edit_select.origx = x;
  edit_select.y = edit_select.origy = y;

  edit_select.cumlx = 0;
  edit_select.cumly = 0;

  /*  Make a check to see if it should be a floating selection translation  */
  if (edit_type == MaskToLayerTranslate && gimage_floating_sel (gdisp->gimage))
    edit_type = FloatingSelTranslate;

  if (edit_type == LayerTranslate)
    {
      layer = gimage_get_active_layer (gdisp->gimage);
      if (layer_is_floating_sel (layer))
	edit_type = FloatingSelTranslate;
    }

  edit_select.edit_type = edit_type;

  edit_select.old_button_release = tool->button_release_func;
  edit_select.old_motion         = tool->motion_func;
  edit_select.old_control        = tool->control_func;
  edit_select.old_cursor_update  = tool->cursor_update_func;
  edit_select.old_scroll_lock    = tool->scroll_lock;
  edit_select.old_auto_snap_to   = tool->auto_snap_to;

  edit_select.first_move         = TRUE;

  /*  find the bounding box of the selection mask -
   *  this is used for the case of a MaskToLayerTranslate,
   *  where the translation will result in floating the selection
   *  mask and translating the resulting layer
   */
  drawable_mask_bounds (gimage_active_drawable (gdisp->gimage),
			&edit_select.x1, &edit_select.y1,
			&edit_select.x2, &edit_select.y2);

  edit_selection_snap (gdisp, bevent->x, bevent->y);

  /*  reset the function pointers on the selection tool  */
  tool->button_release_func = edit_selection_button_release;
  tool->motion_func         = edit_selection_motion;
  tool->control_func        = edit_selection_control;
  tool->cursor_update_func  = edit_selection_cursor_update;
  tool->scroll_lock         = EDIT_SELECT_SCROLL_LOCK;
  tool->auto_snap_to        = FALSE;

  /*  pause the current selection  */
  selection_pause (gdisp->select);

  /* initialize the statusbar display */
  edit_select.context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "edit_select");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), edit_select.context_id, _("Move: 0, 0"));

  /*  Create and start the selection core  */
  edit_select.core = draw_core_new (edit_selection_draw);
  draw_core_start (edit_select.core,
		   gdisp->canvas->window,
		   tool);
}


void
edit_selection_button_release (Tool           *tool,
			       GdkEventButton *bevent,
			       gpointer        gdisp_ptr)
{
  gint x, y;
  GDisplay * gdisp;
  Layer *layer;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  resume the current selection and ungrab the pointer  */
  selection_resume (gdisp->select);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), edit_select.context_id);

  /*  Stop and free the selection core  */
  draw_core_stop (edit_select.core, tool);
  draw_core_free (edit_select.core);
  edit_select.core = NULL;
  tool->state      = INACTIVE;

  tool->button_release_func = edit_select.old_button_release;
  tool->motion_func         = edit_select.old_motion;
  tool->control_func        = edit_select.old_control;
  tool->cursor_update_func  = edit_select.old_cursor_update;
  tool->scroll_lock         = edit_select.old_scroll_lock;
  tool->auto_snap_to        = edit_select.old_auto_snap_to;

  /* MaskTranslate is performed here at movement end, not 'live' like
   *  the other translation types.
   */
  if (edit_select.edit_type == MaskTranslate)
    {
      edit_selection_snap (gdisp, bevent->x, bevent->y);
      x = edit_select.x;
      y = edit_select.y;
      
      /* move the selection -- whether there has been net movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimage_mask_translate (gdisp->gimage, 
			     edit_select.cumlx,
			     edit_select.cumly);
      
      if (edit_select.first_move)
	{
	  gimp_image_undo_freeze (gdisp->gimage);
	  edit_select.first_move = FALSE;
	}
    }
  
  /* thaw the undo again */
  gimp_image_undo_thaw (gdisp->gimage);

  if (edit_select.cumlx == 0 && edit_select.cumly == 0)
    {
      /* The user either didn't actually move the selection,
	 or moved it around and eventually just put it back in
	 exactly the same spot. */

     /*  If no movement occured and the type is FloatingSelTranslate,
	  check if the layer is a floating selection.  If so, anchor. */
      if (edit_select.edit_type == FloatingSelTranslate)
	{
	  layer = gimage_get_active_layer (gdisp->gimage);
	  if (layer_is_floating_sel (layer))
	    floating_sel_anchor (layer);
	}
    }
  else
    {
      paths_transform_xy (gdisp->gimage, edit_select.cumlx, edit_select.cumly);
    }
    
  undo_push_group_end (gdisp->gimage);

  if (bevent->state & GDK_BUTTON3_MASK) /* OPERATION CANCELLED */
    {
      /* Operation cancelled - undo the undo-group! */
      undo_pop (gdisp->gimage);
    }

  gdisplays_flush ();
}


void
edit_selection_motion (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  gchar offset[STATUSBAR_SIZE];

  if (tool->state != ACTIVE)
    {
      g_warning ("Tracking motion while !ACTIVE");
      return;
    }

  gdisp = (GDisplay *) gdisp_ptr;

  gdk_flush();

  draw_core_pause (edit_select.core, tool);

  edit_selection_snap (gdisp, mevent->x, mevent->y);

  /**********************************************adam hack*************/
  /********************************************************************/
  {
    gint x, y;
    Layer *layer;
    Layer *floating_layer;
    GSList *layer_list;

    x = edit_select.x;
    y = edit_select.y;

    /* if there has been movement, move the selection  */
    if (edit_select.origx != x || edit_select.origy != y)
      {
	gint xoffset, yoffset;
	
	xoffset = x - edit_select.origx;
	yoffset = y - edit_select.origy;

	edit_select.cumlx += xoffset;
	edit_select.cumly += yoffset;

	switch (edit_select.edit_type)
	  {
	  case MaskTranslate:
	    /*  we don't do the actual edit selection move here.  */
	    edit_select.origx = x;
	    edit_select.origy = y;
	    break;
	
	  case LayerTranslate:
	    if ((floating_layer = gimage_floating_sel (gdisp->gimage)))
	      floating_sel_relax (floating_layer, TRUE);
      
	    /*  translate the layer--and any "linked" layers as well  */
	    layer_list = gdisp->gimage->layers;
	    while (layer_list)
	      {
		layer = (Layer *) layer_list->data;
		if (layer == gdisp->gimage->active_layer || 
		    layer_linked (layer))
		  {
		    layer_translate (layer, xoffset, yoffset);
		  }
		layer_list = g_slist_next (layer_list);
	      }
      
	    if (floating_layer)
	      floating_sel_rigor (floating_layer, TRUE);

	    if (edit_select.first_move)
	      {
		gimp_image_undo_freeze (gdisp->gimage);
		edit_select.first_move = FALSE;
	      }
	    break;
	
	  case MaskToLayerTranslate:
	    if (!gimage_mask_float (gdisp->gimage, 
				    gimage_active_drawable (gdisp->gimage),
				    0, 0))
	      {
		/* no region to float, abort safely */
		 draw_core_resume (edit_select.core, tool);
		 return;
	      }

	    /* this is always the first move, since we switch to 
	       FloatingSelTranslate when finished here */
	    gimp_image_undo_freeze (gdisp->gimage);
	    edit_select.first_move = FALSE; 

	    edit_select.origx -= edit_select.x1;
	    edit_select.origy -= edit_select.y1;
	    edit_select.x2 -= edit_select.x1;
	    edit_select.y2 -= edit_select.y1;
	    edit_select.x1 = 0;
	    edit_select.y1 = 0;

	    edit_select.edit_type = FloatingSelTranslate;
	    break;
      
	  case FloatingSelTranslate:
	    layer = gimage_get_active_layer (gdisp->gimage);
      
	    floating_sel_relax (layer, TRUE);
	    layer_translate (layer, xoffset, yoffset);
	    floating_sel_rigor (layer, TRUE);

	    if (edit_select.first_move)
	      {
		gimp_image_undo_freeze (gdisp->gimage);
		edit_select.first_move = FALSE;
	      }
	    break;

	  default:
	    g_warning ("esm / BAD FALLTHROUGH");
	  }
      }

    gdisplay_flush(gdisp);
  }
  /********************************************************************/
  /********************************************************************/
  

  gtk_statusbar_pop (GTK_STATUSBAR(gdisp->statusbar), edit_select.context_id);
  if (gdisp->dot_for_dot)
    {
      g_snprintf (offset, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Move: "),
		  edit_select.cumlx,
		  ", ",
		  edit_select.cumly);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (offset, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Move: "), 
		  (edit_select.cumlx) * unit_factor /
		  gdisp->gimage->xresolution,
		  ", ",
		  (edit_select.cumly) * unit_factor /
		  gdisp->gimage->yresolution);
    }
  gtk_statusbar_push (GTK_STATUSBAR(gdisp->statusbar), edit_select.context_id,
		      offset);

  draw_core_resume (edit_select.core, tool);
}


void
edit_selection_draw (Tool *tool)
{
  gint i;
  gint diff_x, diff_y;
  GDisplay * gdisp;
  GdkSegment * seg;
  Selection * select;
  Layer *layer;
  GSList *layer_list;
  gint floating_sel;
  gint x1, y1, x2, y2;
  gint x3, y3, x4, y4;
  gint off_x, off_y;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  select = gdisp->select;

  switch (edit_select.edit_type)
    {
    case MaskTranslate:
      layer = gimage_get_active_layer (gdisp->gimage);
      floating_sel = layer_is_floating_sel (layer);

      diff_x = SCALEX (gdisp, edit_select.cumlx);
      diff_y = SCALEY (gdisp, edit_select.cumly);

      /*  offset the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}
      seg = select->segs_out;
      for (i = 0; i < select->num_segs_out; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}

      if (! floating_sel)
	gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			   select->segs_in, select->num_segs_in);

      gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			 select->segs_out, select->num_segs_out);

      /*  reset the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      seg = select->segs_out;
      for (i = 0; i < select->num_segs_out; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      break;

    case MaskToLayerTranslate:
      gdisplay_transform_coords (gdisp, edit_select.x1, edit_select.y1, &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp, edit_select.x2, edit_select.y2, &x2, &y2, TRUE);
      gdk_draw_rectangle (edit_select.core->win,
			  edit_select.core->gc, 0,
			  x1, y1,
			  x2 - x1 + 1, y2 - y1 + 1);
      break;

    case LayerTranslate:
      gdisplay_transform_coords (gdisp, 0, 0, &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp,
				 drawable_width ( GIMP_DRAWABLE (gdisp->gimage->active_layer)),
				 drawable_height ( GIMP_DRAWABLE (gdisp->gimage->active_layer)),
				 &x2, &y2, TRUE);

      /*  Now, expand the rectangle to include all linked layers as well  */
      layer_list = gdisp->gimage->layers;
      while (layer_list)
	{
	  layer = (Layer *) layer_list->data;
	  if (((layer) != gdisp->gimage->active_layer) && layer_linked (layer))
	    {
	      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
	      gdisplay_transform_coords (gdisp, off_x, off_y, &x3, &y3, FALSE);
	      gdisplay_transform_coords (gdisp,
					 off_x + drawable_width (GIMP_DRAWABLE (layer)),
					 off_y + drawable_height (GIMP_DRAWABLE (layer)),
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
	  layer_list = g_slist_next (layer_list);
	}

      gdk_draw_rectangle (edit_select.core->win,
			  edit_select.core->gc, 0,
			  x1, y1,
			  x2 - x1, y2 - y1);
      break;

    case FloatingSelTranslate:
      diff_x = SCALEX (gdisp, edit_select.cumlx);
      diff_y = SCALEY (gdisp, edit_select.cumly);

     seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}

      /*  Draw the items  */
      gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			 select->segs_in, select->num_segs_in);

      /*  reset the the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      break;
    }
}


void
edit_selection_control (Tool       *tool,
			ToolAction  action,
			gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      draw_core_pause (edit_select.core, tool);
      break;

    case RESUME :
      draw_core_resume (edit_select.core, tool);
      break;

    case HALT :
      draw_core_stop (edit_select.core, tool);
      draw_core_free (edit_select.core);
      break;

    default:
      break;
    }
}


void
edit_selection_cursor_update (Tool           *tool,
			      GdkEventMotion *mevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
}

static gint
process_event_queue_keys (GdkEventKey *kevent, ...)
/* GdkKeyType, GdkModifierType, value ... 0 
 * could move this function to a more central location so it can be used
 * by other tools? */
{
#define FILTER_MAX_KEYS 50
  va_list argp;
  GdkEvent *event;
  GList *list = NULL;
  guint keys[FILTER_MAX_KEYS];
  GdkModifierType modifiers[FILTER_MAX_KEYS];
  gint values[FILTER_MAX_KEYS];
  gint i = 0, nkeys = 0, value = 0, done = 0, discard_event;
  GtkWidget *orig_widget;

  va_start(argp, kevent);
  while (nkeys <FILTER_MAX_KEYS && (keys[nkeys] = va_arg (argp, guint)) != 0)
  {
    modifiers[nkeys] = va_arg (argp, GdkModifierType);
    values[nkeys]    = va_arg (argp, int);
    nkeys++;
  }
  va_end(argp);

  for (i = 0; i < nkeys; i++)
    if (kevent->keyval == keys[i] && kevent->state == modifiers[i])
      value += values[i];

  orig_widget = gtk_get_event_widget((GdkEvent*)kevent);

  while (gdk_events_pending() > 0 && !done)
  {
    discard_event = 0;
    event = gdk_event_get();
    if (orig_widget != gtk_get_event_widget(event))
    {
      done = 1;
    }
    else
    {
      if (event->any.type == GDK_KEY_PRESS)
      {
	for (i = 0; i < nkeys; i++)
	  if (event->key.keyval == keys[i] &&
	      event->key.state  == modifiers[i])
	  {
	    discard_event = 1;
	    value += values[i];
	  }
	if (!discard_event)
	  done = 1;
      }
	     /* should there be more types here? */
      else if (event->any.type != GDK_KEY_RELEASE &&
	       event->any.type != GDK_MOTION_NOTIFY &&
	       event->any.type != GDK_EXPOSE)
	done = 1;
    }

    if (!discard_event)
      list = g_list_prepend(list, event);
    else
      gdk_event_free(event);
  }
  while (list) /* unget the unused events and free the list */
  {
    gdk_event_put((GdkEvent*)list->data);
    gdk_event_free((GdkEvent*)list->data);
    list = g_list_remove_link (list, list);
  }
  return value;
#undef FILTER_MAX_KEYS
}

void
edit_sel_arrow_keys_func (Tool        *tool,
			  GdkEventKey *kevent,
			  gpointer     gdisp_ptr)
{
  gint inc_x, inc_y, mask_inc_x, mask_inc_y;
  GDisplay *gdisp;
  Layer *layer;
  Layer *floating_layer;
  GSList *layer_list;
  EditType edit_type;

  layer = NULL;

  gdisp = (GDisplay *) gdisp_ptr;

  inc_x = process_event_queue_keys(kevent,
			    GDK_Left,  0,              -1, 
			    GDK_Left,  GDK_SHIFT_MASK, -1*ARROW_VELOCITY, 
			    GDK_Right, 0,               1, 
			    GDK_Right, GDK_SHIFT_MASK,  ARROW_VELOCITY, 
			    0);
  inc_y = process_event_queue_keys(kevent,
			    GDK_Up,   0,              -1, 
			    GDK_Up,   GDK_SHIFT_MASK, -1*ARROW_VELOCITY, 
			    GDK_Down, 0,               1, 
			    GDK_Down, GDK_SHIFT_MASK,   ARROW_VELOCITY, 
			    0);

  mask_inc_x = process_event_queue_keys(kevent,
				 GDK_Left,  GDK_MOD1_MASK,              -1, 
				 GDK_Left,  (GDK_MOD1_MASK | GDK_SHIFT_MASK),
				            -1*ARROW_VELOCITY, 
				 GDK_Right, GDK_MOD1_MASK,               1, 
				 GDK_Right, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
				            ARROW_VELOCITY, 
				 0);
  mask_inc_y = process_event_queue_keys(kevent,
				 GDK_Up,   GDK_MOD1_MASK,              -1, 
				 GDK_Up,   (GDK_MOD1_MASK | GDK_SHIFT_MASK),
				            -1*ARROW_VELOCITY, 
				 GDK_Down, GDK_MOD1_MASK,               1, 
				 GDK_Down, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
				            ARROW_VELOCITY, 
				 0);
  if (inc_x == 0 && inc_y == 0  &&  mask_inc_x == 0 && mask_inc_y == 0)
    return;

  undo_push_group_start (gdisp->gimage, LAYER_DISPLACE_UNDO);

  if (mask_inc_x != 0 || mask_inc_y != 0)
    gimage_mask_translate (gdisp->gimage, mask_inc_x, mask_inc_y);

  if (inc_x != 0 || inc_y != 0)
  {
    layer = gimage_get_active_layer (gdisp->gimage);
    if (layer_is_floating_sel (layer))
      edit_type = FloatingSelTranslate;
    else
      edit_type = LayerTranslate;

    switch (edit_type)
    {
     case MaskToLayerTranslate:
       gimage_mask_float (gdisp->gimage,
			  gimage_active_drawable (gdisp->gimage),
			  inc_x, inc_y);
       break;

     case LayerTranslate:
  
       if ((floating_layer = gimage_floating_sel (gdisp->gimage)))
	 floating_sel_relax (floating_layer, TRUE);

       /*  translate the layer--and any "linked" layers as well  */
       layer_list = gdisp->gimage->layers;
       while (layer_list)
       {
	 layer = (Layer *) layer_list->data;
	 if (((layer) == gdisp->gimage->active_layer) || layer_linked (layer))
	   layer_translate (layer, inc_x, inc_y);
	 layer_list = g_slist_next (layer_list);
       }

       if (floating_layer)
	 floating_sel_rigor (floating_layer, TRUE);

       break;

     case FloatingSelTranslate:

       floating_sel_relax (layer, TRUE);

       layer_translate (layer, inc_x, inc_y);

       floating_sel_rigor (layer, TRUE);

       break;

     default:
       /*  this won't occur  */
       break;
    }
  }
  undo_push_group_end (gdisp->gimage);
  gdisplays_flush ();
}
