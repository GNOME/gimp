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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "boundary.h"
#include "by_color_select.h"
#include "colormaps.h"
#include "drawable.h"
#include "draw_core.h"
#include "general.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gdisplay.h"
#include "rect_select.h"

#define DEFAULT_FUZZINESS 15
#define PREVIEW_WIDTH   256
#define PREVIEW_HEIGHT  256
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK


typedef struct _ByColorSelect ByColorSelect;

struct _ByColorSelect
{
  int  x, y;         /*  Point from which to execute seed fill  */
  int  operation;    /*  add, subtract, normal color selection  */
};

typedef struct _ByColorDialog ByColorDialog;

struct _ByColorDialog
{
  GtkWidget   *shell;
  GtkWidget   *preview;
  GtkWidget   *gimage_name;

  int          threshold; /*  threshold value for color select  */
  int          operation; /*  Add, Subtract, Replace  */
  GImage      *gimage;    /*  gimage which is currently under examination  */
};

/*  by_color select action functions  */

static void   by_color_select_button_press   (Tool *, GdkEventButton *, gpointer);
static void   by_color_select_button_release (Tool *, GdkEventButton *, gpointer);
static void   by_color_select_motion         (Tool *, GdkEventMotion *, gpointer);
static void   by_color_select_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   by_color_select_control        (Tool *, int, gpointer);

static ByColorDialog *  by_color_select_new_dialog     (void);
static void             by_color_select_render         (ByColorDialog *, GImage *);
static void             by_color_select_draw           (ByColorDialog *, GImage *);
static gint             by_color_select_preview_events (GtkWidget *, GdkEventButton *, ByColorDialog *);
static void             by_color_select_type_callback  (GtkWidget *, gpointer);
static void             by_color_select_reset_callback (GtkWidget *, gpointer);
static void             by_color_select_close_callback (GtkWidget *, gpointer);
static gint             by_color_select_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void             by_color_select_fuzzy_update   (GtkAdjustment *, gpointer);
static void             by_color_select_preview_button_press (ByColorDialog *, GdkEventButton *);


static SelectionOptions *by_color_options = NULL;
static ByColorDialog *by_color_dialog = NULL;

static int        is_pixel_sufficiently_different (unsigned char *, unsigned char *, int, int, int, int);
static Channel *  by_color_select_color (GImage *, GimpDrawable *, unsigned char *, int, int, int);
static void       by_color_select (GImage *, GimpDrawable *, unsigned char *, int, int, int, int, double, int);
static Argument * by_color_select_invoker (Argument *);

/*  by_color selection machinery  */

static int
is_pixel_sufficiently_different (unsigned char *col1,
				 unsigned char *col2,
				 int            antialias,
				 int            threshold,
				 int            bytes,
				 int            has_alpha)
{
  int diff;
  int max;
  int b;
  int alpha;

  max = 0;
  alpha = (has_alpha) ? bytes - 1 : bytes;

  /*  if there is an alpha channel, never select transparent regions  */
  if (has_alpha && col2[alpha] == 0)
    return 0;

  for (b = 0; b < alpha; b++)
    {
      diff = col1[b] - col2[b];
      diff = abs (diff);
      if (diff > max)
	max = diff;
    }

  if (antialias && threshold > 0)
    {
      float aa;

      aa = 1.5 - ((float) max / threshold);
      if (aa <= 0)
	return 0;
      else if (aa < 0.5)
	return (unsigned char) (aa * 512);
      else
	return 255;
    }
  else
    {
      if (max > threshold)
	return 0;
      else
	return 255;
    }
}

static Channel *
by_color_select_color (GImage        *gimage,
		       GimpDrawable  *drawable,
		       unsigned char *color,
		       int            antialias,
		       int            threshold,
		       int            sample_merged)
{
  /*  Scan over the gimage's active layer, finding pixels within the specified
   *  threshold from the given R, G, & B values.  If antialiasing is on,
   *  use the same antialiasing scheme as in fuzzy_select.  Modify the gimage's
   *  mask to reflect the additional selection
   */
  Channel *mask;
  PixelRegion imagePR, maskPR;
  unsigned char *image_data;
  unsigned char *mask_data;
  unsigned char *idata, *mdata;
  unsigned char rgb[MAX_CHANNELS];
  int has_alpha, indexed;
  int width, height;
  int bytes, color_bytes, alpha;
  int i, j;
  void * pr;
  int d_type;

  /*  Get the image information  */
  if (sample_merged)
    {
      bytes = gimage_composite_bytes (gimage);
      d_type = gimage_composite_type (gimage);
      has_alpha = (d_type == RGBA_GIMAGE ||
		   d_type == GRAYA_GIMAGE ||
		   d_type == INDEXEDA_GIMAGE);
      indexed = d_type == INDEXEDA_GIMAGE || d_type == INDEXED_GIMAGE;
      width = gimage->width;
      height = gimage->height;
      pixel_region_init (&imagePR, gimage_composite (gimage), 0, 0, width, height, FALSE);
    }
  else
    {
      bytes = drawable_bytes (drawable);
      d_type = drawable_type (drawable);
      has_alpha = drawable_has_alpha (drawable);
      indexed = drawable_indexed (drawable);
      width = drawable_width (drawable);
      height = drawable_height (drawable);

      pixel_region_init (&imagePR, drawable_data (drawable), 0, 0, width, height, FALSE);
    }

  if (indexed) {
    /* indexed colors are always RGB or RGBA */
    color_bytes = has_alpha ? 4 : 3;
  } else {
    /* RGB, RGBA, GRAY and GRAYA colors are shaped just like the image */
    color_bytes = bytes;
  }

  alpha = bytes - 1;
  mask = channel_new_mask (gimage->ID, width, height);
  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 
		     0, 0, width, height, TRUE);

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (2, &imagePR, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;
      mask_data = maskPR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;
	  mdata = mask_data;
	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimage_get_color (gimage, d_type, rgb, idata);

	      /*  Plug the alpha channel in there  */
	      if (has_alpha)
		rgb[color_bytes - 1] = idata[alpha];

	      /*  Find how closely the colors match  */
	      *mdata++ = is_pixel_sufficiently_different (color, rgb, antialias,
							  threshold, color_bytes, has_alpha);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	  mask_data += maskPR.rowstride;
	}
    }

  return mask;
}

static void
by_color_select (GImage        *gimage,
		 GimpDrawable  *drawable,
		 unsigned char *color,
		 int            threshold,
		 int            op,
		 int            antialias,
		 int            feather,
		 double         feather_radius,
		 int            sample_merged)
{
  Channel * new_mask;
  int off_x, off_y;

  if (!drawable) 
    return;

  new_mask = by_color_select_color (gimage, drawable, color, antialias, threshold, sample_merged);

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  if (sample_merged)
    {
      off_x = 0; off_y = 0;
    }
  else
    drawable_offsets (drawable, &off_x, &off_y);

  if (feather)
    channel_feather (new_mask, gimage_get_mask (gimage),
		     feather_radius, op, off_x, off_y);
  else
    channel_combine_mask (gimage_get_mask (gimage),
			  new_mask, op, off_x, off_y);

  channel_delete (new_mask);
}


/*  by_color select action functions  */

static void
by_color_select_button_press (Tool           *tool,
			      GdkEventButton *bevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  ByColorSelect *by_color_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  by_color_sel = (ByColorSelect *) tool->private;

  if (!by_color_dialog)
    return;

  by_color_sel->x = bevent->x;
  by_color_sel->y = bevent->y;

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  /*  Defaults  */
  by_color_sel->operation = REPLACE;

  /*  Based on modifiers, and the "by color" dialog's selection mode  */
  if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    by_color_sel->operation = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    by_color_sel->operation = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    by_color_sel->operation = INTERSECT;
  else
    by_color_sel->operation = by_color_dialog->operation;

  /*  Make sure the "by color" select dialog is visible  */
  if (! GTK_WIDGET_VISIBLE (by_color_dialog->shell))
    gtk_widget_show (by_color_dialog->shell);

  /*  Update the by_color_dialog's active gdisp pointer  */
  if (by_color_dialog->gimage)
    by_color_dialog->gimage->by_color_select = FALSE;
  by_color_dialog->gimage = gdisp->gimage;
  gdisp->gimage->by_color_select = TRUE;
}

static void
by_color_select_button_release (Tool           *tool,
				GdkEventButton *bevent,
				gpointer        gdisp_ptr)
{
  ByColorSelect * by_color_sel;
  GDisplay * gdisp;
  int x, y;
  GimpDrawable *drawable;
  Tile *tile;
  unsigned char col[MAX_CHANNELS];
  unsigned char *data;
  int use_offsets;

  gdisp = (GDisplay *) gdisp_ptr;
  by_color_sel = (ByColorSelect *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      use_offsets = (by_color_options->sample_merged) ? FALSE : TRUE;
      gdisplay_untransform_coords (gdisp, by_color_sel->x, by_color_sel->y, &x, &y, FALSE, use_offsets);

      /*  Get the start color  */
      if (by_color_options->sample_merged)
	{
	  if (x < 0 || y < 0 || x >= gdisp->gimage->width || y >= gdisp->gimage->height)
	    return;
	  tile = tile_manager_get_tile (gimage_composite (gdisp->gimage), x, y, 0);
	  tile_ref (tile);
	  data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
	}
      else
	{
	  if (x < 0 || y < 0 || x >= drawable_width (drawable) || y >= drawable_height (drawable))
	    return;
	  tile = tile_manager_get_tile (drawable_data (drawable), x, y, 0);
	  tile_ref (tile);
	  data = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
	}

      gimage_get_color (gdisp->gimage, gimage_composite_type(gdisp->gimage), col, data);
      tile_unref (tile, FALSE);

      /*  select the area  */
      by_color_select (gdisp->gimage, drawable, col,
		       by_color_dialog->threshold,
		       by_color_sel->operation,
		       by_color_options->antialias,
		       by_color_options->feather,
		       by_color_options->feather_radius,
		       by_color_options->sample_merged);

      /*  show selection on all views  */
      gdisplays_flush ();

      /*  update the preview window  */
      by_color_select_render (by_color_dialog, gdisp->gimage);
      by_color_select_draw (by_color_dialog, gdisp->gimage);
    }
}

static void
by_color_select_motion (Tool           *tool,
			GdkEventMotion *mevent,
			gpointer        gdisp_ptr)
{
}

static void
by_color_select_cursor_update (Tool           *tool,
			       GdkEventMotion *mevent,
			       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
    if (layer == gdisp->gimage->active_layer)
      {
	gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
	return;
      }
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
by_color_select_control (Tool     *tool,
			 int       action,
			 gpointer  gdisp_ptr)
{
  ByColorSelect * by_color_sel;

  by_color_sel = (ByColorSelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (by_color_dialog)
	{
	  if (by_color_dialog->gimage)
	    by_color_dialog->gimage->by_color_select = FALSE;
	  by_color_dialog->gimage = NULL;
	  by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
	}
      break;
    }
}

Tool *
tools_new_by_color_select ()
{
  Tool * tool;
  ByColorSelect * private;

  /*  The tool options  */
  if (!by_color_options)
    by_color_options = create_selection_options (BY_COLOR_SELECT);

  /*  The "by color" dialog  */
  if (!by_color_dialog)
    by_color_dialog = by_color_select_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (by_color_dialog->shell))
      gtk_widget_show (by_color_dialog->shell);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (ByColorSelect *) g_malloc (sizeof (ByColorSelect));

  tool->type = BY_COLOR_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = by_color_select_button_press;
  tool->button_release_func = by_color_select_button_release;
  tool->motion_func = by_color_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = by_color_select_cursor_update;
  tool->control_func = by_color_select_control;

  return tool;
}

void
tools_free_by_color_select (Tool *tool)
{
  ByColorSelect * by_color_sel;

  by_color_sel = (ByColorSelect *) tool->private;

  /*  Close the color select dialog  */
  if (by_color_dialog)
    {
      if (by_color_dialog->gimage)
	by_color_dialog->gimage->by_color_select = FALSE;
      by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
    }

  g_free (by_color_sel);
}

void
by_color_select_initialize (void *gimage_ptr)
{
  GImage *gimage;

  gimage = (GImage *) gimage_ptr;

  /*  update the preview window  */
  if (by_color_dialog)
    {
      by_color_dialog->gimage = gimage;
      gimage->by_color_select = TRUE;
      by_color_select_render (by_color_dialog, gimage);
      by_color_select_draw (by_color_dialog, gimage);
    }
}


/****************************/
/*  Select by Color dialog  */
/****************************/

ByColorDialog *
by_color_select_new_dialog ()
{
  ByColorDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *options_box;
  GtkWidget *label;
  GtkWidget *util_box;
  GtkWidget *push_button;
  GtkWidget *slider;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkObject *data;
  GSList *group = NULL;
  int i;
  char *button_names[4] =
  {
    "Replace",
    "Add",
    "Subtract",
    "Intersect"
  };
  int button_values[4] =
  {
    REPLACE,
    ADD,
    SUB,
    INTERSECT
  };

  bcd = g_malloc (sizeof (ByColorDialog));
  bcd->gimage = NULL;
  bcd->operation = REPLACE;
  bcd->threshold = DEFAULT_FUZZINESS;

  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "by_color_selection", "Gimp");
  gtk_window_set_title (GTK_WINDOW (bcd->shell), "By Color Selection");
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (bcd->shell)->action_area), 2);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      (GtkSignalFunc) by_color_select_delete_callback,
		      bcd);

  /*  The vbox */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The horizontal box containing preview  & options box */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview  */
  util_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), util_box, FALSE, FALSE, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (util_box), frame, FALSE, FALSE, 0);
  bcd->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (bcd->preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (bcd->preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (bcd->preview), "button_press_event",
		      (GtkSignalFunc) by_color_select_preview_events,
		      bcd);
  gtk_container_add (GTK_CONTAINER (frame), bcd->preview);

  gtk_widget_show (bcd->preview);
  gtk_widget_show (frame);
  gtk_widget_show (util_box);

  /*  options box  */
  options_box = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (options_box), 5);
  gtk_box_pack_start (GTK_BOX (hbox), options_box, TRUE, TRUE, 0);

  /*  Create the active brush label  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);
  bcd->gimage_name = gtk_label_new ("Inactive");
  gtk_box_pack_start (GTK_BOX (util_box), bcd->gimage_name, FALSE, FALSE, 2);

  gtk_widget_show (bcd->gimage_name);
  gtk_widget_show (util_box);

  /*  Create the selection mode radio box  */
  frame = gtk_frame_new ("Selection Mode");
  gtk_box_pack_start (GTK_BOX (options_box), frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < (sizeof(button_names) / sizeof(button_names[0])); i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) by_color_select_type_callback,
			  (gpointer) ((long) button_values[i]));
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (frame);

  /*  Create the opacity scale widget  */
  util_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new ("Fuzziness Threshold");
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  data = gtk_adjustment_new (bcd->threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) by_color_select_fuzzy_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  /*  The reset push button  */
  push_button = gtk_button_new_with_label ("Reset");
  GTK_WIDGET_SET_FLAGS (push_button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->action_area), push_button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (push_button), "clicked",
		      (GtkSignalFunc) by_color_select_reset_callback,
		      bcd);
  gtk_widget_grab_default (push_button);
  gtk_widget_show (push_button);

  /*  The close push button  */
  push_button = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (push_button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->action_area), push_button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (push_button), "clicked",
		      (GtkSignalFunc) by_color_select_close_callback,
		      bcd);
  gtk_widget_show (push_button);

  gtk_widget_show (options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
by_color_select_render (ByColorDialog *bcd,
			GImage        *gimage)
{
  Channel * mask;
  MaskBuf * scaled_buf = NULL;
  unsigned char *buf;
  PixelRegion srcPR, destPR;
  unsigned char * src;
  int subsample;
  int width, height;
  int srcwidth;
  int i;
  int scale;

  mask = gimage_get_mask (gimage);
  if ((drawable_width (GIMP_DRAWABLE(mask)) > PREVIEW_WIDTH) ||
      (drawable_height (GIMP_DRAWABLE(mask)) > PREVIEW_HEIGHT))
    {
      if (((float) drawable_width (GIMP_DRAWABLE (mask)) / (float) PREVIEW_WIDTH) >
	  ((float) drawable_height (GIMP_DRAWABLE (mask)) / (float) PREVIEW_HEIGHT))
	{
	  width = PREVIEW_WIDTH;
	  height = (drawable_height (GIMP_DRAWABLE (mask)) * PREVIEW_WIDTH) / drawable_width (GIMP_DRAWABLE (mask));
	}
      else
	{
	  width = (drawable_width (GIMP_DRAWABLE (mask)) * PREVIEW_HEIGHT) / drawable_height (GIMP_DRAWABLE (mask));
	  height = PREVIEW_HEIGHT;
	}

      scale = TRUE;
    }
  else
    {
      width = drawable_width (GIMP_DRAWABLE (mask));
      height = drawable_height (GIMP_DRAWABLE (mask));

      scale = FALSE;
    }

  if ((width != bcd->preview->requisition.width) ||
      (height != bcd->preview->requisition.height))
    gtk_preview_size (GTK_PREVIEW (bcd->preview), width, height);

  /*  clear the image buf  */
  buf = (unsigned char *) g_malloc (bcd->preview->requisition.width);
  memset (buf, 0, bcd->preview->requisition.width);
  for (i = 0; i < bcd->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (bcd->preview), buf, 0, i, bcd->preview->requisition.width);
  g_free (buf);

  /*  if the mask is empty, no need to scale and update again  */
  if (gimage_mask_is_empty (gimage))
    return;

  if (scale)
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      while ((width * (subsample + 1) * 2 < drawable_width (GIMP_DRAWABLE (mask))) &&
	     (height * (subsample + 1) * 2 < drawable_height (GIMP_DRAWABLE (mask))))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 drawable_width (GIMP_DRAWABLE (mask)), 
			 drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      subsample_region (&srcPR, &destPR, subsample);
    }
  else
    {
      pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 drawable_width (GIMP_DRAWABLE (mask)), 
			 drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      copy_region (&srcPR, &destPR);
    }

  src = mask_buf_data (scaled_buf);
  srcwidth = scaled_buf->width;
  for (i = 0; i < height; i++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (bcd->preview), src, 0, i, width);
      src += srcwidth;
    }

  mask_buf_free (scaled_buf);
}

static void
by_color_select_draw (ByColorDialog *bcd,
		      GImage        *gimage)
{
  /*  Draw the image buf to the preview window  */
  gtk_widget_draw (bcd->preview, NULL);

  /*  Update the gimage label to reflect the displayed gimage name  */
  gtk_label_set (GTK_LABEL (bcd->gimage_name), prune_filename (gimage_filename (gimage)));
}

static gint
by_color_select_preview_events (GtkWidget      *widget,
				GdkEventButton *bevent,
				ByColorDialog  *bcd)
{
  switch (bevent->type)
    {
    case GDK_BUTTON_PRESS:
      by_color_select_preview_button_press (bcd, bevent);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
by_color_select_type_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  if (by_color_dialog)
    by_color_dialog->operation = (long) client_data;
}

static void
by_color_select_reset_callback (GtkWidget *widget,
				gpointer   client_data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) client_data;
  if (!bcd->gimage)
    return;

  /*  reset the mask  */
  gimage_mask_clear (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static gint
by_color_select_delete_callback (GtkWidget *w,
				 GdkEvent  *e,
				 gpointer   client_data)
{
  by_color_select_close_callback (w, client_data);

  return FALSE;
}

static void
by_color_select_close_callback (GtkWidget *widget,
				gpointer   client_data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);
}

static void
by_color_select_fuzzy_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;
  bcd->threshold = (int) adjustment->value;
}

static void
by_color_select_preview_button_press (ByColorDialog  *bcd,
				      GdkEventButton *bevent)
{
  int x, y;
  int replace, operation;
  GimpDrawable *drawable;
  Tile *tile;
  unsigned char *col;

  if (!bcd->gimage)
    return;
  drawable = gimage_active_drawable (bcd->gimage);

  /*  Defaults  */
  replace = FALSE;
  operation = REPLACE;

  /*  Based on modifiers, and the "by color" dialog's selection mode  */
  if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    operation = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    operation = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    operation = INTERSECT;
  else
    operation = by_color_dialog->operation;

  /*  Find x, y and modify selection  */

  /*  Get the start color  */
  if (by_color_options->sample_merged)
    {
      x = bcd->gimage->width * bevent->x / bcd->preview->requisition.width;
      y = bcd->gimage->height * bevent->y / bcd->preview->requisition.height;
      if (x < 0 || y < 0 || x >= bcd->gimage->width || y >= bcd->gimage->height)
	return;
      tile = tile_manager_get_tile (gimage_composite (bcd->gimage), x, y, 0);
      tile_ref (tile);
      col = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
    }
  else
    {
      int offx, offy;

      drawable_offsets (drawable, &offx, &offy);
      x = drawable_width (drawable) * bevent->x / bcd->preview->requisition.width - offx;
      y = drawable_height (drawable) * bevent->y / bcd->preview->requisition.height - offy;
      if (x < 0 || y < 0 || x >= drawable_width (drawable) || y >= drawable_height (drawable))
	return;
      tile = tile_manager_get_tile (drawable_data (drawable), x, y, 0);
      tile_ref (tile);
      col = tile->data + tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
    }

  by_color_select (bcd->gimage, drawable, col,
		   bcd->threshold,
		   operation,
		   by_color_options->antialias,
		   by_color_options->feather,
		   by_color_options->feather_radius,
		   by_color_options->sample_merged);

  tile_unref (tile, FALSE);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}


/*  The by_color_select procedure definition  */
ProcArg by_color_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_COLOR,
    "color",
    "the color to select"
  },
  { PDB_INT32,
    "threshold",
    "threshold in intensity levels: 0 <= threshold <= 255"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing On/Off"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  }
};

ProcRecord by_color_select_proc =
{
  "gimp_by_color_select",
  "Create a selection by selecting all pixels (in the specified drawable) with the same (or similar) color to that specified.",
  "This tool creates a selection over the specified image.  A by-color selection is determined by the supplied color under the constraints of the specified threshold.  Essentially, all pixels (in the drawable) that have color sufficiently close to the specified color (as determined by the threshold value) are included in the selection.  The antialiasing parameter allows the final selection mask to contain intermediate values based on close misses to the threshold bar.  Feathering can be enabled optionally and is controlled with the \"feather_radius\" paramter.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  by_color_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { by_color_select_invoker } },
};


static Argument *
by_color_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int op;
  int threshold;
  int antialias;
  int feather;
  int sample_merged;
  unsigned char color[3];
  double feather_radius;
  int int_value;

  drawable    = NULL;
  op          = REPLACE;
  threshold   = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  color  */
  if (success)
    {
      int i;
      unsigned char *color_array;

      color_array = (unsigned char *) args[2].value.pdb_pointer;
      for (i = 0; i < 3; i++)
	color[i] = color_array[i];
    }
  /*  threshold  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value <= 255)
	threshold = int_value;
      else
	success = FALSE;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing?  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[7].value.pdb_float;
    }
  /*  sample merged  */
  if (success)
    {
      int_value = args[8].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }

  /*  call the ellipse_select procedure  */
  if (success)
    by_color_select (gimage, drawable, color, threshold, op,
		     antialias, feather, feather_radius, sample_merged);

  return procedural_db_return_args (&by_color_select_proc, success);
}
