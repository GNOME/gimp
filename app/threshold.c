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
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "interface.h"
#include "threshold.h"

#include "config.h"
#include "libgimp/gimpintl.h"

#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150

#define LOW        0x1
#define HIGH       0x2
#define HISTORGAM  0x4
#define ALL       (LOW | HIGH | HISTOGRAM)

/*  the threshold structures  */

typedef struct _Threshold Threshold;

struct _Threshold
{
  gint x, y;    /*  coords for last mouse click  */
};

/*  the threshold tool options  */
static ToolOptions *threshold_options = NULL;

/*  the threshold tool dialog  */
static ThresholdDialog *threshold_dialog = NULL;

/*  threshold action functions  */
static void   threshold_control (Tool *, ToolAction, gpointer);

static ThresholdDialog * threshold_dialog_new (void);

static void   threshold_update                     (ThresholdDialog *,
						    gint);
static void   threshold_preview                    (ThresholdDialog *);
static void   threshold_reset_callback             (GtkWidget *, gpointer);
static void   threshold_ok_callback                (GtkWidget *, gpointer);
static void   threshold_cancel_callback            (GtkWidget *, gpointer);
static void   threshold_preview_update             (GtkWidget *, gpointer);
static void   threshold_low_threshold_adjustment_update  (GtkAdjustment *,
							  gpointer);
static void   threshold_high_threshold_adjustment_update (GtkAdjustment *,
							  gpointer);

static void   threshold                 (PixelRegion *, PixelRegion *, void *);
static void   threshold_histogram_range (HistogramWidget *, gint, gint,
					 gpointer);
/*  threshold machinery  */

void
threshold_2 (void        *data,
	     PixelRegion *srcPR,
	     PixelRegion *destPR)
{
  /*  this function just re-orders the arguments so we can use 
   *  pixel_regions_process_paralell
   */
  threshold (srcPR, destPR, data);
}

static void
threshold (PixelRegion *srcPR,
	   PixelRegion *destPR,
	   void        *data)
{
  ThresholdDialog *td;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int has_alpha, alpha;
  int w, h, b;
  int value;

  td = (ThresholdDialog *) data;

  h = srcPR->h;
  src = srcPR->data;
  dest = destPR->data;
  has_alpha = (srcPR->bytes == 2 || srcPR->bytes == 4);
  alpha = has_alpha ? srcPR->bytes - 1 : srcPR->bytes;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;
      while (w--)
	{
	  if (td->color)
	    {
	      value = MAX (s[RED_PIX], s[GREEN_PIX]);
	      value = MAX (value, s[BLUE_PIX]);

	      value = (value >= td->low_threshold && value <= td->high_threshold ) ? 255 : 0;
	    }
	  else
	    value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 255 : 0;

	  for (b = 0; b < alpha; b++)
	    d[b] = value;

	  if (has_alpha)
	    d[alpha] = s[alpha];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

/*  threshold action functions  */

static void
threshold_control (Tool       *tool,
		   ToolAction  action,
		   gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (threshold_dialog)
	threshold_cancel_callback (NULL, (gpointer) threshold_dialog);
      break;

    default:
      break;
    }
}

Tool *
tools_new_threshold (void)
{
  Tool * tool;
  Threshold * private;

  /*  The tool options  */
  if (! threshold_options)
    {
      threshold_options = tool_options_new (_("Threshold"));
      tools_register (THRESHOLD, threshold_options);
    }

  tool = tools_new_tool (THRESHOLD);
  private = g_new (Threshold, 1);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->control_func = threshold_control;

  return tool;
}

void
tools_free_threshold (Tool *tool)
{
  Threshold * thresh;

  thresh = (Threshold *) tool->private;

  /*  Close the threshold dialog  */
  if (threshold_dialog)
    threshold_cancel_callback (NULL, (gpointer) threshold_dialog);

  g_free (thresh);
}

void
threshold_initialize (GDisplay *gdisp)
{
  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Threshold does not operate on indexed drawables."));
      return;
    }

  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);

  threshold_dialog->low_threshold  = 127;
  threshold_dialog->high_threshold = 255;

  threshold_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  threshold_dialog->color = drawable_color (threshold_dialog->drawable);
  threshold_dialog->image_map =
    image_map_create (gdisp, threshold_dialog->drawable);

  gimp_histogram_calculate_drawable (threshold_dialog->hist,
				     threshold_dialog->drawable);

  gtk_signal_handler_block_by_data (GTK_OBJECT (threshold_dialog->histogram),
				    threshold_dialog);
  histogram_widget_update (threshold_dialog->histogram,
			   threshold_dialog->hist);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (threshold_dialog->histogram),
				      threshold_dialog);

  threshold_update (threshold_dialog, ALL);

  if (threshold_dialog->preview)
    threshold_preview (threshold_dialog);
}

/**********************/
/*  Threshold dialog  */
/**********************/

static ThresholdDialog *
threshold_dialog_new (void)
{
  ThresholdDialog *td;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkObject *data;

  td = g_new (ThresholdDialog, 1);
  td->preview        = TRUE;
  td->low_threshold  = 127;
  td->high_threshold = 255;
  td->hist           = gimp_histogram_new ();

  /*  The shell and main vbox  */
  td->shell =
    gimp_dialog_new (_("Threshold"), "threshold",
		     tools_help_func, NULL,
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("OK"), threshold_ok_callback,
		     td, NULL, NULL, TRUE, FALSE,
		     _("Reset"), threshold_reset_callback,
		     td, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), threshold_cancel_callback,
		     td, NULL, NULL, FALSE, TRUE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (td->shell)->vbox), vbox);

  /*  Horizontal box for threshold text widget  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold Range:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low threshold spinbutton  */
  data = gtk_adjustment_new (td->low_threshold, 0.0, 255.0, 1.0, 10.0, 0.0);
  td->low_threshold_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (td->low_threshold_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (td->low_threshold_data), "value_changed",
		      GTK_SIGNAL_FUNC (threshold_low_threshold_adjustment_update),
		      td);

  gtk_widget_show (spinbutton);

  /* high threshold spinbutton  */
  data = gtk_adjustment_new (td->high_threshold, 0.0, 255.0, 1.0, 10.0, 0.0);
  td->high_threshold_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (td->high_threshold_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (td->high_threshold_data), "value_changed",
		      GTK_SIGNAL_FUNC (threshold_high_threshold_adjustment_update),
		      td);

  gtk_widget_show (spinbutton);

  gtk_widget_show (hbox);

  /*  The threshold histogram  */
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  td->histogram = histogram_widget_new (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (td->histogram));

  gtk_signal_connect (GTK_OBJECT (td->histogram), "rangechanged",
		      GTK_SIGNAL_FUNC (threshold_histogram_range),
		      td);

  gtk_widget_show (GTK_WIDGET(td->histogram));

  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), td->preview);
  gtk_box_pack_end (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (threshold_preview_update),
		      td);

  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (td->shell);

  return td;
}

static void
threshold_update (ThresholdDialog *td,
		  gint             update)
{
  if (update & LOW)
    {
      gtk_adjustment_set_value (td->low_threshold_data, td->low_threshold);
    }
  if (update & HIGH)
    {
      gtk_adjustment_set_value (td->high_threshold_data, td->high_threshold);
    }
  if (update & HISTOGRAM)
    {
      histogram_widget_range (td->histogram,
			      td->low_threshold,
			      td->high_threshold);
    }
}

static void
threshold_preview (ThresholdDialog *td)
{
  if (!td->image_map)
    {
      g_warning ("threshold_preview(): No image map");
      return;
    }

  active_tool->preserve = TRUE;
  image_map_apply (td->image_map, threshold, td);
  active_tool->preserve = FALSE;
}

static void
threshold_reset_callback (GtkWidget *widget,
			  gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  td->low_threshold  = 127.0;
  td->high_threshold = 255.0;

  threshold_update (td, ALL);

  if (td->preview)
    threshold_preview (td);
}

static void
threshold_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  gimp_dialog_hide (td->shell);
  
  active_tool->preserve = TRUE;

  if (!td->preview)
    image_map_apply (td->image_map, threshold, (void *) td);

  if (td->image_map)
    image_map_commit (td->image_map);

  active_tool->preserve = FALSE;

  td->image_map = NULL;

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
threshold_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  gimp_dialog_hide (td->shell);

  if (td->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (td->image_map);
      active_tool->preserve = FALSE;

      td->image_map = NULL;
      gdisplays_flush ();
    }

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
threshold_preview_update (GtkWidget *widget,
			  gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      td->preview = TRUE;
      threshold_preview (td);
    }
  else
    td->preview = FALSE;
}

static void
threshold_low_threshold_adjustment_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  if (td->low_threshold != adjustment->value)
    {
      td->low_threshold = adjustment->value;

      threshold_update (td, HISTOGRAM);

      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_adjustment_update (GtkAdjustment *adjustment,
					    gpointer       data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  if (td->high_threshold != adjustment->value)
    {
      td->high_threshold = adjustment->value;

      threshold_update (td, HISTOGRAM);

      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_histogram_range (HistogramWidget *widget,
			   gint             start,
			   gint             end,
			   gpointer         data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  td->low_threshold  = start;
  td->high_threshold = end;

  threshold_update (td, LOW | HIGH);

  if (td->preview)
    threshold_preview (td);
}
