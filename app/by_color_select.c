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
#include "appenv.h"
#include "boundary.h"
#include "by_color_select.h"
#include "colormaps.h"
#include "drawable.h"
#include "draw_core.h"
#include "gimage_mask.h"
#include "gimpdnd.h"
#include "gimprc.h"
#include "gimpset.h"
#include "gimpui.h"
#include "gdisplay.h"
#include "selection_options.h"

#include "tile.h"			/* ick. */

#include "libgimp/gimpintl.h"

#define DEFAULT_FUZZINESS    15
#define PREVIEW_WIDTH       256
#define PREVIEW_HEIGHT      256
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK

/*  the by color selection structures  */

typedef struct _ByColorSelect ByColorSelect;

struct _ByColorSelect
{
  gint  x, y;       /*  Point from which to execute seed fill  */
  gint  operation;  /*  add, subtract, normal color selection  */
};

typedef struct _ByColorDialog ByColorDialog;

struct _ByColorDialog
{
  GtkWidget *shell;

  GtkWidget *preview;
  GtkWidget *gimage_name;

  gint       threshold;  /*  threshold value for color select             */
  gint       operation;  /*  Add, Subtract, Replace                       */
  GImage    *gimage;     /*  gimage which is currently under examination  */
};

/*  the by color selection tool options  */
static SelectionOptions * by_color_options = NULL;

/*  the by color selection dialog  */
static ByColorDialog    * by_color_dialog = NULL;

/*  dnd stuff  */
static GtkTargetEntry by_color_select_targets[] =
{
  GIMP_TARGET_COLOR
};
static guint n_by_color_select_targets =  (sizeof (by_color_select_targets) /
					   sizeof (by_color_select_targets[0]));

static void by_color_select_color_drop (GtkWidget *, guchar, guchar, guchar,
					gpointer);

/*  by_color select action functions  */

static void by_color_select_button_press   (Tool *, GdkEventButton *, gpointer);
static void by_color_select_button_release (Tool *, GdkEventButton *, gpointer);
static void by_color_select_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void by_color_select_control        (Tool *, ToolAction,       gpointer);

static ByColorDialog * by_color_select_dialog_new (void);

static void   by_color_select_render               (ByColorDialog *, GImage *);
static void   by_color_select_draw                 (ByColorDialog *, GImage *);
static gint   by_color_select_preview_events       (GtkWidget *,
						    GdkEventButton *,
						    ByColorDialog *);
static void   by_color_select_type_callback        (GtkWidget *, gpointer);
static void   by_color_select_reset_callback       (GtkWidget *, gpointer);
static void   by_color_select_close_callback       (GtkWidget *, gpointer);
static void   by_color_select_fuzzy_update         (GtkAdjustment *, gpointer);
static void   by_color_select_preview_button_press (ByColorDialog *,
						    GdkEventButton *);

static gint       is_pixel_sufficiently_different (guchar *, guchar *,
						   gint, gint, gint, gint);
static Channel *  by_color_select_color           (GImage *, GimpDrawable *,
						   guchar *, gint, gint, gint);


/*  by_color selection machinery  */

static int
is_pixel_sufficiently_different (guchar *col1,
				 guchar *col2,
				 gint    antialias,
				 gint    threshold,
				 gint    bytes,
				 gint    has_alpha)
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
	return (guchar) (aa * 512);
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
by_color_select_color (GImage       *gimage,
		       GimpDrawable *drawable,
		       guchar       *color,
		       gint          antialias,
		       gint          threshold,
		       gint          sample_merged)
{
  /*  Scan over the gimage's active layer, finding pixels within the specified
   *  threshold from the given R, G, & B values.  If antialiasing is on,
   *  use the same antialiasing scheme as in fuzzy_select.  Modify the gimage's
   *  mask to reflect the additional selection
   */
  Channel *mask;
  PixelRegion imagePR, maskPR;
  guchar *image_data;
  guchar *mask_data;
  guchar *idata, *mdata;
  guchar rgb[MAX_CHANNELS];
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
      pixel_region_init (&imagePR, gimage_composite (gimage),
			 0, 0, width, height, FALSE);
    }
  else
    {
      bytes = drawable_bytes (drawable);
      d_type = drawable_type (drawable);
      has_alpha = drawable_has_alpha (drawable);
      indexed = drawable_indexed (drawable);
      width = drawable_width (drawable);
      height = drawable_height (drawable);

      pixel_region_init (&imagePR, drawable_data (drawable),
			 0, 0, width, height, FALSE);
    }

  if (indexed)
    {
      /* indexed colors are always RGB or RGBA */
      color_bytes = has_alpha ? 4 : 3;
    }
  else
    {
      /* RGB, RGBA, GRAY and GRAYA colors are shaped just like the image */
      color_bytes = bytes;
    }

  alpha = bytes - 1;
  mask = channel_new_mask (gimage, width, height);
  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 
		     0, 0, width, height, TRUE);

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (2, &imagePR, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
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
	      *mdata++ = is_pixel_sufficiently_different (color,
							  rgb,
							  antialias,
							  threshold,
							  color_bytes,
							  has_alpha);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	  mask_data += maskPR.rowstride;
	}
    }

  return mask;
}

void
by_color_select (GImage       *gimage,
		 GimpDrawable *drawable,
		 guchar       *color,
		 gint          threshold,
		 gint          op,
		 gint          antialias,
		 gint          feather,
		 gdouble       feather_radius,
		 gint          sample_merged)
{
  Channel * new_mask;
  int off_x, off_y;

  if (!drawable) 
    return;

  new_mask = by_color_select_color (gimage, drawable, color,
				    antialias, threshold, sample_merged);

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
		     feather_radius,
		     feather_radius,
		     op, off_x, off_y);
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

  tool->drawable = gimage_active_drawable (gdisp->gimage);

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

  if (by_color_dialog->gimage != gdisp->gimage)
    {
      gdk_draw_rectangle
	(by_color_dialog->preview->window,
	 by_color_dialog->preview->style->bg_gc[GTK_STATE_NORMAL],
	 TRUE,
	 0, 0,
	 by_color_dialog->preview->allocation.width,
	 by_color_dialog->preview->allocation.width);
    }

  by_color_dialog->gimage = gdisp->gimage;
  gdisp->gimage->by_color_select = TRUE;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
                    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK |
                    GDK_BUTTON_RELEASE_MASK,
                    NULL, NULL, bevent->time);
}

static void
by_color_select_button_release (Tool           *tool,
				GdkEventButton *bevent,
				gpointer        gdisp_ptr)
{
  ByColorSelect * by_color_sel;
  GDisplay * gdisp;
  gint x, y;
  GimpDrawable *drawable;
  guchar *color;
  gint use_offsets;

  gdisp = (GDisplay *) gdisp_ptr;
  by_color_sel = (ByColorSelect *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  gdk_pointer_ungrab (bevent->time);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      use_offsets = (by_color_options->sample_merged) ? FALSE : TRUE;
      gdisplay_untransform_coords (gdisp, by_color_sel->x, by_color_sel->y,
				   &x, &y, FALSE, use_offsets);

      /*  Get the start color  */
      if (by_color_options->sample_merged)
	{
	  if (!(color = gimp_image_get_color_at (gdisp->gimage, x, y)))
	    return;
	}
      else
	{
	  if (!(color = gimp_drawable_get_color_at (drawable, x, y)))
	    return;
	}

      /*  select the area  */
      by_color_select (gdisp->gimage, drawable, color,
		       by_color_dialog->threshold,
		       by_color_sel->operation,
		       by_color_options->antialias,
		       by_color_options->feather,
		       by_color_options->feather_radius,
		       by_color_options->sample_merged);

      g_free (color);

      /*  show selection on all views  */
      gdisplays_flush ();

      /*  update the preview window  */
      by_color_select_render (by_color_dialog, gdisp->gimage);
      by_color_select_draw (by_color_dialog, gdisp->gimage);
    }
}

static void
by_color_select_cursor_update (Tool           *tool,
			       GdkEventMotion *mevent,
			       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
    if (layer == gdisp->gimage->active_layer)
      {
	gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
	return;
      }

  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
by_color_select_control (Tool       *tool,
			 ToolAction  action,
			 gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;

    case RESUME :
      break;

    case HALT :
      if (by_color_dialog)
	by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
      break;

    default:
      break;
    }
}

static void
by_color_select_options_reset (void)
{
  selection_options_reset (by_color_options);
}

Tool *
tools_new_by_color_select (void)
{
  Tool * tool;
  ByColorSelect * private;

  /*  The tool options  */
  if (!by_color_options)
    {
      by_color_options =
	selection_options_new (BY_COLOR_SELECT, by_color_select_options_reset);
      tools_register (BY_COLOR_SELECT, (ToolOptions *) by_color_options);
    }

  /*  The "by color" dialog  */
  if (!by_color_dialog)
    by_color_dialog = by_color_select_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (by_color_dialog->shell))
      gtk_widget_show (by_color_dialog->shell);

  tool = tools_new_tool (BY_COLOR_SELECT);
  private = g_new (ByColorSelect, 1);

  tool->scroll_lock = TRUE;  /*  Disallow scrolling  */

  tool->private = (void *) private;

  tool->button_press_func   = by_color_select_button_press;
  tool->button_release_func = by_color_select_button_release;
  tool->cursor_update_func  = by_color_select_cursor_update;
  tool->control_func        = by_color_select_control;

  return tool;
}

void
tools_free_by_color_select (Tool *tool)
{
  ByColorSelect * by_color_sel;

  by_color_sel = (ByColorSelect *) tool->private;

  /*  Close the color select dialog  */
  if (by_color_dialog)
    by_color_select_close_callback (NULL, (gpointer) by_color_dialog);

  g_free (by_color_sel);
}

void
by_color_select_initialize_by_image (GImage *gimage)
{
  /*  update the preview window  */
  if (by_color_dialog)
    {
      by_color_dialog->gimage = gimage;
      gimage->by_color_select = TRUE;
      by_color_select_render (by_color_dialog, gimage);
      by_color_select_draw (by_color_dialog, gimage);
    }
}

void
by_color_select_initialize (GDisplay *gdisp)
{
  by_color_select_initialize_by_image (gdisp->gimage);
}

/****************************/
/*  Select by Color dialog  */
/****************************/

static ByColorDialog *
by_color_select_dialog_new (void)
{
  ByColorDialog *bcd;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *options_box;
  GtkWidget *label;
  GtkWidget *util_box;
  GtkWidget *slider;
  GtkObject *data;

  bcd = g_new (ByColorDialog, 1);
  bcd->gimage    = NULL;
  bcd->operation = REPLACE;
  bcd->threshold = DEFAULT_FUZZINESS;

  /*  The shell and main vbox  */
  bcd->shell = gimp_dialog_new (_("By Color Selection"), "by_color_selection",
				tools_help_func, NULL,
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Reset"), by_color_select_reset_callback,
				bcd, NULL, FALSE, FALSE,
				_("Close"), by_color_select_close_callback,
				bcd, NULL, TRUE, TRUE,

				NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bcd->shell)->vbox), hbox);

  /*  The preview  */
  util_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), util_box, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (util_box), frame, FALSE, FALSE, 0);

  bcd->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (bcd->preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (bcd->preview, PREVIEW_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (frame), bcd->preview);

  gtk_signal_connect (GTK_OBJECT (bcd->preview), "button_press_event",
		      GTK_SIGNAL_FUNC (by_color_select_preview_events),
		      bcd);

  /*  dnd colors to the image window  */
  gtk_drag_dest_set (bcd->preview, 
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP, 
                     by_color_select_targets,
                     n_by_color_select_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (bcd->preview, by_color_select_color_drop, bcd);

  gtk_widget_show (bcd->preview);
  gtk_widget_show (frame);
  gtk_widget_show (util_box);

  /*  options box  */
  options_box = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), options_box, FALSE, FALSE, 0);

  /*  Create the active image label  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);

  bcd->gimage_name = gtk_label_new (_("Inactive"));
  gtk_box_pack_start (GTK_BOX (util_box), bcd->gimage_name, FALSE, FALSE, 0);

  gtk_widget_show (bcd->gimage_name);
  gtk_widget_show (util_box);

  /*  Create the selection mode radio box  */
  frame = 
    gimp_radio_group_new (TRUE, _("Selection Mode"),

			  _("Replace"), by_color_select_type_callback,
			  (gpointer) REPLACE, NULL, NULL, TRUE,
			  _("Add"), by_color_select_type_callback,
			  (gpointer) ADD, NULL, NULL, FALSE,
			  _("Subtract"), by_color_select_type_callback,
			  (gpointer) SUB, NULL, NULL, FALSE,
			  _("Intersect"), by_color_select_type_callback,
			  (gpointer) INTERSECT, NULL, NULL, FALSE,

			  NULL);

  gtk_box_pack_start (GTK_BOX (options_box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Create the opacity scale widget  */
  util_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fuzziness Threshold"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (util_box);

  data = gtk_adjustment_new (bcd->threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      GTK_SIGNAL_FUNC (by_color_select_fuzzy_update),
		      bcd);

  gtk_widget_show (slider);

  gtk_widget_show (options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
by_color_select_render (ByColorDialog *bcd,
			GImage        *gimage)
{
  Channel * mask;
  MaskBuf * scaled_buf = NULL;
  guchar *buf;
  PixelRegion srcPR, destPR;
  guchar * src;
  gint subsample;
  gint width, height;
  gint srcwidth;
  gint i;
  gint scale;

  mask = gimage_get_mask (gimage);
  if ((drawable_width (GIMP_DRAWABLE(mask)) > PREVIEW_WIDTH) ||
      (drawable_height (GIMP_DRAWABLE(mask)) > PREVIEW_HEIGHT))
    {
      if (((float) drawable_width (GIMP_DRAWABLE (mask)) / (float) PREVIEW_WIDTH) >
	  ((float) drawable_height (GIMP_DRAWABLE (mask)) / (float) PREVIEW_HEIGHT))
	{
	  width = PREVIEW_WIDTH;
	  height = ((drawable_height (GIMP_DRAWABLE (mask)) * PREVIEW_WIDTH) /
		    drawable_width (GIMP_DRAWABLE (mask)));
	}
      else
	{
	  width = ((drawable_width (GIMP_DRAWABLE (mask)) * PREVIEW_HEIGHT) /
		   drawable_height (GIMP_DRAWABLE (mask)));
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
  buf = g_new0 (guchar, bcd->preview->requisition.width);
  for (i = 0; i < bcd->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (bcd->preview), buf,
			  0, i, bcd->preview->requisition.width);
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
  gtk_label_set_text (GTK_LABEL (bcd->gimage_name),
		      g_basename (gimage_filename (gimage)));
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
			       gpointer   data)
{
  if (by_color_dialog)
    by_color_dialog->operation = (long) data;
}

static void
by_color_select_reset_callback (GtkWidget *widget,
				gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  if (!bcd->gimage)
    return;

  /*  check if the image associated to the mask still exists  */
  if (!drawable_gimage (GIMP_DRAWABLE (gimage_get_mask (bcd->gimage))))
    return;

  /*  reset the mask  */
  gimage_mask_clear (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_close_callback (GtkWidget *widget,
				gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  if (bcd->gimage && gimp_set_have (image_context, bcd->gimage))
    {
      bcd->gimage->by_color_select = FALSE;
      bcd->gimage = NULL;
    }
}

static void
by_color_select_fuzzy_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  bcd->threshold = (gint) (adjustment->value + 0.5);
}

static void
by_color_select_preview_button_press (ByColorDialog  *bcd,
				      GdkEventButton *bevent)
{
  gint x, y;
  gint replace, operation;
  GimpDrawable *drawable;
  Tile *tile;
  guchar *col;

  if (!bcd->gimage)
    return;

  drawable = gimage_active_drawable (bcd->gimage);

  /*  check if the gimage associated to the drawable still exists  */
  if (!drawable_gimage (drawable))
    return;

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
      tile = tile_manager_get_tile (gimage_composite (bcd->gimage), x, y, TRUE, FALSE);
      col = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
    }
  else
    {
      int offx, offy;

      drawable_offsets (drawable, &offx, &offy);
      x = drawable_width (drawable) * bevent->x / bcd->preview->requisition.width - offx;
      y = drawable_height (drawable) * bevent->y / bcd->preview->requisition.height - offy;
      if (x < 0 || y < 0 || x >= drawable_width (drawable) || y >= drawable_height (drawable))
	return;
      tile = tile_manager_get_tile (drawable_data (drawable), x, y, TRUE, FALSE);
      col = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
    }

  by_color_select (bcd->gimage, drawable, col,
		   bcd->threshold,
		   operation,
		   by_color_options->antialias,
		   by_color_options->feather,
		   by_color_options->feather_radius,
		   by_color_options->sample_merged);

  tile_release (tile, FALSE);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_color_drop (GtkWidget *widget,
                            guchar      r,
                            guchar      g,
                            guchar      b,
                            gpointer    data)

{
  GimpDrawable *drawable;
  ByColorDialog *bcd;
  guchar col[3];

  bcd = (ByColorDialog*) data;
  drawable = gimage_active_drawable (bcd->gimage);
  
  col[0] = r;
  col[1] = g;
  col[2] = b;

  by_color_select (bcd->gimage, 
                   drawable, 
                   col, 
                   bcd->threshold,
                   bcd->operation,
                   by_color_options->antialias,
                   by_color_options->feather,
                   by_color_options->feather_radius,
                   by_color_options->sample_merged);     

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}
