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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/pixel-region.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"

#include "widgets/gimphistogramview.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpthresholdtool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "app_procs.h"
#include "gdisplay.h"
#include "image_map.h"

#include "libgimp/gimpintl.h"


#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150

#define LOW        0x1
#define HIGH       0x2
#define HISTOGRAM  0x4
#define ALL       (LOW | HIGH | HISTOGRAM)


/*  local function prototypes  */

static void   gimp_threshold_tool_class_init (GimpThresholdToolClass *klass);
static void   gimp_threshold_tool_init       (GimpThresholdTool      *threshold_tool);

static void   gimp_threshold_tool_destroy    (GtkObject  *object);

static void   gimp_threshold_tool_initialize (GimpTool   *tool,
					      GDisplay   *gdisp);
static void   gimp_threshold_tool_control    (GimpTool   *tool,
					      ToolAction  action,
					      GDisplay   *gdisp);

static ThresholdDialog * threshold_dialog_new (void);

static void   threshold_update                (ThresholdDialog *td,
					       gint             update);
static void   threshold_preview               (ThresholdDialog *td);
static void   threshold_reset_callback        (GtkWidget       *widget,
					       gpointer         data);
static void   threshold_ok_callback           (GtkWidget       *widget,
					       gpointer         data);
static void   threshold_cancel_callback       (GtkWidget       *widget,
					       gpointer         data);
static void   threshold_preview_update        (GtkWidget       *widget,
					       gpointer         data);
static void   threshold_low_threshold_adjustment_update  (GtkAdjustment *adj,
							  gpointer       data);
static void   threshold_high_threshold_adjustment_update (GtkAdjustment *adj,
							  gpointer       data);

static void   threshold                       (PixelRegion     *,
					       PixelRegion     *,
					       gpointer         );
static void   threshold_histogram_range       (HistogramWidget *,
					       gint             ,
					       gint             ,
					       gpointer         );


/*  the threshold tool options  */
static GimpToolOptions *threshold_options = NULL;

/*  the threshold tool dialog  */
static ThresholdDialog *threshold_dialog = NULL;

static GimpImageMapToolClass *parent_class = NULL;


/*  functions  */

void
gimp_threshold_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_THRESHOLD_TOOL,
                              FALSE,
			      "gimp:threshold_tool",
			      _("Threshold"),
			      _("Reduce image to two colors using a threshold"),
			      N_("/Image/Colors/Threshold..."), NULL,
			      NULL, "tools/threshold.html",
			      GIMP_STOCK_TOOL_THRESHOLD);
}

GtkType
gimp_threshold_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpThresholdTool",
        sizeof (GimpThresholdTool),
        sizeof (GimpThresholdToolClass),
        (GtkClassInitFunc) gimp_threshold_tool_class_init,
        (GtkObjectInitFunc) gimp_threshold_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_IMAGE_MAP_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_threshold_tool_class_init (GimpThresholdToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_IMAGE_MAP_TOOL);

  object_class->destroy  = gimp_threshold_tool_destroy;

  tool_class->initialize = gimp_threshold_tool_initialize;
  tool_class->control    = gimp_threshold_tool_control;
}

static void
gimp_threshold_tool_init (GimpThresholdTool *bc_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (bc_tool);

  if (! threshold_options)
    {
      threshold_options = tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_THRESHOLD_TOOL,
					  (GimpToolOptions *) threshold_options);
    }
}

static void
gimp_threshold_tool_destroy (GtkObject *object)
{
  threshold_dialog_hide ();

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_threshold_tool_initialize (GimpTool *tool,
				GDisplay *gdisp)
{
  if (! gdisp)
    {
      threshold_dialog_hide ();
      return;
    }

  if (gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
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

  threshold_dialog->drawable = gimp_image_active_drawable (gdisp->gimage);
  threshold_dialog->color = gimp_drawable_is_rgb (threshold_dialog->drawable);
  threshold_dialog->image_map =
    image_map_create (gdisp, threshold_dialog->drawable);

  gimp_drawable_calculate_histogram (threshold_dialog->drawable,
				     threshold_dialog->hist);

  g_signal_handlers_block_by_func (G_OBJECT (threshold_dialog->histogram),
                                   threshold_histogram_range,
                                   threshold_dialog);
  histogram_widget_update (threshold_dialog->histogram,
			   threshold_dialog->hist);
  g_signal_handlers_unblock_by_func (G_OBJECT (threshold_dialog->histogram),
                                     threshold_histogram_range,
                                     threshold_dialog);
  
  threshold_update (threshold_dialog, ALL);

  if (threshold_dialog->preview)
    threshold_preview (threshold_dialog);
}

static void
gimp_threshold_tool_control (GimpTool   *tool,
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
      threshold_dialog_hide ();
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}


/*  threshold machinery  */

void
threshold_2 (gpointer     data,
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

void
threshold_dialog_hide (void)
{
  if (threshold_dialog)
    threshold_cancel_callback (NULL, (gpointer) threshold_dialog);
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
		     tool_manager_help_func, NULL,
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_OK, threshold_ok_callback,
		     td, NULL, NULL, TRUE, FALSE,
		     _("Reset"), threshold_reset_callback,
		     td, NULL, NULL, TRUE, FALSE,
		     GTK_STOCK_CANCEL, threshold_cancel_callback,
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

  g_signal_connect (G_OBJECT (td->low_threshold_data), "value_changed",
                    GTK_SIGNAL_FUNC (threshold_low_threshold_adjustment_update),
                    td);

  gtk_widget_show (spinbutton);

  /* high threshold spinbutton  */
  data = gtk_adjustment_new (td->high_threshold, 0.0, 255.0, 1.0, 10.0, 0.0);
  td->high_threshold_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (td->high_threshold_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (td->high_threshold_data), "value_changed",
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

  g_signal_connect (G_OBJECT (td->histogram), "range_changed",
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

  g_signal_connect (G_OBJECT (toggle), "toggled",
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
  GimpTool *active_tool;

  if (!td->image_map)
    {
      g_warning ("threshold_preview(): No image map");
      return;
    }

  active_tool = tool_manager_get_active (the_gimp);

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
  GimpTool        *active_tool;

  td = (ThresholdDialog *) data;

  gimp_dialog_hide (td->shell);

  active_tool = tool_manager_get_active (the_gimp);

  active_tool->preserve = TRUE;

  if (!td->preview)
    image_map_apply (td->image_map, threshold, (void *) td);

  if (td->image_map)
    image_map_commit (td->image_map);

  active_tool->preserve = FALSE;

  td->image_map = NULL;

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
threshold_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  ThresholdDialog *td;
  GimpTool        *active_tool;

  td = (ThresholdDialog *) data;

  gimp_dialog_hide (td->shell);

  active_tool = tool_manager_get_active (the_gimp);

  if (td->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (td->image_map);
      active_tool->preserve = FALSE;

      td->image_map = NULL;
      gdisplays_flush ();
    }

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
threshold_preview_update (GtkWidget *widget,
			  gpointer   data)
{
  ThresholdDialog *td;
  GimpTool        *active_tool;

  td = (ThresholdDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      td->preview = TRUE;
      threshold_preview (td);
    }
  else
    {    
      td->preview = FALSE;
      if (td->image_map)
	{
	  active_tool = tool_manager_get_active (the_gimp);

	  active_tool->preserve = TRUE;
	  image_map_clear (td->image_map);
	  active_tool->preserve = FALSE;
	  gdisplays_flush ();
	}
    }
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
