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
#include "base/threshold.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimpthresholdtool.h"
#include "tool_manager.h"

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

static void   gimp_threshold_tool_finalize   (GObject          *object);

static void   gimp_threshold_tool_initialize (GimpTool         *tool,
					      GimpDisplay      *gdisp);

static void   gimp_threshold_tool_map        (GimpImageMapTool *image_map_tool);
static void   gimp_threshold_tool_dialog     (GimpImageMapTool *image_map_tool);
static void   gimp_threshold_tool_reset      (GimpImageMapTool *image_map_tool);

static void   threshold_update                (GimpThresholdTool *t_tool,
					       gint               update);
static void   threshold_low_threshold_adjustment_update  (GtkAdjustment *adj,
							  gpointer       data);
static void   threshold_high_threshold_adjustment_update (GtkAdjustment *adj,
							  gpointer       data);
static void   threshold_histogram_range                  (GimpHistogramView *,
                                                          gint               ,
                                                          gint               ,
                                                          gpointer           );


static GimpImageMapToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_threshold_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_THRESHOLD_TOOL,
                NULL,
                FALSE,
                "gimp-threshold-tool",
                _("Threshold"),
                _("Reduce image to two colors using a threshold"),
                N_("/Layer/Colors/Threshold..."), NULL,
                NULL, "tools/threshold.html",
                GIMP_STOCK_TOOL_THRESHOLD,
                data);
}

GType
gimp_threshold_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpThresholdToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_threshold_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpThresholdTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_threshold_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpThresholdTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_threshold_tool_class_init (GimpThresholdToolClass *klass)
{
  GObjectClass          *object_class;
  GimpToolClass         *tool_class;
  GimpImageMapToolClass *image_map_tool_class;

  object_class         = G_OBJECT_CLASS (klass);
  tool_class           = GIMP_TOOL_CLASS (klass);
  image_map_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize       = gimp_threshold_tool_finalize;

  tool_class->initialize       = gimp_threshold_tool_initialize;

  image_map_tool_class->map    = gimp_threshold_tool_map;
  image_map_tool_class->dialog = gimp_threshold_tool_dialog;
  image_map_tool_class->reset  = gimp_threshold_tool_reset;
}

static void
gimp_threshold_tool_init (GimpThresholdTool *t_tool)
{
  GimpImageMapTool *image_map_tool;

  image_map_tool = GIMP_IMAGE_MAP_TOOL (t_tool);

  image_map_tool->shell_title = _("Threshold");
  image_map_tool->shell_name  = "threshold";

  t_tool->threshold = g_new0 (Threshold, 1);
  t_tool->hist      = gimp_histogram_new ();

  t_tool->threshold->low_threshold  = 127;
  t_tool->threshold->high_threshold = 255;
}

static void
gimp_threshold_tool_finalize (GObject *object)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (object);

  if (t_tool->threshold)
    {
      g_free (t_tool->threshold);
      t_tool->threshold = NULL;
    }

  if (t_tool->hist)
    {
      gimp_histogram_free (t_tool->hist);
      t_tool->hist = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_threshold_tool_initialize (GimpTool    *tool,
				GimpDisplay *gdisp)
{
  GimpThresholdTool *t_tool;
  GimpDrawable      *drawable;

  t_tool = GIMP_THRESHOLD_TOOL (tool);

  if (gdisp &&
      gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Threshold does not operate on indexed drawables."));
      return;
    }

  drawable = gimp_image_active_drawable (gdisp->gimage);

  t_tool->threshold->color          = gimp_drawable_is_rgb (drawable);
  t_tool->threshold->low_threshold  = 127;
  t_tool->threshold->high_threshold = 255;

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);

  gimp_drawable_calculate_histogram (drawable, t_tool->hist);

  g_signal_handlers_block_by_func (G_OBJECT (t_tool->histogram),
                                   threshold_histogram_range,
                                   t_tool);
  gimp_histogram_view_update (t_tool->histogram, t_tool->hist);
  g_signal_handlers_unblock_by_func (G_OBJECT (t_tool->histogram),
                                     threshold_histogram_range,
                                     t_tool);

  threshold_update (t_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
}

static void
gimp_threshold_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);

  gimp_image_map_apply (image_map_tool->image_map,
                        threshold,
                        t_tool->threshold);
}


/**********************/
/*  Threshold dialog  */
/**********************/

static void
gimp_threshold_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool;
  GtkWidget         *hbox;
  GtkWidget         *spinbutton;
  GtkWidget         *label;
  GtkWidget         *frame;
  GtkObject         *data;

  t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);

  /*  Horizontal box for threshold text widget  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold Range:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low threshold spinbutton  */
  data = gtk_adjustment_new (t_tool->threshold->low_threshold,
                             0.0, 255.0, 1.0, 10.0, 0.0);
  t_tool->low_threshold_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (t_tool->low_threshold_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (G_OBJECT (t_tool->low_threshold_data), "value_changed",
                    G_CALLBACK (threshold_low_threshold_adjustment_update),
                    t_tool);

  /* high threshold spinbutton  */
  data = gtk_adjustment_new (t_tool->threshold->high_threshold,
                             0.0, 255.0, 1.0, 10.0, 0.0);
  t_tool->high_threshold_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (t_tool->high_threshold_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (G_OBJECT (t_tool->high_threshold_data), "value_changed",
                    G_CALLBACK (threshold_high_threshold_adjustment_update),
                    t_tool);

  gtk_widget_show (hbox);

  /*  The threshold histogram  */
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
  gtk_widget_show (frame);

  t_tool->histogram = gimp_histogram_view_new (HISTOGRAM_WIDTH,
                                               HISTOGRAM_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (t_tool->histogram));
  gtk_widget_show (GTK_WIDGET (t_tool->histogram));

  g_signal_connect (G_OBJECT (t_tool->histogram), "range_changed",
                    G_CALLBACK (threshold_histogram_range),
                    t_tool);
}

static void
gimp_threshold_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);

  t_tool->threshold->low_threshold  = 127.0;
  t_tool->threshold->high_threshold = 255.0;

  threshold_update (t_tool, ALL);
}

static void
threshold_update (GimpThresholdTool *t_tool,
		  gint               update)
{
  if (update & LOW)
    gtk_adjustment_set_value (t_tool->low_threshold_data,
                              t_tool->threshold->low_threshold);

  if (update & HIGH)
    gtk_adjustment_set_value (t_tool->high_threshold_data,
                              t_tool->threshold->high_threshold);

  if (update & HISTOGRAM)
    gimp_histogram_view_range (t_tool->histogram,
                               t_tool->threshold->low_threshold,
                               t_tool->threshold->high_threshold);
}

static void
threshold_low_threshold_adjustment_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (data);

  if (t_tool->threshold->low_threshold != adjustment->value)
    {
      t_tool->threshold->low_threshold = adjustment->value;

      threshold_update (t_tool, HISTOGRAM);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
    }
}

static void
threshold_high_threshold_adjustment_update (GtkAdjustment *adjustment,
					    gpointer       data)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (data);

  if (t_tool->threshold->high_threshold != adjustment->value)
    {
      t_tool->threshold->high_threshold = adjustment->value;

      threshold_update (t_tool, HISTOGRAM);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
    }
}

static void
threshold_histogram_range (GimpHistogramView *widget,
			   gint               start,
			   gint               end,
			   gpointer           data)
{
  GimpThresholdTool *t_tool;

  t_tool = GIMP_THRESHOLD_TOOL (data);

  t_tool->threshold->low_threshold  = start;
  t_tool->threshold->high_threshold = end;

  threshold_update (t_tool, LOW | HIGH);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
}
