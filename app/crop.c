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
#include <string.h>

#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "crop.h"
#include "cursorutil.h"
#include "draw_core.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimphelp.h"
#include "gimpui.h"
#include "info_dialog.h"
#include "tool_options_ui.h"
#include "undo.h"

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

#define STATUSBAR_SIZE 128

/*  possible crop functions  */
#define CREATING        0
#define MOVING          1
#define RESIZING_LEFT   2
#define RESIZING_RIGHT  3
#define CROPPING        4

/*  speed of key movement  */
#define ARROW_VELOCITY 25

/*  the crop structures  */

typedef struct _Crop Crop;
struct _Crop
{
  DrawCore * core;       /*  Core select object          */

  int        startx;     /*  starting x coord            */
  int        starty;     /*  starting y coord            */

  int        lastx;      /*  previous x coord            */
  int        lasty;      /*  previous y coord            */

  int        x1, y1;     /*  upper left hand coordinate  */
  int        x2, y2;     /*  lower right hand coords     */

  int        srw, srh;   /*  width and height of corners */

  int        tx1, ty1;   /*  transformed coords          */
  int        tx2, ty2;   /*                              */

  int        function;   /*  moving or resizing          */
  guint      context_id; /*  for the statusbar           */
};

typedef struct _CropOptions CropOptions;
struct _CropOptions
{
  ToolOptions  tool_options;

  gboolean     layer_only;
  gboolean     layer_only_d;
  GtkWidget   *layer_only_w;

  gboolean     allow_enlarge;
  gboolean     allow_enlarge_d;
  GtkWidget   *allow_enlarge_w;

  CropType     type;
  CropType     type_d;
  GtkWidget   *type_w[2];
};


/*  the crop tool options  */
static CropOptions *crop_options = NULL;

/*  the crop tool info dialog  */
static InfoDialog  *crop_info = NULL;

static gdouble      orig_vals[2];
static gdouble      size_vals[2];

static GtkWidget   *origin_sizeentry;
static GtkWidget   *size_sizeentry;


/*  Crop type functions  */
static void crop_button_press       (Tool *, GdkEventButton *, gpointer);
static void crop_button_release     (Tool *, GdkEventButton *, gpointer);
static void crop_motion             (Tool *, GdkEventMotion *, gpointer);
static void crop_cursor_update      (Tool *, GdkEventMotion *, gpointer);
static void crop_control            (Tool *, ToolAction,       gpointer);
static void crop_arrow_keys_func    (Tool *, GdkEventKey *,    gpointer);
static void crop_modifier_key_func  (Tool *, GdkEventKey *,    gpointer);

/*  Crop helper functions  */
static void crop_recalc             (Tool *, Crop *);
static void crop_start              (Tool *, Crop *);
static void crop_adjust_guides      (GImage *, int, int, int, int);

/*  Crop dialog functions  */
static void crop_info_update        (Tool *);
static void crop_info_create        (Tool *);
static void crop_crop_callback      (GtkWidget *, gpointer);
static void crop_resize_callback    (GtkWidget *, gpointer);
static void crop_close_callback     (GtkWidget *, gpointer);

/* Crop area-select functions */ 
typedef enum
{
  AUTO_CROP_NOTHING = 0,
  AUTO_CROP_ALPHA   = 1,
  AUTO_CROP_COLOR   = 2
} AutoCropType;

typedef guchar       * (*GetColorFunc)    (GtkObject *, int, int);
typedef AutoCropType   (*ColorsEqualFunc) (guchar *, guchar *, int);

static void   crop_selection_callback    (GtkWidget *, gpointer);
static void   crop_automatic_callback    (GtkWidget *, gpointer);
static AutoCropType crop_guess_bgcolor   (GtkObject *, GetColorFunc, 
					  int, int, guchar *, int, int, int, int);
static int   crop_colors_equal           (guchar *, guchar *, int);
static int   crop_colors_alpha           (guchar *, guchar *, int);

/*  Crop dialog callback funtions  */
static void   crop_orig_changed (GtkWidget *, gpointer);
static void   crop_size_changed (GtkWidget *, gpointer);


/*  Functions  */

static void
crop_options_reset (void)
{
  CropOptions *options = crop_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(options->allow_enlarge_w),
				options->allow_enlarge_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE); 
}

static CropOptions *
crop_options_new (void)
{
  CropOptions *options;
  GtkWidget *vbox;
  GtkWidget *frame;
  gchar* type_label[2] = { N_("Crop"), N_("Resize") };
  gint   type_value[2] = { CROP_CROP, RESIZE_CROP };

  /*  the new crop tool options structure  */
  options = (CropOptions *) g_malloc (sizeof (CropOptions));
  tool_options_init ((ToolOptions *) options,
		     _("Crop & Resize Options"),
		     crop_options_reset);
  options->layer_only    = options->layer_only_d    = FALSE;
  options->allow_enlarge = options->allow_enlarge_d = FALSE;
  options->type          = options->type_d          = CROP_CROP;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  layer toggle  */
  options->layer_only_w =
    gtk_check_button_new_with_label(_("Current Layer Only"));
  gtk_box_pack_start (GTK_BOX (vbox), options->layer_only_w,
		      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->layer_only_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->layer_only);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only_d);
  gtk_widget_show (options->layer_only_w);

  /*  enlarge toggle  */
  options->allow_enlarge_w = gtk_check_button_new_with_label (_("Allow Enlarging"));
  gtk_box_pack_start (GTK_BOX (vbox), options->allow_enlarge_w,
		      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->allow_enlarge_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->allow_enlarge);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_enlarge_w),
				options->allow_enlarge_d);
  gtk_widget_show (options->allow_enlarge_w);

  /*  tool toggle  */
  frame = tool_options_radio_buttons_new (_("Tool Toggle"), 
					  &options->type,
					   options->type_w,
					   type_label,
					   type_value,
					   2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
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

  if (tool->state == INACTIVE ||
      gdisp_ptr != tool->gdisp_ptr)
    {
      crop->function = CREATING;
    }
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

      /*  If the pointer is in the rectangular region, crop or resize it!  */
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
      tool->drawable = gimage_active_drawable (gdisp->gimage);

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
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
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

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), crop->context_id);

  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      if (crop->function == CROPPING)
	{
	  if (crop_options->type == CROP_CROP)
	    crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2, 
			crop_options->layer_only, TRUE);
	  else
	    crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2, 
			crop_options->layer_only, FALSE);

	  /*  Finish the tool  */
	  crop_close_callback (NULL, NULL);
	}
      else
	crop_info_update (tool);
    }
}

static void
crop_adjust_guides (GImage *gimage,
                    int x1, int y1,
                    int x2, int y2)

{
  GList * glist;
  Guide * guide;
  gint remove_guide;

  for (glist = gimage->guides; glist; glist = g_list_next (glist))
    {
      guide = (Guide *) glist->data;
      remove_guide = FALSE;

      switch (guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  if ((guide->position < y1) ||(guide->position > y2))
	    remove_guide = TRUE;
	  break;

	case ORIENTATION_VERTICAL:
	  if ((guide->position < x1) ||(guide->position > x2))
	    remove_guide = TRUE;
	  break;
	}
	
      /* edit the guide */
      gdisplays_expose_guide (gimage, guide);
      gdisplays_flush();

      if (remove_guide)
	{
	  guide->position = -1;
          guide = NULL;
	}
      else
	{
	  if (guide->orientation == ORIENTATION_HORIZONTAL)
	    {
	      guide->position -= y1 ;
	    }
	  else
	    {
	      guide->position -= x1;
	    }
	}
    }
}
		 

static void
crop_motion (Tool           *tool,
	     GdkEventMotion *mevent,
	     gpointer        gdisp_ptr)
{
  Crop * crop;
  GDisplay * gdisp;
  Layer * layer;
  int x1, y1, x2, y2;
  int curx, cury;
  int inc_x, inc_y;
  gchar size[STATUSBAR_SIZE];
  int min_x, min_y, max_x, max_y; 
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
  
  if (crop_options->layer_only)
    {
      layer = (gdisp->gimage)->active_layer;
      drawable_offsets (GIMP_DRAWABLE(layer), &min_x, &min_y);
      max_x  = drawable_width (GIMP_DRAWABLE(layer)) + min_x;
      max_y = drawable_height (GIMP_DRAWABLE(layer)) + min_y;
    }
  else
    {
      min_x = min_y = 0;
      max_x = gdisp->gimage->width;
      max_y = gdisp->gimage->height;
    }

  switch (crop->function)
    {
    case CREATING :
      if (!crop_options->allow_enlarge)
	{
	  x1 = BOUNDS (x1, min_x, max_x);
	  y1 = BOUNDS (y1, min_y, max_y);
	  x2 = BOUNDS (x2, min_x, max_x);
	  y2 = BOUNDS (y2, min_y, max_y);
	}
      break;

    case RESIZING_LEFT :
      x1 = crop->tx1 + inc_x;
      y1 = crop->ty1 + inc_y;
      if (!crop_options->allow_enlarge)
	{
	  x1 = BOUNDS (x1, min_x, max_x);
	  y1 = BOUNDS (y1, min_y, max_y);
	}
      x2 = MAXIMUM (x1, crop->tx2);
      y2 = MAXIMUM (y1, crop->ty2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case RESIZING_RIGHT :
      x2 = crop->tx2 + inc_x;
      y2 = crop->ty2 + inc_y;
      if (!crop_options->allow_enlarge)
	{
	  x2 = BOUNDS (x2, min_x, max_x);
	  y2 = BOUNDS (y2, min_y, max_y);
	}
      x1 = MINIMUM (crop->tx1, x2);
      y1 = MINIMUM (crop->ty1, y2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case MOVING :
      if (!crop_options->allow_enlarge)
	{
	  inc_x = BOUNDS (inc_x, min_x - crop->tx1, max_x - crop->tx2);
	  inc_y = BOUNDS (inc_y, min_y - crop->ty1, max_y - crop->ty2);
	}
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
	  gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
	  
	  g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		      _("Crop: "), 
		      (gdouble) (crop->tx2 - crop->tx1) * unit_factor /
		      gdisp->gimage->xresolution,
		      " x ",
		      (gdouble) (crop->ty2 - crop->ty1) * unit_factor /
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
  GdkCursorType ctype;
  Crop * crop;

  gdisp = (GDisplay *) gdisp_ptr;
  crop = (Crop *) tool->private;

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
    {
      if ( crop_options->type == CROP_CROP )
	ctype = GDK_ICON;
      else 
	ctype = GDK_SIZING;
    }
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
  Layer * layer;
  Crop * crop;
  int min_x, min_y, max_x, max_y;

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

      if (crop_options->layer_only)
	{
	  layer = (gdisp->gimage)->active_layer;
	  drawable_offsets (GIMP_DRAWABLE(layer), &min_x, &min_y);
	  max_x  = drawable_width (GIMP_DRAWABLE(layer)) + min_x;
	  max_y = drawable_height (GIMP_DRAWABLE(layer)) + min_y;
	}
      else
	{
	  min_x = min_y = 0;
	  max_x = gdisp->gimage->width;
	  max_y = gdisp->gimage->height;
	}

      if (kevent->state & GDK_CONTROL_MASK)  /* RESIZING */
	{
	  crop->tx2 = crop->tx2 + inc_x;
	  crop->ty2 = crop->ty2 + inc_y;
	  if (!crop_options->allow_enlarge)
	    {
	      crop->tx2 = BOUNDS (crop->tx2, min_x, max_x);
	      crop->ty2 = BOUNDS (crop->ty2, min_y, max_y);
	    }
	  crop->tx1 = MINIMUM (crop->tx1, crop->tx2);
	  crop->ty1 = MINIMUM (crop->ty1, crop->ty2);
	}
      else
	{
	  if (!crop_options->allow_enlarge)
	    {	  
	      inc_x = BOUNDS (inc_x, -crop->tx1, gdisp->gimage->width - crop->tx2);
	      inc_y = BOUNDS (inc_y, -crop->ty1, gdisp->gimage->height - crop->ty2);
	    }
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
crop_modifier_key_func (Tool        *tool,
			GdkEventKey *kevent,
			gpointer     gdisp_ptr)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (crop_options->allow_enlarge_w), !crop_options->allow_enlarge);
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
      if (crop_options->type == CROP_CROP)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (crop_options->type_w[RESIZE_CROP]), TRUE);
      else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (crop_options->type_w[CROP_CROP]), TRUE);
      break;
    }
}

static void
crop_control (Tool       *tool,
	      ToolAction  action,
	      gpointer    gdisp_ptr)
{
  Crop * crop;

  crop = (Crop *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (crop->core, tool);
      break;

    case RESUME:
      crop_recalc (tool, crop);
      draw_core_resume (crop->core, tool);
      break;

    case HALT:
      crop_close_callback (NULL, NULL);
      break;

    default:
      break;
    }
}

void
crop_draw (Tool *tool)
{
  Crop * crop;
  GDisplay * gdisp;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  crop = (Crop *) tool->private;

#define SRW 10
#define SRH 10

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

#undef SRW
#undef SRH

  crop_info_update (tool);
}

Tool *
tools_new_crop ()
{
  Tool * tool;
  Crop * private;

  /*  The tool options  */
  if (! crop_options)
    {
      crop_options = crop_options_new ();
      tools_register (CROP, (ToolOptions *) crop_options);
    }

  tool = tools_new_tool (CROP);
  private = g_new (Crop, 1);

  private->core = draw_core_new (crop_draw);
  private->startx = private->starty = 0;
  private->function = CREATING;

  tool->private = (void *) private;

  tool->preserve = FALSE;  /* XXX Check me */

  tool->button_press_func   = crop_button_press;
  tool->button_release_func = crop_button_release;
  tool->motion_func         = crop_motion;
  tool->arrow_keys_func     = crop_arrow_keys_func;
  tool->modifier_key_func   = crop_modifier_key_func;
  tool->cursor_update_func  = crop_cursor_update;
  tool->control_func        = crop_control;

  return tool;
}

void
tools_free_crop (Tool *tool)
{
  Crop * crop;

  crop = (Crop *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (crop->core, tool);

  if (crop_info)
    crop_close_callback (NULL, NULL);

  draw_core_free (crop->core);
  g_free (crop);
}

void
crop_image (GImage *gimage,
	    int     x1,
	    int     y1,
	    int     x2,
	    int     y2,
	    int     layer_only,
	    int     crop_layers)
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

    if (layer_only)
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
	GSList * next;

	layer = (Layer *) list->data;
	next = g_slist_next (list);

	layer_translate (layer, -x1, -y1);

	drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

	if (crop_layers)
	  {
	    drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

	    lx1 = BOUNDS (off_x, 0, gimage->width);
	    ly1 = BOUNDS (off_y, 0, gimage->height);
	    lx2 = BOUNDS ((drawable_width (GIMP_DRAWABLE (layer)) + off_x), 0, gimage->width);
	    ly2 = BOUNDS ((drawable_height (GIMP_DRAWABLE (layer)) + off_y), 0, gimage->height);
	    width = lx2 - lx1;
	    height = ly2 - ly1;
	    
	    if (width && height)
	      layer_resize (layer, width, height,
			    -(lx1 - off_x),
			    -(ly1 - off_y));
	    else
	      gimage_remove_layer (gimage, layer);
	  }

	list = next;
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

  if (! crop_info)
    crop_info_create (tool);

  gtk_signal_handler_block_by_data (GTK_OBJECT (origin_sizeentry), crop_info);
  gtk_signal_handler_block_by_data (GTK_OBJECT (size_sizeentry), crop_info);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
				  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
				  gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
			    0, gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
			    0, gdisp->gimage->height);
      
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry), 0,
				  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry), 1,
				  gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_sizeentry), 0,
			    0, gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_sizeentry), 1,
			    0, gdisp->gimage->height);

  if (old_gdisp != gdisp)
    {
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (origin_sizeentry),
				gdisp->gimage->unit) ;
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_sizeentry),
				gdisp->gimage->unit);

      if (gdisp->dot_for_dot)
	{
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (origin_sizeentry),
				    UNIT_PIXEL);
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_sizeentry),
				    UNIT_PIXEL);
	}
    }

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (size_sizeentry), crop_info);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (origin_sizeentry), crop_info);

  old_gdisp = gdisp;

  /* initialize the statusbar display */
  crop->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "crop");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      crop->context_id, _("Crop: 0 x 0"));

  draw_core_start (crop->core, gdisp->canvas->window, tool);
}


/***************************/
/*  Crop dialog functions  */
/***************************/

static void
crop_info_create (Tool *tool)
{
  GDisplay  *gdisp;
  GtkWidget *spinbutton;
  GtkWidget *bbox;
  GtkWidget *button;

  gdisp = (GDisplay *) tool->gdisp_ptr;

  /*  create the info dialog  */
  crop_info = info_dialog_new (_("Crop & Resize Information"),
			       tools_help_func, NULL);

  /*  create the action area  */
  gimp_dialog_create_action_area (GTK_DIALOG (crop_info->shell),

				  _("Crop"), crop_crop_callback,
				  NULL, NULL, TRUE, FALSE,
				  _("Resize"), crop_resize_callback,
				  NULL, NULL, FALSE, FALSE,
				  _("Close"), crop_close_callback,
				  NULL, NULL, FALSE, FALSE,

				  NULL);

  /*  add the information fields  */
  spinbutton = info_dialog_add_spinbutton (crop_info, _("Origin X:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  origin_sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Y:"), orig_vals, 1,
			       gdisp->dot_for_dot ? 
			       UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       crop_orig_changed, crop_info);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (origin_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
					 -65536, 65536);

  spinbutton = info_dialog_add_spinbutton (crop_info, _("Width:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  size_sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Height:"), size_vals, 1,
			       gdisp->dot_for_dot ? 
			       UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       crop_size_changed, crop_info);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (size_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_sizeentry), 1,
					 -65536, 65536);

  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 2, 0);

  /* Create the area selection buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 4);

  button = gtk_button_new_with_label (_("From Selection"));
  gtk_container_add( GTK_CONTAINER(bbox), button);
  gtk_signal_connect(GTK_OBJECT (button) , "clicked",
		     (GtkSignalFunc) crop_selection_callback, NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Auto Shrink"));
  gtk_container_add(GTK_CONTAINER (bbox), button);
  gtk_signal_connect(GTK_OBJECT (button) , "clicked",
		     (GtkSignalFunc) crop_automatic_callback, NULL);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (crop_info->vbox), bbox, FALSE, FALSE, 2);
  gtk_widget_show (bbox);
}

static void
crop_info_update (Tool *tool)
{
  Crop * crop;
  GDisplay * gdisp;

  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;

  orig_vals[0] = crop->tx1;
  orig_vals[1] = crop->ty1;
  size_vals[0] = crop->tx2 - crop->tx1;
  size_vals[1] = crop->ty2 - crop->ty1;

  info_dialog_update (crop_info);
  info_dialog_popup (crop_info);
}

static void
crop_crop_callback (GtkWidget *w,
		    gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  GDisplay * gdisp;

  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;
  crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2,
	      crop_options->layer_only, TRUE);

  /*  Finish the tool  */
  crop_close_callback (NULL, NULL);
}

static void
crop_resize_callback (GtkWidget *w,
		      gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  GDisplay * gdisp;

  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;
  crop_image (gdisp->gimage, crop->tx1, crop->ty1, crop->tx2, crop->ty2, 
	      crop_options->layer_only, FALSE);

  /*  Finish the tool  */
  crop_close_callback (NULL, NULL);
}

static void
crop_close_callback (GtkWidget *w,
		     gpointer   client_data)
{
  Tool * tool;
  Crop * crop;

  tool = active_tool;
  crop = (Crop *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (crop->core, tool);

  info_dialog_popdown (crop_info);

  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;
  tool->state = INACTIVE;
}

static void
crop_selection_callback (GtkWidget *w,
			 gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  Layer * layer;
  GDisplay * gdisp;

  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;

  draw_core_pause (crop->core, tool);
  if (! gimage_mask_bounds (gdisp->gimage, &crop->tx1, &crop->ty1, &crop->tx2, &crop->ty2))
    {
      if (crop_options->layer_only)
	{
	  layer = (gdisp->gimage)->active_layer;
	  drawable_offsets (GIMP_DRAWABLE(layer), &crop->tx1, &crop->ty1);
	  crop->tx2 = drawable_width  (GIMP_DRAWABLE(layer)) + crop->tx1;
	  crop->ty2 = drawable_height (GIMP_DRAWABLE(layer)) + crop->ty1;
	}
      else
	{
	  crop->tx1 = crop->ty1 = 0;
	  crop->tx2 = gdisp->gimage->width;
	  crop->ty2 = gdisp->gimage->height;
	}
    }
  crop_recalc (tool, crop);
  draw_core_resume (crop->core, tool);
}


static void
crop_automatic_callback (GtkWidget *w,
			 gpointer   client_data)
{
  Tool * tool;
  Crop * crop;
  GDisplay * gdisp;
  GimpDrawable * active_drawable = NULL;  
  GetColorFunc get_color_func;
  ColorsEqualFunc colors_equal_func;
  GtkObject *get_color_obj;
  guchar bgcolor[4] = {0, 0, 0, 0};
  gint has_alpha = FALSE;
  PixelRegion PR;
  guchar *buffer = NULL;
  gint offset_x, offset_y, width, height, bytes;
  gint x, y, abort;
  gint x1, y1, x2, y2;
  
  tool = active_tool;
  crop = (Crop *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;

  draw_core_pause (crop->core, tool);
  gimp_add_busy_cursors ();
 
  /* You should always keep in mind that crop->tx2 and crop->ty2 are the NOT the
     coordinates of the bottomright corner of the area to be cropped. They point 
     at the pixel located one to the right and one to the bottom.  
   */

  if (crop_options->layer_only)
    {
      if (!(active_drawable =  gimage_active_drawable (gdisp->gimage)))
	return;
      width  = drawable_width  (GIMP_DRAWABLE (active_drawable)); 
      height = drawable_height (GIMP_DRAWABLE (active_drawable));
      bytes  = drawable_bytes  (GIMP_DRAWABLE (active_drawable));
      if (drawable_has_alpha (GIMP_DRAWABLE (active_drawable)))
	has_alpha = TRUE;
      get_color_obj = GTK_OBJECT (active_drawable);
      get_color_func = (GetColorFunc) gimp_drawable_get_color_at;
      drawable_offsets (GIMP_DRAWABLE (active_drawable), &offset_x, &offset_y);
    }
  else
    {
      width  = gdisp->gimage->width;
      height = gdisp->gimage->height;
      has_alpha = TRUE; 
      bytes  = gimp_image_composite_bytes (gdisp->gimage);
      get_color_obj = GTK_OBJECT (gdisp->gimage);
      get_color_func = (GetColorFunc) gimp_image_get_color_at;
      offset_x = offset_y = 0;
   }

  x1 = crop->tx1 - offset_x  > 0      ? crop->tx1 - offset_x : 0;
  x2 = crop->tx2 - offset_x  < width  ? crop->tx2 - offset_x : width;
  y1 = crop->ty1 - offset_y  > 0      ? crop->ty1 - offset_y : 0;
  y2 = crop->ty2 - offset_y  < height ? crop->ty2 - offset_y : height;
      
  switch (crop_guess_bgcolor (get_color_obj, get_color_func, bytes, has_alpha, 
			      bgcolor, x1, x2-1, y1, y2-1))
    {
    case AUTO_CROP_ALPHA:
      colors_equal_func = (ColorsEqualFunc) crop_colors_alpha;
      break;
    case AUTO_CROP_COLOR:
      colors_equal_func = (ColorsEqualFunc) crop_colors_equal;
      break;
    default:
      goto FINISH;
    }

  width  = x2 - x1;
  height = y2 - y1;

  if (crop_options->layer_only)
    pixel_region_init (&PR, drawable_data (active_drawable), 
		       x1, y1, width, height, FALSE);
  else
    pixel_region_init (&PR, gimp_image_composite (gdisp->gimage), 
		       x1, y1, width, height, FALSE);

  /* The following could be optimized further by processing the smaller side first
     instead of defaulting to width    --Sven                                      
   */

  buffer = g_malloc ((width > height ? width : height) * bytes);

 /* Check how many of the top lines are uniform/transparent. */
  abort = FALSE;
  for (y = y1; y < y2 && !abort; y++)
    {
      pixel_region_get_row (&PR, x1, y, width, buffer, 1);
      for (x = 0; x < width && !abort; x++)
	abort = !(colors_equal_func) (bgcolor, buffer + x * bytes, bytes);
    }
  if (y == y2 && !abort) 
    goto FINISH;
  y1 = y - 1;

  /* Check how many of the bottom lines are uniform/transparent. */
  abort = FALSE;
  for (y = y2; y > y1 && !abort; y--)
    {
      pixel_region_get_row (&PR, x1, y-1 , width, buffer, 1);
      for (x = 0; x < width && !abort; x++)
	abort = !(colors_equal_func) (bgcolor, buffer + x * bytes, bytes);
    }
  y2 = y + 1;

  /* compute a new height for the next operations */
  height = y2 - y1;

  /* Check how many of the left lines are uniform/transparent. */
  abort = FALSE;
  for (x = x1; x < x2 && !abort; x++)
    {
      pixel_region_get_col (&PR, x, y1, height, buffer, 1);
      for (y = 0; y < height && !abort; y++)
	abort = !(colors_equal_func) (bgcolor, buffer + y * bytes, bytes);
    }
  x1 = x - 1;
 
 /* Check how many of the right lines are uniform/transparent. */
  abort = FALSE;
  for (x = x2; x > x1 && !abort; x--)
    {
      pixel_region_get_col (&PR, x-1, y1, height, buffer, 1);
      for (y = 0; y < height && !abort; y++)
	abort = !(colors_equal_func) (bgcolor, buffer + y * bytes, bytes);
    }
  x2 = x + 1;

  crop->tx1 = offset_x + x1;
  crop->tx2 = offset_x + x2;
  crop->ty1 = offset_y + y1;
  crop->ty2 = offset_y + y2;

  crop_recalc (tool, crop);

 FINISH:
  g_free (buffer);
  gimp_remove_busy_cursors (NULL);
  draw_core_resume (crop->core, tool);

  return;
}

static AutoCropType
crop_guess_bgcolor (GtkObject    *get_color_obj,
		    GetColorFunc  get_color_func,
		    int           bytes,
		    int           has_alpha,
		    guchar       *color,
		    int           x1, 
		    int           x2, 
		    int           y1, 
		    int           y2) 
{
  guchar *tl = NULL;
  guchar *tr = NULL;
  guchar *bl = NULL;
  guchar *br = NULL;
  gint i, alpha;

  for (i = 0; i < bytes; i++)
    color[i] = 0; 

  /* First check if there's transparency to crop. If not, guess the 
   * background-color to see if at least 2 corners are equal.
   */

  if (!(tl = (*get_color_func) (get_color_obj, x1, y1))) 
    goto ERROR;
  if (!(tr = (*get_color_func) (get_color_obj, x1, y2))) 
    goto ERROR;
  if (!(bl = (*get_color_func) (get_color_obj, x2, y1))) 
    goto ERROR;  
  if (!(br = (*get_color_func) (get_color_obj, x2, y2))) 
    goto ERROR;

  if (has_alpha)
    {
      alpha = bytes - 1;
      if ((tl[alpha] == 0 && tr[alpha] == 0) ||
	  (tl[alpha] == 0 && bl[alpha] == 0) ||
	  (tr[alpha] == 0 && br[alpha] == 0) ||
	  (bl[alpha] == 0 && br[alpha] == 0))
	{
	  g_free (tl); 
	  g_free (tr); 
	  g_free (bl); 
	  g_free (br);
	  return AUTO_CROP_ALPHA;
	}
    }
  
  if (crop_colors_equal (tl, tr, bytes) || crop_colors_equal (tl, bl, bytes))
    memcpy (color, tl, bytes);
  else if (crop_colors_equal (br, bl, bytes) || crop_colors_equal (br, tr, bytes))
    memcpy (color, br, bytes);
  else
    goto ERROR;

  g_free (tl); 
  g_free (tr); 
  g_free (bl); 
  g_free (br);
  return AUTO_CROP_COLOR;

 ERROR:
  g_free (tl); 
  g_free (tr); 
  g_free (bl); 
  g_free (br);
  return AUTO_CROP_NOTHING;
}

static int 
crop_colors_equal (guchar *col1, 
		   guchar *col2, 
		   int     bytes) 
{
  int equal = TRUE;
  int b;
  
  for (b = 0; b < bytes; b++) {
    if (col1[b] != col2[b]) {
      equal = FALSE;
      break;
    }
  }
  
  return equal;
}

static int 
crop_colors_alpha (guchar *dummy, 
		   guchar *col, 
		   int     bytes) 
{
  if (col[bytes-1] == 0)
    return TRUE;
  else
    return FALSE;
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

  tool = active_tool;

  if (tool && active_tool->type == CROP)
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

  tool = active_tool;

  if (tool && active_tool->type == CROP)
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
