/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/hue-saturation.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimphuesaturationtool.h"
#include "gimpimagemapoptions.h"

#include "gimp-intl.h"


#define SLIDER_WIDTH  200
#define DA_WIDTH       40
#define DA_HEIGHT      20

#define HUE_SLIDER         (1 << 0)
#define LIGHTNESS_SLIDER   (1 << 1)
#define SATURATION_SLIDER  (1 << 2)
#define DRAW               (1 << 3)
#define OVERLAP_SLIDER     (1 << 4)
#define SLIDERS            (HUE_SLIDER        | \
                            LIGHTNESS_SLIDER  | \
                            SATURATION_SLIDER | \
                            OVERLAP_SLIDER)
#define ALL                (SLIDERS | DRAW)


/*  local function prototypes  */

static void     gimp_hue_saturation_tool_finalize   (GObject          *object);

static gboolean gimp_hue_saturation_tool_initialize (GimpTool         *tool,
                                                     GimpDisplay      *display,
                                                     GError          **error);

static void     gimp_hue_saturation_tool_map        (GimpImageMapTool *im_tool);
static void     gimp_hue_saturation_tool_dialog     (GimpImageMapTool *im_tool);
static void     gimp_hue_saturation_tool_reset      (GimpImageMapTool *im_tool);

static void     hue_saturation_update               (GimpHueSaturationTool *hs_tool,
                                                     gint                   update);
static void     hue_saturation_partition_callback           (GtkWidget     *widget,
                                                             gpointer       data);
static void     hue_saturation_partition_reset_callback     (GtkWidget     *widget,
                                                             gpointer       data);
static void     hue_saturation_hue_adjustment_update        (GtkAdjustment *adj,
                                                             gpointer       data);
static void     hue_saturation_lightness_adjustment_update  (GtkAdjustment *adj,
                                                             gpointer       data);
static void     hue_saturation_saturation_adjustment_update (GtkAdjustment *adj,
                                                             gpointer       data);
static void     hue_saturation_overlap_adjustment_update    (GtkAdjustment *adj,
                                                             gpointer       data);


G_DEFINE_TYPE (GimpHueSaturationTool, gimp_hue_saturation_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_hue_saturation_tool_parent_class

static gint default_colors[6][3] =
{
  { 255,   0,   0 },
  { 255, 255,   0 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  {   0,   0, 255 },
  { 255,   0, 255 }
};


void
gimp_hue_saturation_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_HUE_SATURATION_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-hue-saturation-tool",
                _("Hue-Saturation"),
                _("Hue-Saturation Tool: Adjust hue, saturation, and lightness"),
                N_("Hue-_Saturation..."), NULL,
                NULL, GIMP_HELP_TOOL_HUE_SATURATION,
                GIMP_STOCK_TOOL_HUE_SATURATION,
                data);
}

static void
gimp_hue_saturation_tool_class_init (GimpHueSaturationToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize    = gimp_hue_saturation_tool_finalize;

  tool_class->initialize    = gimp_hue_saturation_tool_initialize;

  im_tool_class->shell_desc = _("Adjust Hue / Lightness / Saturation");

  im_tool_class->map        = gimp_hue_saturation_tool_map;
  im_tool_class->dialog     = gimp_hue_saturation_tool_dialog;
  im_tool_class->reset      = gimp_hue_saturation_tool_reset;
}

static void
gimp_hue_saturation_tool_init (GimpHueSaturationTool *hs_tool)
{
  hs_tool->hue_saturation = g_slice_new0 (HueSaturation);
  hs_tool->hue_partition  = GIMP_ALL_HUES;

  hue_saturation_init (hs_tool->hue_saturation);
}

static void
gimp_hue_saturation_tool_finalize (GObject *object)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (object);

  g_slice_free (HueSaturation, hs_tool->hue_saturation);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_hue_saturation_tool_initialize (GimpTool     *tool,
                                     GimpDisplay  *display,
                                     GError      **error)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (tool);
  GimpDrawable          *drawable;

  drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Hue-Saturation operates only on RGB color layers."));
      return FALSE;
    }

  hue_saturation_init (hs_tool->hue_saturation);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  hue_saturation_update (hs_tool, ALL);

  return TRUE;
}

static void
gimp_hue_saturation_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);

  gimp_image_map_apply (image_map_tool->image_map,
                        (GimpImageMapApplyFunc) hue_saturation,
                        hs_tool->hue_saturation);
}


/***************************/
/*  Hue-Saturation dialog  */
/***************************/

static void
gimp_hue_saturation_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);
  GtkWidget             *vbox;
  GtkWidget             *abox;
  GtkWidget             *table;
  GtkWidget             *slider;
  GtkWidget             *button;
  GtkWidget             *frame;
  GtkWidget             *hbox;
  GtkObject             *data;
  GtkSizeGroup          *label_group;
  GtkSizeGroup          *spinner_group;
  GSList                *group = NULL;
  gint                   i;

  const struct
  {
    const gchar *label;
    const gchar *tooltip;
    gint         label_col;
    gint         label_row;
    gint         frame_col;
    gint         frame_row;
  }
  hue_partition_table[] =
  {
    { N_("M_aster"), N_("Adjust all colors"), 2, 3, 0, 0 },
    { N_("_R"),      N_("Red"),               2, 1, 2, 0 },
    { N_("_Y"),      N_("Yellow"),            1, 2, 0, 2 },
    { N_("_G"),      N_("Green"),             1, 4, 0, 4 },
    { N_("_C"),      N_("Cyan"),              2, 5, 2, 6 },
    { N_("_B"),      N_("Blue"),              3, 4, 4, 4 },
    { N_("_M"),      N_("Magenta"),           3, 2, 4, 2 }
  };

  frame = gimp_frame_new (_("Select Primary Color to Adjust"));
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), frame,
                      TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  /*  The table containing hue partitions  */
  table = gtk_table_new (7, 5, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 3, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 2);
  gtk_container_add (GTK_CONTAINER (abox), table);

  /*  the radio buttons for hue partitions  */
  for (i = 0; i < G_N_ELEMENTS (hue_partition_table); i++)
    {
      button = gtk_radio_button_new_with_mnemonic (group,
                                                   gettext (hue_partition_table[i].label));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_object_set_data (G_OBJECT (button), "hue_partition",
                         GINT_TO_POINTER (i));

      gimp_help_set_help_data (button,
                               gettext (hue_partition_table[i].tooltip),
                               NULL);

      if (i == 0)
        gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      gtk_table_attach (GTK_TABLE (table), button,
                        hue_partition_table[i].label_col,
                        hue_partition_table[i].label_col + 1,
                        hue_partition_table[i].label_row,
                        hue_partition_table[i].label_row + 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

      if (i > 0)
        {
          GimpRGB color = { 0.0 };

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_table_attach (GTK_TABLE (table), frame,
                            hue_partition_table[i].frame_col,
                            hue_partition_table[i].frame_col + 1,
                            hue_partition_table[i].frame_row,
                            hue_partition_table[i].frame_row + 1,
                            GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (frame);

          hs_tool->hue_partition_da[i - 1] = gimp_color_area_new (&color,
                                                                  GIMP_COLOR_AREA_FLAT,
                                                                  0);
          gtk_widget_set_size_request (hs_tool->hue_partition_da[i - 1],
                                       DA_WIDTH, DA_HEIGHT);
          gtk_container_add (GTK_CONTAINER (frame),
                             hs_tool->hue_partition_da[i - 1]);
          gtk_widget_show (hs_tool->hue_partition_da[i - 1]);
        }

      g_signal_connect (button, "toggled",
                        G_CALLBACK (hue_saturation_partition_callback),
                        hs_tool);

      gtk_widget_show (button);
    }

  gtk_widget_show (table);

  label_group  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  spinner_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* Create the 'Overlap' option slider */
  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                               _("_Overlap:"), SLIDER_WIDTH, -1,
                               0.0, 0, 100.0, 1.0, 15.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  hs_tool->overlap_data = GTK_ADJUSTMENT (data);

  gtk_size_group_add_widget (label_group, GIMP_SCALE_ENTRY_LABEL (data));
  gtk_size_group_add_widget (spinner_group, GIMP_SCALE_ENTRY_SPINBUTTON (data));
  g_object_unref (label_group);
  g_object_unref (spinner_group);

  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (hue_saturation_overlap_adjustment_update),
                    hs_tool);

  frame = gimp_frame_new (_("Adjust Selected Color"));
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  The table containing sliders  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Create the hue scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                               _("_Hue:"), SLIDER_WIDTH, -1,
                               0.0, -180.0, 180.0, 1.0, 15.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  hs_tool->hue_data = GTK_ADJUSTMENT (data);

  gtk_size_group_add_widget (label_group, GIMP_SCALE_ENTRY_LABEL (data));
  gtk_size_group_add_widget (spinner_group, GIMP_SCALE_ENTRY_SPINBUTTON (data));

  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (hue_saturation_hue_adjustment_update),
                    hs_tool);

  /*  Create the lightness scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                               _("_Lightness:"), SLIDER_WIDTH, -1,
                               0.0, -100.0, 100.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  hs_tool->lightness_data = GTK_ADJUSTMENT (data);

  gtk_size_group_add_widget (label_group, GIMP_SCALE_ENTRY_LABEL (data));
  gtk_size_group_add_widget (spinner_group, GIMP_SCALE_ENTRY_SPINBUTTON (data));

  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (hue_saturation_lightness_adjustment_update),
                    hs_tool);

  /*  Create the saturation scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                               _("_Saturation:"), SLIDER_WIDTH, -1,
                               0.0, -100.0, 100.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  hs_tool->saturation_data = GTK_ADJUSTMENT (data);

  gtk_size_group_add_widget (label_group, GIMP_SCALE_ENTRY_LABEL (data));
  gtk_size_group_add_widget (spinner_group, GIMP_SCALE_ENTRY_SPINBUTTON (data));

  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (hs_tool->saturation_data, "value-changed",
                    G_CALLBACK (hue_saturation_saturation_adjustment_update),
                    hs_tool);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Color"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (hue_saturation_partition_reset_callback),
                    hs_tool);
}

static void
gimp_hue_saturation_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (image_map_tool);

  hue_saturation_init (hs_tool->hue_saturation);
  hue_saturation_update (hs_tool, ALL);
}

static void
hue_saturation_update (GimpHueSaturationTool *hs_tool,
                       gint                   update)
{
  gint    rgb[3];
  GimpRGB color;
  gint    i;

  hue_saturation_calculate_transfers (hs_tool->hue_saturation);

  if (update & HUE_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->hue_data),
                              hs_tool->hue_saturation->hue[hs_tool->hue_partition]);

  if (update & LIGHTNESS_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->lightness_data),
                              hs_tool->hue_saturation->lightness[hs_tool->hue_partition]);

  if (update & SATURATION_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->saturation_data),
                              hs_tool->hue_saturation->saturation[hs_tool->hue_partition]);

  if (update & OVERLAP_SLIDER)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (hs_tool->overlap_data),
                              hs_tool->hue_saturation->overlap);

  if (update & DRAW)
    {
      for (i = 0; i < 6; i++)
        {
          rgb[RED_PIX]   = default_colors[i][RED_PIX];
          rgb[GREEN_PIX] = default_colors[i][GREEN_PIX];
          rgb[BLUE_PIX]  = default_colors[i][BLUE_PIX];

          gimp_rgb_to_hsl_int (rgb, rgb + 1, rgb + 2);

          rgb[RED_PIX]   = hs_tool->hue_saturation->hue_transfer[i][rgb[RED_PIX]];
          rgb[GREEN_PIX] = hs_tool->hue_saturation->saturation_transfer[i][rgb[GREEN_PIX]];
          rgb[BLUE_PIX]  = hs_tool->hue_saturation->lightness_transfer[i][rgb[BLUE_PIX]];

          gimp_hsl_to_rgb_int (rgb, rgb + 1, rgb + 2);

          gimp_rgb_set_uchar (&color,
                              (guchar) rgb[0], (guchar) rgb[1], (guchar) rgb[2]);

          gimp_color_area_set_color (GIMP_COLOR_AREA (hs_tool->hue_partition_da[i]),
                                     &color);
        }
    }
}

static void
hue_saturation_partition_callback (GtkWidget *widget,
                                   gpointer   data)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);
  GimpHueRange           partition;

  partition = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "hue_partition"));

  if (hs_tool->hue_partition != partition)
    {
      hs_tool->hue_partition = partition;
      hue_saturation_update (hs_tool, SLIDERS);
    }
}

static void
hue_saturation_partition_reset_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  hue_saturation_partition_reset (hs_tool->hue_saturation,
                                  hs_tool->hue_partition);
  hue_saturation_update (hs_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
}

static void
hue_saturation_hue_adjustment_update (GtkAdjustment *adjustment,
                                      gpointer       data)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);
  GimpHueRange           part    = hs_tool->hue_partition;

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
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);
  GimpHueRange           part    = hs_tool->hue_partition;

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
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);
  GimpHueRange           part    = hs_tool->hue_partition;

  if (hs_tool->hue_saturation->saturation[part] != adjustment->value)
    {
      hs_tool->hue_saturation->saturation[part] = adjustment->value;
      hue_saturation_update (hs_tool, DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
    }
}

static void
hue_saturation_overlap_adjustment_update (GtkAdjustment *adjustment,
                                          gpointer       data)
{
  GimpHueSaturationTool *hs_tool = GIMP_HUE_SATURATION_TOOL (data);

  if (hs_tool->hue_saturation->overlap != adjustment->value)
    {
      hs_tool->hue_saturation->overlap = adjustment->value;
      hue_saturation_update (hs_tool, DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (hs_tool));
    }
}
