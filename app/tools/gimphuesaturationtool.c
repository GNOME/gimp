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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/hue-saturation.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "display/gimpdisplay.h"

#include "gimphuesaturationtool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


#define HUE_PARTITION_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK

#define SLIDER_WIDTH  200
#define DA_WIDTH       40
#define DA_HEIGHT      20

#define HUE_PARTITION      0x0
#define HUE_SLIDER         0x1
#define LIGHTNESS_SLIDER   0x2
#define SATURATION_SLIDER  0x4
#define DRAW               0x40
#define ALL                0xFF


/*  local function prototypes  */

static void   gimp_hue_saturation_tool_class_init (GimpHueSaturationToolClass *klass);
static void   gimp_hue_saturation_tool_init       (GimpHueSaturationTool      *tool);

static void   gimp_hue_saturation_tool_finalize   (GObject          *object);

static void   gimp_hue_saturation_tool_initialize (GimpTool         *tool,
                                                   GimpDisplay      *gdisp);

static void   gimp_hue_saturation_tool_map        (GimpImageMapTool *image_map_tool);
static void   gimp_hue_saturation_tool_dialog     (GimpImageMapTool *image_map_tool);
static void   gimp_hue_saturation_tool_reset      (GimpImageMapTool *image_map_tool);

static void   hue_saturation_update                  (GimpHueSaturationTool *hs_tool,
						      gint                   update);
static void   hue_saturation_partition_callback      (GtkWidget *widget,
						      gpointer   data);
static void   hue_saturation_hue_adjustment_update        (GtkAdjustment *adj,
							   gpointer       data);
static void   hue_saturation_lightness_adjustment_update  (GtkAdjustment *adj,
							   gpointer       data);
static void   hue_saturation_saturation_adjustment_update (GtkAdjustment *adj,
							   gpointer       data);
static gint   hue_saturation_hue_partition_events    (GtkWidget          *widget,
						      GdkEvent           *event,
						      GimpHueSaturationTool *hs_tool);


/*  Local variables  */

static gint default_colors[6][3] =
{
  { 255,   0,   0 },
  { 255, 255,   0 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  {   0,   0, 255 },
  { 255,   0, 255 }
};

static GimpImageMapToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_hue_saturation_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_HUE_SATURATION_TOOL,
                NULL,
                FALSE,
                "gimp-hue-saturation-tool",
                _("Hue-Saturation"),
                _("Adjust hue and saturation"),
                N_("/Layer/Colors/Hue-Saturation..."), NULL,
                NULL, "tools/hue_saturation.html",
                GIMP_STOCK_TOOL_HUE_SATURATION,
                data);
}

GType
gimp_hue_saturation_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpHueSaturationToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_hue_saturation_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpHueSaturationTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_hue_saturation_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpHueSaturationTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_hue_saturation_tool_class_init (GimpHueSaturationToolClass *klass)
{
  GObjectClass          *object_class;
  GimpToolClass         *tool_class;
  GimpImageMapToolClass *image_map_tool_class;

  object_class         = G_OBJECT_CLASS (klass);
  tool_class           = GIMP_TOOL_CLASS (klass);
  image_map_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize       = gimp_hue_saturation_tool_finalize;

  tool_class->initialize       = gimp_hue_saturation_tool_initialize;

  image_map_tool_class->map    = gimp_hue_saturation_tool_map;
  image_map_tool_class->dialog = gimp_hue_saturation_tool_dialog;
  image_map_tool_class->reset  = gimp_hue_saturation_tool_reset;
}

static void
gimp_hue_saturation_tool_init (GimpHueSaturationTool *hs_tool)
{
  GimpImageMapTool *image_map_tool;

  image_map_tool = GIMP_IMAGE_MAP_TOOL (hs_tool);

  image_map_tool->shell_title = _("Hue-Saturation");
  image_map_tool->shell_name  = "hue_saturation";

  hs_tool->hue_saturation     = g_new0 (HueSaturation, 1);
  hs_tool->hue_partition      = GIMP_ALL_HUES;
}

static void
gimp_hue_saturation_tool_finalize (GObject *object)
{
  GimpHueSaturationTool *hs_tool;

  hs_tool = GIMP_HUE_SATURATION_TOOL (object);

  if (hs_tool->hue_saturation)
    {
      g_free (hs_tool->hue_saturation);
      hs_tool->hue_saturation = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_hue_saturation_tool_initialize (GimpTool    *tool,
				     GimpDisplay *gdisp)
{
  GimpHueSaturationTool *hs_tool;
  gint                   i;

  hs_tool = GIMP_HUE_SATURATION_TOOL (tool);

  if (gdisp &&
      ! gimp_drawable_is_rgb (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Hue-Saturation operates only on RGB color drawables."));
      return;
    }

  for (i = 0; i < 7; i++)
    {
      hs_tool->hue_saturation->hue[i]        = 0.0;
      hs_tool->hue_saturation->lightness[i]  = 0.0;
      hs_tool->hue_saturation->saturation[i] = 0.0;
    }

  hs_tool->hue_partition = GIMP_ALL_HUES;

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);

  hue_saturation_update (hs_tool, ALL);
}

static void
gimp_hue_saturation_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool;

  hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);

  gimp_image_map_apply (image_map_tool->image_map,
                        hue_saturation,
                        hs_tool->hue_saturation);
}


/***************************/
/*  Hue-Saturation dialog  */
/***************************/

static void
gimp_hue_saturation_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool;
  GtkWidget             *main_hbox;
  GtkWidget             *vbox;
  GtkWidget             *table;
  GtkWidget             *label;
  GtkWidget             *slider;
  GtkWidget             *abox;
  GtkWidget             *spinbutton;
  GtkWidget             *radio_button;
  GtkWidget             *frame;
  GtkObject             *data;
  GSList                *group = NULL;
  gint                   i;

  gchar *hue_partition_names[] =
  {
    N_("Master"),
    N_("R"),
    N_("Y"),
    N_("G"),
    N_("C"),
    N_("B"),
    N_("M")
  };

  hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);

  /*  The main hbox containing hue partitions and sliders  */
  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), main_hbox,
                      FALSE, FALSE, 0);

  /*  The table containing hue partitions  */
  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), table, FALSE, FALSE, 0);

  /*  the radio buttons for hue partitions  */
  for (i = 0; i < 7; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, gettext (hue_partition_names[i]));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_button));
      g_object_set_data (G_OBJECT (radio_button), "hue_partition",
                         GINT_TO_POINTER (i));

      if (!i)
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 2, 0, 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	}
      else
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 1, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	  frame = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	  gtk_table_attach (GTK_TABLE (table), frame,  1, 2, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	  gtk_widget_show (frame);

	  hs_tool->hue_partition_da[i - 1] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (hs_tool->hue_partition_da[i - 1]),
			    DA_WIDTH, DA_HEIGHT);
	  gtk_widget_set_events (hs_tool->hue_partition_da[i - 1],
				 HUE_PARTITION_MASK);
	  gtk_container_add (GTK_CONTAINER (frame),
                             hs_tool->hue_partition_da[i - 1]);
	  gtk_widget_show (hs_tool->hue_partition_da[i - 1]);

	  g_signal_connect (G_OBJECT (hs_tool->hue_partition_da[i - 1]), "event",
                            G_CALLBACK (hue_saturation_hue_partition_events),
                            hs_tool);
	}

      g_signal_connect (G_OBJECT (radio_button), "toggled",
                        G_CALLBACK (hue_saturation_partition_callback),
                        hs_tool);

      gtk_widget_show (radio_button);
    }

  gtk_widget_show (table);

  /*  The vbox for the table and preview toggle  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Hue / Lightness / Saturation Adjustments"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  The table containing sliders  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the hue scale widget  */
  label = gtk_label_new (_("Hue:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -180, 180.0, 1.0, 1.0, 0.0);
  hs_tool->hue_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hs_tool->hue_data);
  gtk_widget_set_size_request (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hs_tool->hue_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 74, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (hs_tool->hue_data), "value_changed",
                    G_CALLBACK (hue_saturation_hue_adjustment_update),
                    hs_tool);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Create the lightness scale widget  */
  label = gtk_label_new (_("Lightness:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hs_tool->lightness_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hs_tool->lightness_data);
  gtk_widget_set_size_request (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hs_tool->lightness_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (hs_tool->lightness_data), "value_changed",
                    G_CALLBACK (hue_saturation_lightness_adjustment_update),
                    hs_tool);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Create the saturation scale widget  */
  label = gtk_label_new (_("Saturation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hs_tool->saturation_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hs_tool->saturation_data);
  gtk_widget_set_size_request (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hs_tool->saturation_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (hs_tool->saturation_data), "value_changed",
                    G_CALLBACK (hue_saturation_saturation_adjustment_update),
                    hs_tool);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  gtk_widget_show (table);

  gtk_widget_show (vbox);
  gtk_widget_show (main_hbox);
}

static void
gimp_hue_saturation_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool;

  hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);

  hs_tool->hue_saturation->hue[hs_tool->hue_partition]        = 0.0;
  hs_tool->hue_saturation->lightness[hs_tool->hue_partition]  = 0.0;
  hs_tool->hue_saturation->saturation[hs_tool->hue_partition] = 0.0;

  hue_saturation_update (hs_tool, ALL);
}

static void
hue_saturation_update (GimpHueSaturationTool *hs_tool,
		       gint                   update)
{
  gint   i, j, b;
  gint   rgb[3];
  guchar buf[DA_WIDTH * 3];

  if (update & HUE_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->hue_data),
                              hs_tool->hue_saturation->hue[hs_tool->hue_partition]);

  if (update & LIGHTNESS_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->lightness_data),
                              hs_tool->hue_saturation->lightness[hs_tool->hue_partition]);

  if (update & SATURATION_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->saturation_data),
                              hs_tool->hue_saturation->saturation[hs_tool->hue_partition]);

  hue_saturation_calculate_transfers (hs_tool->hue_saturation);

  for (i = 0; i < 6; i++)
    {
      rgb[RED_PIX]   = default_colors[i][RED_PIX];
      rgb[GREEN_PIX] = default_colors[i][GREEN_PIX];
      rgb[BLUE_PIX]  = default_colors[i][BLUE_PIX];

      gimp_rgb_to_hls_int (rgb, rgb + 1, rgb + 2);

      rgb[RED_PIX]   = hs_tool->hue_saturation->hue_transfer[i][rgb[RED_PIX]];
      rgb[GREEN_PIX] = hs_tool->hue_saturation->lightness_transfer[i][rgb[GREEN_PIX]];
      rgb[BLUE_PIX]  = hs_tool->hue_saturation->saturation_transfer[i][rgb[BLUE_PIX]];

      gimp_hls_to_rgb_int (rgb, rgb + 1, rgb + 2);

      for (j = 0; j < DA_WIDTH; j++)
	for (b = 0; b < 3; b++)
	  buf[j * 3 + b] = (guchar) rgb[b];

      for (j = 0; j < DA_HEIGHT; j++)
	gtk_preview_draw_row (GTK_PREVIEW (hs_tool->hue_partition_da[i]),
			      buf, 0, j, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_queue_draw (hs_tool->hue_partition_da[i]);
    }
}

static void
hue_saturation_partition_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpHueSaturationTool *hs_tool;
  GimpHueRange           partition;

  hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  partition = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "hue_partition"));
  hs_tool->hue_partition = partition;

  hue_saturation_update (hs_tool, ALL);
}

static void
hue_saturation_hue_adjustment_update (GtkAdjustment *adjustment,
				      gpointer       data)
{
  GimpHueSaturationTool *hs_tool;
  GimpHueRange           part;

  hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  part = hs_tool->hue_partition;

  if (hs_tool->hue_saturation->hue[part] != adjustment->value)
    {
      hs_tool->hue_saturation->hue[part] = adjustment->value;
      hue_saturation_update (hs_tool, DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
    }
}

static void
hue_saturation_lightness_adjustment_update (GtkAdjustment *adjustment,
					    gpointer       data)
{
  GimpHueSaturationTool *hs_tool;
  GimpHueRange           part;

  hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  part = hs_tool->hue_partition;

  if (hs_tool->hue_saturation->lightness[part] != adjustment->value)
    {
      hs_tool->hue_saturation->lightness[part] = adjustment->value;
      hue_saturation_update (hs_tool, DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
    }
}

static void
hue_saturation_saturation_adjustment_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  GimpHueSaturationTool *hs_tool;
  GimpHueRange           part;

  hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  part = hs_tool->hue_partition;

  if (hs_tool->hue_saturation->saturation[part] != adjustment->value)
    {
      hs_tool->hue_saturation->saturation[part] = adjustment->value;
      hue_saturation_update (hs_tool, DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
    }
}

static gint
hue_saturation_hue_partition_events (GtkWidget             *widget,
				     GdkEvent              *event,
				     GimpHueSaturationTool *hs_tool)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      hue_saturation_update (hs_tool, HUE_PARTITION);
      break;

    default:
      break;
    }

  return FALSE;
}
