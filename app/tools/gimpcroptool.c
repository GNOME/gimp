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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "cursorutil.h"
#include "draw_core.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "crop.h"
#include "info_dialog.h"
#include "undo.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpsizeentry.h"

#define STATUSBAR_SIZE 128

typedef struct _crop Crop;

struct _crop
{
  DrawCore *      core;       /*  Core select object          */

  int             startx;     /*  starting x coord            */
  int             starty;     /*  starting y coord            */

  int             lastx;      /*  previous x coord            */
  int             lasty;      /*  previous y coord            */

  int             x1, y1;     /*  upper left hand coordinate  */
  int             x2, y2;     /*  lower right hand coords     */

  int             srw, srh;   /*  width and height of corners */

  int             tx1, ty1;   /*  transformed coords          */
  int             tx2, ty2;   /*                              */

  int             function;   /*  moving or resizing          */
  guint           context_id; /*  for the statusbar           */
};

/* possible crop functions */
#define CREATING        0
#define MOVING          1
#define RESIZING_LEFT   2
#define RESIZING_RIGHT  3
#define CROPPING        4

/* speed of key movement */
#define ARROW_VELOCITY 25

static InfoDialog * crop_info = NULL;
static gfloat       orig_vals[2];
static gfloat       size_vals[2];

/*  crop action functions  */
static void crop_button_press       (Tool *, GdkEventButton *, gpointer);
static void crop_button_release     (Tool *, GdkEventButton *, gpointer);
static void crop_motion             (Tool *, GdkEventMotion *, gpointer);
static void crop_cursor_update      (Tool *, GdkEventMotion *, gpointer);
static void crop_control            (Tool *, int, gpointer);
static void crop_arrow_keys_func    (Tool *, GdkEventKey *, gpointer);


/*  Crop helper functions   */
static void crop_image              (GImage *gimage, int, int, int, int);
static void crop_recalc             (Tool *, Crop *);
static void crop_start              (Tool *, Crop *);
static void crop_adjust_guides      (GImage *, int, int, int, int);

/*  Crop dialog functions  */
static void crop_info_update        (Tool *);
static void crop_info_create        (Tool *);
static void crop_ok_callback        (GtkWidget *, gpointer);
static void crop_selection_callback (GtkWidget *, gpointer);
static void crop_close_callback     (GtkWidget *, gpointer);

/*  Crop dialog callback funtions  */
static void crop_orig_changed       (GtkWidget *, gpointer);
static void crop_size_changed       (GtkWidget *, gpointer);

/*  Options callbacks */
static void crop_checkbutton_update (GtkWidget *, gpointer);
			
static GtkWidget *options_widget = NULL;
static CropToolOptions options;

static Argument *crop_invoker (Argument *);


/*  Functions  */

static void
init_crop_options()
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *checkbutton;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the main label  */
  label = gtk_label_new (_("Crop Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* layer toggle */
  checkbutton = gtk_check_button_new_with_label(_("Current layer only"));
  gtk_box_pack_start(GTK_BOX(vbox), checkbutton,
		     FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(checkbutton), "toggled",
		     (GtkSignalFunc) crop_checkbutton_update,
		     &options.layer_only);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
			       options.layer_only);
  gtk_widget_show(checkbutton);

  /* Register this selection options widget with the main tools
   * options dialog */

  tools_register_options (CROP, vbox);
}

static void
crop_checkbutton_update (GtkWidget *w,
			 gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


static void
crop_button_press (Tool           *tool,
		   GdkEventButton *bevent,
		   gpointer        gdisp_ptr)
{
  Crop * crop;
  GDisplay * gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  crop = (Crop *) tool->private;

  if (tool->state == INACTIVE)
    crop->function = CREATING;
  else if (gdisp_ptr != tool->gdisp_ptr)
    crop->function = CREATING;
  else
    {
      /*  If the cursor is in either the upper left or lower right boxes,
	  The new function will be to resize the current crop area        */
      if (bevent->x == BOUNDS (bevent->x, crop->x1, crop->x1 + crop->srw) &&
	  bevent->y == BOUNDS (bevent->y, crop->y1, crop->y1 + crop->srh))
	crop->function = RESIZING_LEFT;
      else if (bevent->x == BOUNDS (bevent->x, crop->x2 - crop->srw, crop->x2) &&
	       bevent->y == BOUNDS (bevent->y, crop->y2 - crop->srh, crop->y2))
	crop->function = RESIZING_RIGHT;

      /*  If the cursor is in either the upper right or lower left boxes,
	  The new function will be to translate the current crop area     */
      else if  ((bevent->x == BOUNDS (bevent->x, crop->x1, crop->x1 + crop->srw) &&
		 bevent->y == BOUNDS (bevent->y, crop->y2 - crop->srh, crop->y2)) ||
		(bevent->x == BOUNDS (bevent->x, crop->x2 - crop->srw, crop->x2) &&
		 bevent->y == BOUNDS (bevent->y, crop->y1, crop->y1 + crop->srh)))
	crop->function = MOVING;

      /*  If the pointer is in the rectangular region, crop it!  */
      else if (bevent->x > crop->x1 && bevent->x < crop->x2 &&
	       bevent->y > crop->y1 && bevent->y < crop->y2)
	crop->function = CROPPING;

      /*  otherwise, the new function will be creating, since we want to start anew  */
      else
	crop->function = CREATING;
    }

  if (crop->function == CREATING)
    {
      if (tool->state == ACTIVE)
	draw_core_stop (crop->core, tool);

      tool->gdisp_ptr = gdisp_ptr;

      gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
				   &crop->tx1, &crop->ty1, TRUE, FALSE);
      crop->tx2 = crop->tx1;
      crop->ty2 = crop->ty1;

      crop_start (tool, crop);
    }

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &crop->startx, &crop->starty, TRUE, FALSE);
  crop->lastx = crop->startx;
  crop->lasty = crop->starty;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
}

static void
crop_button_release (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
  Crop * crop;
  GDisplay *gdisp;

  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR(gdisp->statusbar), crop->context_id);

  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      if (crop->function == CROPPING) 
	crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2);
      else
	{
	  crop_info_update (tool);
	  return;
	}
    }

  /*  Finish the tool  */
  draw_core_stop (crop->core, tool);
  info_dialog_popdown (crop_info);
  tool->state = INACTIVE;
}

static void
crop_adjust_guides (GImage *gimage,
                    int x1, int y1,
                    int x2, int y2)

{
GList * glist;
Guide * guide;
gint remove_guide;
/* initialize the traverser */
glist = gimage->guides;
while (glist  != NULL)  {
	guide = (Guide *) glist->data;
	remove_guide = FALSE;

	switch (guide->orientation) {
		case HORIZONTAL_GUIDE:
			if ((guide->position < y1) ||(guide->position > y2))
				remove_guide = TRUE;
		break;
		case VERTICAL_GUIDE:
			if ((guide->position < x1) ||(guide->position > x2))
				remove_guide = TRUE;
		break;
		}
	
		/* edit the guide */
	gdisplays_expose_guide (gimage, guide);
	gdisplays_flush();

	if (remove_guide) {
	  guide->position = -1;
          guide = NULL;
	}
	else {
		if (guide->orientation == HORIZONTAL_GUIDE) {
			guide->position -= y1 ;
			}
		else {
			guide->position -= x1;
 			}
		}
	glist = glist->next;	
	}
}
		 

static void
crop_motion (Tool           *tool,
	     GdkEventMotion *mevent,
	     gpointer        gdisp_ptr)
{
  Crop * crop;
  GDisplay * gdisp;
  int x1, y1, x2, y2;
  int curx, cury;
  int inc_x, inc_y;
  gchar size[STATUSBAR_SIZE];

  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to crop the image  */
  if (crop->function == CROPPING)
    return;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &curx, &cury, TRUE, FALSE);
  x1 = crop->startx;
  y1 = crop->starty;
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (crop->lastx == curx && crop->lasty == cury)
    return;

  draw_core_pause (crop->core, tool);

  switch (crop->function)
    {
    case CREATING :
      x1 = BOUNDS (x1, 0, gdisp->gimage->width);
      y1 = BOUNDS (y1, 0, gdisp->gimage->height);
      x2 = BOUNDS (x2, 0, gdisp->gimage->width);
      y2 = BOUNDS (y2, 0, gdisp->gimage->height);
      break;

    case RESIZING_LEFT :
      x1 = BOUNDS (crop->tx1 + inc_x, 0, gdisp->gimage->width);
      y1 = BOUNDS (crop->ty1 + inc_y, 0, gdisp->gimage->height);
      x2 = MAXIMUM (x1, crop->tx2);
      y2 = MAXIMUM (y1, crop->ty2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case RESIZING_RIGHT :
      x2 = BOUNDS (crop->tx2 + inc_x, 0, gdisp->gimage->width);
      y2 = BOUNDS (crop->ty2 + inc_y, 0, gdisp->gimage->height);
      x1 = MINIMUM (crop->tx1, x2);
      y1 = MINIMUM (crop->ty1, y2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case MOVING :
      inc_x = BOUNDS (inc_x, -crop->tx1, gdisp->gimage->width - crop->tx2);
      inc_y = BOUNDS (inc_y, -crop->ty1, gdisp->gimage->height - crop->ty2);
      x1 = crop->tx1 + inc_x;
      x2 = crop->tx2 + inc_x;
      y1 = crop->ty1 + inc_y;
      y2 = crop->ty2 + inc_y;
      crop->startx = curx;
      crop->starty = cury;
      break;
    }

  /*  make sure that the coords are in bounds  */
  crop->tx1 = MINIMUM (x1, x2);
  crop->ty1 = MINIMUM (y1, y2);
  crop->tx2 = MAXIMUM (x1, x2);
  crop->ty2 = MAXIMUM (y1, y2);

  crop->lastx = curx;
  crop->lasty = cury;

  /*  recalculate the coordinates for crop_draw based on the new values  */
  crop_recalc (tool, crop);

  if (crop->function == CREATING || 
      crop->function == RESIZING_LEFT || crop->function == RESIZING_RIGHT)
    {
      gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), crop->context_id);
      if (gdisp->dot_for_dot)
	{
	  g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		      _("Crop: "), 
		      (crop->tx2 - crop->tx1), " x ", (crop->ty2 - crop->ty1));
	}
      else /* show real world units */
	{
	  float unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
	  
	  g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		      _("Crop: "), 
		      (float)(crop->tx2 - crop->tx1) * unit_factor /
		      gdisp->gimage->xresolution,
		      " x ",
		      (float)(crop->ty2 - crop->ty1) * unit_factor /
		      gdisp->gimage->yresolution);
	}
      gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), crop->context_id,
			  size);
    }
  
  draw_core_resume (crop->core, tool);
}

static void
crop_cursor_update (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;
  GdkCursorType ctype;
  Crop * crop;

  gdisp = (GDisplay *) gdisp_ptr;
  crop = (Crop *) tool->private;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  if (tool->state == INACTIVE ||
      (tool->state == ACTIVE && tool->gdisp_ptr != gdisp_ptr))
    ctype = GDK_CROSS;
  else if (mevent->x == BOUNDS (mevent->x, crop->x1, crop->x1 + crop->srw) &&
      mevent->y == BOUNDS (mevent->y, crop->y1, crop->y1 + crop->srh))
    ctype = GDK_TOP_LEFT_CORNER;
  else if (mevent->x == BOUNDS (mevent->x, crop->x2 - crop->srw, crop->x2) &&
	   mevent->y == BOUNDS (mevent->y, crop->y2 - crop->srh, crop->y2))
    ctype = GDK_BOTTOM_RIGHT_CORNER;
  else if  (mevent->x == BOUNDS (mevent->x, crop->x1, crop->x1 + crop->srw) &&
	    mevent->y == BOUNDS (mevent->y, crop->y2 - crop->srh, crop->y2))
    ctype = GDK_FLEUR;
  else if  (mevent->x == BOUNDS (mevent->x, crop->x2 - crop->srw, crop->x2) &&
	    mevent->y == BOUNDS (mevent->y, crop->y1, crop->y1 + crop->srh))
    ctype = GDK_FLEUR;
  else if (mevent->x > crop->x1 && mevent->x < crop->x2 &&
	   mevent->y > crop->y1 && mevent->y < crop->y2)
    ctype = GDK_ICON;
  else
    ctype = GDK_CROSS;

  gdisplay_install_tool_cursor (gdisp, ctype);
}

static void
crop_arrow_keys_func (Tool        *tool,
		      GdkEventKey *kevent,
		      gpointer     gdisp_ptr)
{
  int inc_x, inc_y;
  GDisplay * gdisp;
  Crop * crop;

  gdisp = (GDisplay *) gdisp_ptr;

  if (tool->state == ACTIVE)
    {
      crop = (Crop *) tool->private;
      inc_x = inc_y = 0;

      switch (kevent->keyval)
	{
	case GDK_Up    : inc_y = -1; break;
	case GDK_Left  : inc_x = -1; break;
	case GDK_Right : inc_x =  1; break;
	case GDK_Down  : inc_y =  1; break;
	}

      /*  If the shift key is down, move by an accelerated increment  */
      if (kevent->state & GDK_SHIFT_MASK)
	{
	  inc_y *= ARROW_VELOCITY;
	  inc_x *= ARROW_VELOCITY;
	}

      draw_core_pause (crop->core, tool);

      if (kevent->state & GDK_CONTROL_MASK)  /* RESIZING */
	{
	  crop->tx2 = BOUNDS (crop->tx2 + inc_x, 0, gdisp->gimage->width);
	  crop->ty2 = BOUNDS (crop->ty2 + inc_y, 0, gdisp->gimage->height);
	  crop->tx1 = MINIMUM (crop->tx1, crop->tx2);
	  crop->ty1 = MINIMUM (crop->ty1, crop->ty2);
	}
      else
	{
	  inc_x = BOUNDS (inc_x, -crop->tx1, gdisp->gimage->width - crop->tx2);
	  inc_y = BOUNDS (inc_y, -crop->ty1, gdisp->gimage->height - crop->ty2);
	  crop->tx1 += inc_x;
	  crop->tx2 += inc_x;
	  crop->ty1 += inc_y;
	  crop->ty2 += inc_y;
	}

      crop_recalc (tool, crop);
      draw_core_resume (crop->core, tool);
    }
}

static void
crop_control (Tool     *tool,
	      int       action,
	      gpointer  gdisp_ptr)
{
  Crop * crop;

  crop = (Crop *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (crop->core, tool);
      break;
    case RESUME :
      crop_recalc (tool, crop);
      draw_core_resume (crop->core, tool);
      break;
    case HALT :
      draw_core_stop (crop->core, tool);
      info_dialog_popdown (crop_info);
      break;
    }
}

void
crop_draw (Tool *tool)
{
  Crop * crop;
  GDisplay * gdisp;

#define SRW 10
#define SRH 10

  gdisp = (GDisplay *) tool->gdisp_ptr;
  crop = (Crop *) tool->private;

  gdk_draw_line (crop->core->win, crop->core->gc,
		 crop->x1, crop->y1, gdisp->disp_width, crop->y1);
  gdk_draw_line (crop->core->win, crop->core->gc,
		 crop->x1, crop->y1, crop->x1, gdisp->disp_height);
  gdk_draw_line (crop->core->win, crop->core->gc,
		 crop->x2, crop->y2, 0, crop->y2);
  gdk_draw_line (crop->core->win, crop->core->gc,
		 crop->x2, crop->y2, crop->x2, 0);

  crop->srw = ((crop->x2 - crop->x1) < SRW) ? (crop->x2 - crop->x1) : SRW;
  crop->srh = ((crop->y2 - crop->y1) < SRH) ? (crop->y2 - crop->y1) : SRH;

  gdk_draw_rectangle (crop->core->win, crop->core->gc, 1,
		      crop->x1, crop->y1, crop->srw, crop->srh);
  gdk_draw_rectangle (crop->core->win, crop->core->gc, 1,
		      crop->x2 - crop->srw, crop->y2-crop->srh, crop->srw, crop->srh);
  gdk_draw_rectangle (crop->core->win, crop->core->gc, 1,
		      crop->x2 - crop->srw, crop->y1, crop->srw, crop->srh);
  gdk_draw_rectangle (crop->core->win, crop->core->gc, 1,
		      crop->x1, crop->y2-crop->srh, crop->srw, crop->srh);

  crop_info_update (tool);
}

Tool *
tools_new_crop ()
{
  Tool * tool;
  Crop * private;

  if (! options_widget)
    init_crop_options ();

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Crop *) g_malloc (sizeof (Crop));

  private->core = draw_core_new (crop_draw);
  private->startx = private->starty = 0;
  private->function = CREATING;

  tool->type = CROP;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;

  tool->button_press_func = crop_button_press;
  tool->button_release_func = crop_button_release;
  tool->motion_func = crop_motion;
  tool->arrow_keys_func = crop_arrow_keys_func;
  tool->cursor_update_func = crop_cursor_update;
  tool->control_func = crop_control;
  tool->preserve = TRUE;  /* XXX Check me */

  return tool;
}

void
tools_free_crop (Tool *tool)
{
  Crop * crop;

  crop = (Crop *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (crop->core, tool);

  draw_core_free (crop->core);

  if (crop_info)
    {
      info_dialog_popdown (crop_info);
      crop_info = NULL;
    }

  g_free (crop);
}

static void
crop_image (GImage *gimage,
	    int     x1,
	    int     y1,
	    int     x2,
	    int     y2)
{
  Layer *layer;
  Layer *floating_layer;
  Channel *channel;
  GList *guide_list_ptr;
  GSList *list;
  int width, height;
  int lx1, ly1, lx2, ly2;
  int off_x, off_y;
  int doff_x, doff_y;


  width = x2 - x1;
  height = y2 - y1;

  /*  Make sure new width and height are non-zero  */
  if (width && height)
  {
    gimp_add_busy_cursors();

    if (options.layer_only)
    {
      undo_push_group_start (gimage, LAYER_RESIZE_UNDO);

      layer = gimage->active_layer;

      if (layer_is_floating_sel (layer))
        floating_sel_relax (layer, TRUE);

      drawable_offsets (GIMP_DRAWABLE(layer), &doff_x, &doff_y);

      off_x = (doff_x - x1);
      off_y = (doff_y - y1);

      layer_resize (layer, width, height, off_x, off_y);

      if (layer_is_floating_sel (layer))
        floating_sel_rigor (layer, TRUE);

      undo_push_group_end (gimage);
    }
    else
    {
      floating_layer = gimage_floating_sel (gimage);

      undo_push_group_start (gimage, CROP_UNDO);

      /*  relax the floating layer  */
      if (floating_layer)
        floating_sel_relax (floating_layer, TRUE);

      /*  Push the image size to the stack  */
      undo_push_gimage_mod (gimage);

      /*  Set the new width and height  */
      gimage->width = width;
      gimage->height = height;

      /*  Resize all channels  */
      list = gimage->channels;
      while (list)
      {
	channel = (Channel *) list->data;
	channel_resize (channel, width, height, -x1, -y1);
	list = g_slist_next (list);
      }

      /*  Don't forget the selection mask!  */
      channel_resize (gimage->selection_mask, width, height, -x1, -y1);
      gimage_mask_invalidate (gimage);

      /*  crop all layers  */
      list = gimage->layers;
      while (list)
      {
	layer = (Layer *) list->data;

	layer_translate (layer, -x1, -y1);

	drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

	lx1 = BOUNDS (off_x, 0, gimage->width);
	ly1 = BOUNDS (off_y, 0, gimage->height);
	lx2 = BOUNDS ((drawable_width (GIMP_DRAWABLE (layer)) + off_x), 0, gimage->width);
	ly2 = BOUNDS ((drawable_height (GIMP_DRAWABLE (layer)) + off_y), 0, gimage->height);
	width = lx2 - lx1;
	height = ly2 - ly1;

	list = g_slist_next (list);

	if (width && height)
	  layer_resize (layer, width, height,
			-(lx1 - off_x),
			-(ly1 - off_y));
	else
	  gimage_remove_layer (gimage, layer);
      }

      /*  Make sure the projection matches the gimage size  */
      gimage_projection_realloc (gimage);

      /*  rigor the floating layer  */
      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      guide_list_ptr = gimage->guides;
      while ( guide_list_ptr != NULL)
      {
        undo_push_guide (gimage, (Guide *)guide_list_ptr->data);
        guide_list_ptr = guide_list_ptr->next;
      }
      undo_push_group_end (gimage);
  
      /* Adjust any guides we might have laying about */
      crop_adjust_guides (gimage, x1, y1, x2, y2); 

      /*  shrink wrap and update all views  */
      channel_invalidate_previews (gimage);
      layer_invalidate_previews (gimage);
      gimage_invalidate_preview (gimage);
      gdisplays_update_full (gimage);
      gdisplays_shrink_wrap (gimage);
    }
    gimp_remove_busy_cursors(NULL);
    gdisplays_flush ();
  }
}

static void
crop_recalc (Tool *tool,
	     Crop *crop)
{
  GDisplay * gdisp;

  gdisp = (GDisplay *) tool->gdisp_ptr;

  gdisplay_transform_coords (gdisp, crop->tx1, crop->ty1,
			     &crop->x1, &crop->y1, FALSE);
  gdisplay_transform_coords (gdisp, crop->tx2, crop->ty2,
			     &crop->x2, &crop->y2, FALSE);
}

static void
crop_start (Tool *tool,
	    Crop *crop)
{
  static GDisplay * old_gdisp = NULL;
  GDisplay        * gdisp;

  gdisp = (GDisplay *) tool->gdisp_ptr;

  crop_recalc (tool, crop);
  /*  if the crop information dialog already exists, delete it  */
  if (crop_info && (gdisp != old_gdisp))
    {
      info_dialog_popdown (crop_info);
      info_dialog_free (crop_info);
      crop_info = NULL;
    }
  old_gdisp = gdisp;

  if (! crop_info)
    crop_info_create (tool);
  
  /* initialize the statusbar display */
  crop->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "crop");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      crop->context_id, _("Crop: 0 x 0"));

  draw_core_start (crop->core, gdisp->canvas->window, tool);
}


/*******************************************************/
/*  Crop dialog functions                              */
/*******************************************************/

static ActionAreaItem action_items[3] =
{
  { N_("Crop"), crop_ok_callback, NULL, NULL },
  { N_("Selection"), crop_selection_callback, NULL, NULL },
  { N_("Close"), crop_close_callback, NULL, NULL },
};

static void
crop_info_create (Tool *tool)
{
  GDisplay      *gdisp;
  GtkWidget     *sizeentry;
  GtkWidget     *spinbutton2;

  gdisp = (GDisplay *) tool->gdisp_ptr;

  /*  create the info dialog  */
  crop_info = info_dialog_new (_("Crop Information"));

  /*  add the information fields  */
  spinbutton2 = info_dialog_add_spinbutton (crop_info, _("Origin X:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Y:"), orig_vals, 1,
			       gdisp->dot_for_dot ? 
			       UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, FALSE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       crop_orig_changed, tool);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
			     GTK_SPIN_BUTTON (spinbutton2), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, orig_vals[0]);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					 -65536, 65536);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1,
				  gdisp->gimage->yresolution, FALSE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, orig_vals[1]);

  spinbutton2 = info_dialog_add_spinbutton (crop_info, _("Width:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Height:"), size_vals, 1,
			       gdisp->dot_for_dot ? 
			       UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, FALSE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       crop_size_changed, tool);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
			     GTK_SPIN_BUTTON (spinbutton2), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, size_vals[0]);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					 -65536, 65536);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1,
				  gdisp->gimage->yresolution, FALSE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, size_vals[1]);

  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 2, 0);

  /* Create the action area  */
  build_action_area (GTK_DIALOG (crop_info->shell), action_items, 3, 0);
}

static void
crop_info_update (Tool *tool)
{
  Crop * crop;
 
  crop = (Crop *) tool->private;

  orig_vals[0] = crop->tx1;
  orig_vals[1] = crop->ty1;
  size_vals[0] = crop->tx2 - crop->tx1;
  size_vals[1] = crop->ty2 - crop->ty1;

  info_dialog_update (crop_info);
  info_dialog_popup (crop_info);
}

static void
crop_ok_callback (GtkWidget *w,
		  gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  GDisplay * gdisp;

  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;
  crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2);

  /*  Finish the tool  */
  draw_core_stop (crop->core, tool);
  info_dialog_popdown (crop_info);
  tool->state = INACTIVE;
}

static void
crop_selection_callback (GtkWidget *w,
			 gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  GDisplay * gdisp;

  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;

  draw_core_pause (crop->core, tool);
  if (! gimage_mask_bounds (gdisp->gimage, &crop->tx1, &crop->ty1, &crop->tx2, &crop->ty2))
    {
      crop->tx1 = crop->ty1 = 0;
      crop->tx2 = gdisp->gimage->width;
      crop->ty2 = gdisp->gimage->height;
    }

  crop_recalc (tool, crop);
  draw_core_resume (crop->core, tool);
}

static void
crop_close_callback (GtkWidget *w,
		     gpointer   client_data)
{
  Tool * tool;

  tool = active_tool;

  draw_core_stop (((Crop *) tool->private)->core, tool);
  info_dialog_popdown (crop_info);
  tool->state = INACTIVE;
}

static void
crop_orig_changed (GtkWidget *w,
		   gpointer   data)
{
  Tool     *tool;
  Crop     *crop;
  GDisplay *gdisp;
  int       ox;
  int       oy;

  tool = (Tool *)data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      crop = (Crop *) tool->private;
      ox = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (w), 0);
      oy = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (w), 1);

      if ((ox != crop->tx1) ||
	  (oy != crop->ty1))
	{
	  draw_core_pause (crop->core, tool);
	  crop->tx2 = crop->tx2 + (ox - crop->tx1);
	  crop->tx1 = ox;
	  crop->ty2 = crop->ty2 + (oy - crop->ty1);
	  crop->ty1 = oy;
	  crop_recalc (tool, crop);
	  draw_core_resume (crop->core, tool);
	}
    }
}

static void
crop_size_changed (GtkWidget *w,
		   gpointer   data)
{
  Tool     *tool;
  Crop     *crop;
  GDisplay *gdisp;
  int       sx;
  int       sy;

  tool = (Tool *)data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      crop = (Crop *) tool->private;
      sx = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (w), 0);
      sy = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (w), 1);

      if ((sx != (crop->tx2 - crop->tx1)) ||
	  (sy != (crop->ty2 - crop->ty1)))
	{
	  draw_core_pause (crop->core, tool);
	  crop->tx2 = sx + crop->tx1;
	  crop->ty2 = sy + crop->ty1;
	  crop_recalc (tool, crop);
	  draw_core_resume (crop->core, tool);
	}
    }
}

/*  The procedure definition  */
ProcArg crop_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "new_width",
    "new image width: (0 < new_width <= width)"
  },
  { PDB_INT32,
    "new_height",
    "new image height: (0 < new_height <= height)"
  },
  { PDB_INT32,
    "offx",
    "x offset: (0 <= offx <= (width - new_width))"
  },
  { PDB_INT32,
    "offy",
    "y offset: (0 <= offy <= (height - new_height))"
  }
};

ProcRecord crop_proc =
{
  "gimp_crop",
  "Crop the image to the specified extents.",
  "This procedure crops the image so that it's new width and height are equal to the supplied parameters.  Offsets are also provided which describe the position of the previous image's content.  All channels and layers within the image are cropped to the new image extents; this includes the image selection mask.  If any parameters are out of range, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  crop_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { crop_invoker } },
};

static Argument *
crop_invoker (Argument *args)
{
  GImage *gimage;
  int success;
  int int_value;
  int new_width, new_height;
  int offx, offy;

  new_width  = 1;
  new_height = 1;
  offx       = 0;
  offy       = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      new_width = args[1].value.pdb_int;
      new_height = args[2].value.pdb_int;

      if (new_width <= 0 || new_height <= 0)
	success = FALSE;
    }
  if (success)
    {
      offx = args[3].value.pdb_int;
      offy = args[4].value.pdb_int;
    }

  if ((new_width <= 0 || new_width > gimage->width) ||
      (new_height <= 0 || new_height > gimage->height) ||
      (offx < 0 || offx > (gimage->width - new_width)) ||
      (offy < 0 || offy > (gimage->height - new_height)))
    success = FALSE;

  if (success)
    crop_image (gimage, offx, offy, offx + new_width, offy + new_height);

  return procedural_db_return_args (&crop_proc, success);
}
