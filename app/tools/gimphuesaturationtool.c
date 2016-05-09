/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimphuesaturationconfig.h"
#include "operations/gimpoperationhuesaturation.h"

#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "display/gimpdisplay.h"

#include "gimpfilteroptions.h"
#include "gimphuesaturationtool.h"

#include "gimp-intl.h"


#define DA_WIDTH  40
#define DA_HEIGHT 20


/*  local function prototypes  */

static void       gimp_hue_saturation_tool_constructed   (GObject         *object);

static gboolean   gimp_hue_saturation_tool_initialize    (GimpTool        *tool,
                                                          GimpDisplay     *display,
                                                          GError         **error);

static gchar    * gimp_hue_saturation_tool_get_operation (GimpFilterTool  *filter_tool,
                                                          gchar          **title,
                                                          gchar          **description,
                                                          gchar          **undo_desc,
                                                          gchar          **icon_name,
                                                          gchar          **help_id);
static void       gimp_hue_saturation_tool_dialog        (GimpFilterTool  *filter_tool);
static void       gimp_hue_saturation_tool_reset         (GimpFilterTool  *filter_tool);

static void       hue_saturation_config_notify           (GObject               *object,
                                                          GParamSpec            *pspec,
                                                          GimpHueSaturationTool *hs_tool);

static void       hue_saturation_update_color_areas      (GimpHueSaturationTool *hs_tool);

static void       hue_saturation_range_callback          (GtkWidget             *widget,
                                                          GimpFilterTool        *filter_tool);
static void       hue_saturation_range_reset_callback    (GtkWidget             *widget,
                                                          GimpFilterTool        *hs_tool);


G_DEFINE_TYPE (GimpHueSaturationTool, gimp_hue_saturation_tool,
               GIMP_TYPE_FILTER_TOOL)

#define parent_class gimp_hue_saturation_tool_parent_class


void
gimp_hue_saturation_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_HUE_SATURATION_TOOL,
                GIMP_TYPE_FILTER_OPTIONS, NULL,
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
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  object_class->constructed              = gimp_hue_saturation_tool_constructed;

  tool_class->initialize                 = gimp_hue_saturation_tool_initialize;

  filter_tool_class->settings_name       = "hue-saturation";
  filter_tool_class->import_dialog_title = _("Import Hue-Saturation Settings");
  filter_tool_class->export_dialog_title = _("Export Hue-Saturation Settings");

  filter_tool_class->get_operation       = gimp_hue_saturation_tool_get_operation;
  filter_tool_class->dialog              = gimp_hue_saturation_tool_dialog;
  filter_tool_class->reset               = gimp_hue_saturation_tool_reset;
}

static void
gimp_hue_saturation_tool_init (GimpHueSaturationTool *hs_tool)
{
}

static void
gimp_hue_saturation_tool_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (GIMP_FILTER_TOOL (object)->config, "notify",
                           G_CALLBACK (hue_saturation_config_notify),
                           object, 0);
}

static gboolean
gimp_hue_saturation_tool_initialize (GimpTool     *tool,
                                     GimpDisplay  *display,
                                     GError      **error)
{
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Hue-Saturation operates only on RGB color layers."));
      return FALSE;
    }

  return GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);
}

static gchar *
gimp_hue_saturation_tool_get_operation (GimpFilterTool  *filter_tool,
                                        gchar          **title,
                                        gchar          **description,
                                        gchar          **undo_desc,
                                        gchar          **icon_name,
                                        gchar          **help_id)
{
  *description = g_strdup (_("Adjust Hue / Lightness / Saturation"));

  return g_strdup ("gimp:hue-saturation");
}


/***************************/
/*  Hue-Saturation dialog  */
/***************************/

static void
gimp_hue_saturation_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpHueSaturationTool   *hs_tool = GIMP_HUE_SATURATION_TOOL (filter_tool);
  GimpHueSaturationConfig *config  = GIMP_HUE_SATURATION_CONFIG (filter_tool->config);
  GtkWidget               *main_vbox;
  GtkWidget               *frame;
  GtkWidget               *vbox;
  GtkWidget               *abox;
  GtkWidget               *table;
  GtkWidget               *scale;
  GtkWidget               *button;
  GtkWidget               *hbox;
  GSList                  *group = NULL;
  gint                     i;

  const struct
  {
    const gchar *label;
    const gchar *tooltip;
    gint         label_col;
    gint         label_row;
    gint         frame_col;
    gint         frame_row;
  }
  hue_range_table[] =
  {
    { N_("M_aster"), N_("Adjust all colors"), 2, 3, 0, 0 },
    { N_("_R"),      N_("Red"),               2, 1, 2, 0 },
    { N_("_Y"),      N_("Yellow"),            1, 2, 0, 2 },
    { N_("_G"),      N_("Green"),             1, 4, 0, 4 },
    { N_("_C"),      N_("Cyan"),              2, 5, 2, 6 },
    { N_("_B"),      N_("Blue"),              3, 4, 4, 4 },
    { N_("_M"),      N_("Magenta"),           3, 2, 4, 2 }
  };

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  frame = gimp_frame_new (_("Select Primary Color to Adjust"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  /*  The table containing hue ranges  */
  table = gtk_table_new (7, 5, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 3, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 2);
  gtk_container_add (GTK_CONTAINER (abox), table);

  /*  the radio buttons for hue ranges  */
  for (i = 0; i < G_N_ELEMENTS (hue_range_table); i++)
    {
      button = gtk_radio_button_new_with_mnemonic (group,
                                                   gettext (hue_range_table[i].label));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_object_set_data (G_OBJECT (button), "gimp-item-data",
                         GINT_TO_POINTER (i));

      gimp_help_set_help_data (button,
                               gettext (hue_range_table[i].tooltip),
                               NULL);

      if (i == 0)
        {
          gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

          hs_tool->range_radio = button;
        }

      gtk_table_attach (GTK_TABLE (table), button,
                        hue_range_table[i].label_col,
                        hue_range_table[i].label_col + 1,
                        hue_range_table[i].label_row,
                        hue_range_table[i].label_row + 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

      if (i > 0)
        {
          GimpRGB color = { 0.0 };

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_table_attach (GTK_TABLE (table), frame,
                            hue_range_table[i].frame_col,
                            hue_range_table[i].frame_col + 1,
                            hue_range_table[i].frame_row,
                            hue_range_table[i].frame_row + 1,
                            GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (frame);

          hs_tool->hue_range_color_area[i - 1] =
            gimp_color_area_new (&color, GIMP_COLOR_AREA_FLAT, 0);
          gtk_widget_set_size_request (hs_tool->hue_range_color_area[i - 1],
                                       DA_WIDTH, DA_HEIGHT);
          gtk_container_add (GTK_CONTAINER (frame),
                             hs_tool->hue_range_color_area[i - 1]);
          gtk_widget_show (hs_tool->hue_range_color_area[i - 1]);
        }

      g_signal_connect (button, "toggled",
                        G_CALLBACK (hue_saturation_range_callback),
                        hs_tool);

      gtk_widget_show (button);
    }

  gtk_widget_show (table);

  /* Create the 'Overlap' option slider */
  scale = gimp_prop_spin_scale_new (filter_tool->config, "overlap",
                                    _("_Overlap"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  frame = gimp_frame_new (_("Adjust Selected Color"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  Create the hue scale widget  */
  scale = gimp_prop_spin_scale_new (filter_tool->config, "hue",
                                    _("_Hue"), 1.0 / 180.0, 15.0 / 180.0, 0);
  gimp_prop_widget_set_factor (scale, 180.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  Create the lightness scale widget  */
  scale = gimp_prop_spin_scale_new (filter_tool->config, "lightness",
                                    _("_Lightness"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  Create the saturation scale widget  */
  scale = gimp_prop_spin_scale_new (filter_tool->config, "saturation",
                                    _("_Saturation"), 0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Color"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (hue_saturation_range_reset_callback),
                    hs_tool);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (hs_tool->range_radio),
                                   config->range);

  hue_saturation_update_color_areas (hs_tool);
}

static void
gimp_hue_saturation_tool_reset (GimpFilterTool *filter_tool)
{
  GimpHueRange range;

  g_object_freeze_notify (filter_tool->config);

  g_object_get (filter_tool->config,
                "range", &range,
                NULL);

  if (filter_tool->default_config)
    {
      gimp_config_copy (GIMP_CONFIG (filter_tool->default_config),
                        GIMP_CONFIG (filter_tool->config),
                        0);
    }
  else
    {
      gimp_config_reset (GIMP_CONFIG (filter_tool->config));
    }

  g_object_set (filter_tool->config,
                "range", range,
                NULL);

  g_object_thaw_notify (filter_tool->config);
}

static void
hue_saturation_config_notify (GObject               *object,
                              GParamSpec            *pspec,
                              GimpHueSaturationTool *hs_tool)
{
  GimpHueSaturationConfig *config = GIMP_HUE_SATURATION_CONFIG (object);

  if (! hs_tool->range_radio)
    return;

  if (! strcmp (pspec->name, "range"))
    {
      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (hs_tool->range_radio),
                                       config->range);
    }

  hue_saturation_update_color_areas (hs_tool);
}

static void
hue_saturation_update_color_areas (GimpHueSaturationTool *hs_tool)
{
  static GimpRGB default_colors[6] =
  {
    { 1.0,   0,   0, },
    { 1.0, 1.0,   0, },
    {   0, 1.0,   0, },
    {   0, 1.0, 1.0, },
    {   0,   0, 1.0, },
    { 1.0,   0, 1.0, }
  };

  GimpFilterTool          *filter_tool = GIMP_FILTER_TOOL (hs_tool);
  GimpHueSaturationConfig *config      = GIMP_HUE_SATURATION_CONFIG (filter_tool->config);
  gint                     i;

  for (i = 0; i < 6; i++)
    {
      GimpRGB color = default_colors[i];

      gimp_operation_hue_saturation_map (config, &color, i + 1,
                                         &color);

      gimp_color_area_set_color (GIMP_COLOR_AREA (hs_tool->hue_range_color_area[i]),
                                 &color);
    }
}

static void
hue_saturation_range_callback (GtkWidget      *widget,
                               GimpFilterTool *filter_tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpHueRange range;

      gimp_radio_button_update (widget, &range);
      g_object_set (filter_tool->config,
                    "range", range,
                    NULL);
    }
}

static void
hue_saturation_range_reset_callback (GtkWidget      *widget,
                                     GimpFilterTool *filter_tool)
{
  gimp_hue_saturation_config_reset_range (GIMP_HUE_SATURATION_CONFIG (filter_tool->config));
}
