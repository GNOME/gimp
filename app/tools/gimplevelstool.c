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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/levels.h"

#include "gegl/gimplevelsconfig.h"
#include "gegl/gimpoperationlevels.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimphandlebar.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimphistogramoptions.h"
#include "gimplevelstool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define PICK_LOW_INPUT    (1 << 0)
#define PICK_GAMMA        (1 << 1)
#define PICK_HIGH_INPUT   (1 << 2)
#define PICK_ALL_CHANNELS (1 << 8)

#define HISTOGRAM_WIDTH    256
#define GRADIENT_HEIGHT    12
#define CONTROL_HEIGHT     10


/*  local function prototypes  */

static void       gimp_levels_tool_finalize       (GObject           *object);

static gboolean   gimp_levels_tool_initialize     (GimpTool          *tool,
                                                   GimpDisplay       *display,
                                                   GError           **error);

static void       gimp_levels_tool_color_picked   (GimpColorTool     *color_tool,
                                                   GimpColorPickState pick_state,
                                                   GimpImageType      sample_type,
                                                   GimpRGB           *color,
                                                   gint               color_index);

static GeglNode * gimp_levels_tool_get_operation  (GimpImageMapTool  *im_tool);
static void       gimp_levels_tool_map            (GimpImageMapTool  *im_tool);
static void       gimp_levels_tool_dialog         (GimpImageMapTool  *im_tool);
static void       gimp_levels_tool_dialog_unmap   (GtkWidget         *dialog,
                                                   GimpLevelsTool    *tool);
static void       gimp_levels_tool_reset          (GimpImageMapTool  *im_tool);
static gboolean   gimp_levels_tool_settings_load  (GimpImageMapTool  *im_tool,
                                                   gpointer           fp,
                                                   GError           **error);
static gboolean   gimp_levels_tool_settings_save  (GimpImageMapTool  *im_tool,
                                                   gpointer           fp);

static void       levels_update_adjustments       (GimpLevelsTool    *tool);
static void       levels_update_input_bar         (GimpLevelsTool    *tool);

static void       levels_channel_callback         (GtkWidget         *widget,
                                                   GimpLevelsTool    *tool);
static void       levels_channel_reset_callback   (GtkWidget         *widget,
                                                   GimpLevelsTool    *tool);

static gboolean   levels_menu_sensitivity         (gint               value,
                                                   gpointer           data);

static void       levels_stretch_callback         (GtkWidget         *widget,
                                                   GimpLevelsTool    *tool);
static void       levels_low_input_changed        (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_gamma_changed            (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_linear_gamma_changed     (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_high_input_changed       (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_low_output_changed       (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_high_output_changed      (GtkAdjustment     *adjustment,
                                                   GimpLevelsTool    *tool);
static void       levels_input_picker_toggled     (GtkWidget         *widget,
                                                   GimpLevelsTool    *tool);


G_DEFINE_TYPE (GimpLevelsTool, gimp_levels_tool, GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_levels_tool_parent_class


void
gimp_levels_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_LEVELS_TOOL,
                GIMP_TYPE_HISTOGRAM_OPTIONS,
                gimp_color_options_gui,
                0,
                "gimp-levels-tool",
                _("Levels"),
                _("Levels Tool: Adjust color levels"),
                N_("_Levels..."), NULL,
                NULL, GIMP_HELP_TOOL_LEVELS,
                GIMP_STOCK_TOOL_LEVELS,
                data);
}

static void
gimp_levels_tool_class_init (GimpLevelsToolClass *klass)
{
  GObjectClass          *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass    *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class    = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize           = gimp_levels_tool_finalize;

  tool_class->initialize           = gimp_levels_tool_initialize;

  color_tool_class->picked         = gimp_levels_tool_color_picked;

  im_tool_class->shell_desc        = _("Adjust Color Levels");
  im_tool_class->settings_name     = "levels";
  im_tool_class->load_dialog_title = _("Load Levels");
  im_tool_class->load_button_tip   = _("Load levels settings from file");
  im_tool_class->save_dialog_title = _("Save Levels");
  im_tool_class->save_button_tip   = _("Save levels settings to file");

  im_tool_class->get_operation     = gimp_levels_tool_get_operation;
  im_tool_class->map               = gimp_levels_tool_map;
  im_tool_class->dialog            = gimp_levels_tool_dialog;
  im_tool_class->reset             = gimp_levels_tool_reset;
  im_tool_class->settings_load     = gimp_levels_tool_settings_load;
  im_tool_class->settings_save     = gimp_levels_tool_settings_save;
}

static void
gimp_levels_tool_init (GimpLevelsTool *tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  tool->lut           = gimp_lut_new ();
  tool->levels        = g_slice_new0 (Levels);
  tool->hist          = NULL;
  tool->channel       = GIMP_HISTOGRAM_VALUE;
  tool->active_picker = NULL;

  levels_init (tool->levels);

  im_tool->apply_func = (GimpImageMapApplyFunc) gimp_lut_process;
  im_tool->apply_data = tool->lut;
}

static void
gimp_levels_tool_finalize (GObject *object)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (object);

  gimp_lut_free (tool->lut);
  g_slice_free (Levels, tool->levels);

  if (tool->config)
    {
      g_object_unref (tool->config);
      tool->config = NULL;
    }

  if (tool->hist)
    {
      gimp_histogram_free (tool->hist);
      tool->hist = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_levels_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpLevelsTool *l_tool   = GIMP_LEVELS_TOOL (tool);
  GimpDrawable   *drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Levels does not operate on indexed layers."));
      return FALSE;
    }

  if (! l_tool->hist)
    l_tool->hist = gimp_histogram_new ();

  gimp_levels_config_reset (l_tool->config);

  levels_init (l_tool->levels);

  l_tool->channel = GIMP_HISTOGRAM_VALUE;
  l_tool->color   = gimp_drawable_is_rgb (drawable);
  l_tool->alpha   = gimp_drawable_has_alpha (drawable);

  if (l_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (l_tool->active_picker),
                                  FALSE);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (l_tool->channel_menu),
                                      levels_menu_sensitivity, l_tool, NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (l_tool->channel_menu),
                                 l_tool->channel);

  /* FIXME: hack */
  if (! l_tool->color)
    l_tool->channel = (l_tool->channel == GIMP_HISTOGRAM_ALPHA) ? 1 : 0;

  levels_update_adjustments (l_tool);

  gimp_drawable_calculate_histogram (drawable, l_tool->hist);
  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (l_tool->hist_view),
                                     l_tool->hist);

  return TRUE;
}

static GeglNode *
gimp_levels_tool_get_operation (GimpImageMapTool *im_tool)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (im_tool);
  GeglNode       *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp-levels",
                       NULL);

  tool->config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  gegl_node_set (node,
                 "config", tool->config,
                 NULL);

  return node;
}

static void
gimp_levels_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool       *tool = GIMP_LEVELS_TOOL (image_map_tool);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      /* FIXME: hack */
      if (! tool->color && channel == 1)
        g_object_set (tool->config,
                      "channel", GIMP_HISTOGRAM_ALPHA,
                      NULL);
      else
        g_object_set (tool->config,
                      "channel", channel,
                      NULL);

      g_object_set (tool->config,
                    "gamma",       tool->levels->gamma[channel],
                    "low-input",   tool->levels->low_input[channel]   / 255.0,
                    "high-input",  tool->levels->high_input[channel]  / 255.0,
                    "low-output",  tool->levels->low_output[channel]  / 255.0,
                    "high-output", tool->levels->high_output[channel] / 255.0,
                    NULL);

      /* FIXME: hack */
      if (! tool->color && channel == 1)
        break;
    }

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) levels_lut_func,
                  tool->levels,
                  gimp_drawable_bytes (image_map_tool->drawable));
}


/*******************/
/*  Levels dialog  */
/*******************/

static GtkWidget *
gimp_levels_tool_color_picker_new (GimpLevelsTool *tool,
                                   guint           value)
{
  GtkWidget   *button;
  GtkWidget   *image;
  const gchar *stock_id;
  const gchar *help;

  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      stock_id = GIMP_STOCK_COLOR_PICKER_BLACK;
      help     = _("Pick black point");
      break;
    case PICK_GAMMA:
      stock_id = GIMP_STOCK_COLOR_PICKER_GRAY;
      help     = _("Pick gray point");
      break;
    case PICK_HIGH_INPUT:
      stock_id = GIMP_STOCK_COLOR_PICKER_WHITE;
      help     = _("Pick white point");
      break;
    default:
      return NULL;
    }

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "draw-indicator", FALSE,
                         NULL);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_misc_set_padding (GTK_MISC (image), 2, 2);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (button, help, NULL);

  g_object_set_data (G_OBJECT (button),
                     "pick-value", GUINT_TO_POINTER (value));
  g_signal_connect (button, "toggled",
                    G_CALLBACK (levels_input_picker_toggled),
                    tool);

  return button;
}

static void
gimp_levels_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool  *tool         = GIMP_LEVELS_TOOL (image_map_tool);
  GimpToolOptions *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GtkListStore    *store;
  GtkWidget       *vbox;
  GtkWidget       *vbox2;
  GtkWidget       *vbox3;
  GtkWidget       *hbox;
  GtkWidget       *hbox2;
  GtkWidget       *label;
  GtkWidget       *menu;
  GtkWidget       *frame;
  GtkWidget       *hbbox;
  GtkWidget       *button;
  GtkWidget       *spinbutton;
  GtkWidget       *bar;
  GtkObject       *data;
  gint             border;

  /*  The option menu for selecting channels  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gimp_enum_store_new_with_range (GIMP_TYPE_HISTOGRAM_CHANNEL,
                                          GIMP_HISTOGRAM_VALUE,
                                          GIMP_HISTOGRAM_ALPHA);
  menu = gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));
  g_object_unref (store);

  g_signal_connect (menu, "changed",
                    G_CALLBACK (levels_channel_callback),
                    tool);
  gimp_enum_combo_box_set_stock_prefix (GIMP_ENUM_COMBO_BOX (menu),
                                        "gimp-channel");
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  tool->channel_menu = menu;

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_channel_reset_callback),
                    tool);

  menu = gimp_prop_enum_stock_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  /*  Input levels frame  */
  frame = gimp_frame_new (_("Input Levels"));
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), frame,
                      TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  tool->hist_view = gimp_histogram_view_new (FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->hist_view, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (tool->hist_view));

  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       GIMP_HISTOGRAM_VIEW (tool->hist_view));

  g_object_get (tool->hist_view, "border-width", &border, NULL);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), border);
  gtk_box_pack_start (GTK_BOX (vbox2), vbox3, FALSE, FALSE, 0);
  gtk_widget_show (vbox3);

  tool->input_bar = g_object_new (GIMP_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->input_bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), tool->input_bar, FALSE, FALSE, 0);
  gtk_widget_show (tool->input_bar);

  bar = g_object_new (GIMP_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), bar, FALSE, FALSE, 0);
  gtk_widget_show (bar);

  tool->input_sliders = g_object_new (GIMP_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (tool->input_sliders, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox3), tool->input_sliders, FALSE, FALSE, 0);
  gtk_widget_show (tool->input_sliders);

  g_signal_connect_swapped (tool->input_bar, "button-press-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->button_press_event),
                            tool->input_sliders);

  g_signal_connect_swapped (tool->input_bar, "button-release-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->button_release_event),
                            tool->input_sliders);

  g_signal_connect_swapped (tool->input_bar, "motion-notify-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->motion_notify_event),
                            tool->input_sliders);

  g_signal_connect_swapped (bar, "button-press-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->button_press_event),
                            tool->input_sliders);

  g_signal_connect_swapped (bar, "button-release-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->button_release_event),
                            tool->input_sliders);

  g_signal_connect_swapped (bar, "motion-notify-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->input_sliders)->motion_notify_event),
                            tool->input_sliders);

  /*  Horizontal box for input levels spinbuttons  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low input spin  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (tool, PICK_LOW_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = gimp_spin_button_new (&data, 0, 0, 255, 1, 10, 10, 0.5, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->low_input = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->low_input, "value-changed",
                    G_CALLBACK (levels_low_input_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->input_sliders), 0,
                                  tool->low_input);

  /*  input gamma spin  */
  spinbutton = gimp_spin_button_new (&data, 1, 0.1, 10, 0.01, 0.1, 1, 0.5, 2);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, FALSE, 0);
  gimp_help_set_help_data (spinbutton, _("Gamma"), NULL);
  gtk_widget_show (spinbutton);

  tool->gamma = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->gamma, "value-changed",
                    G_CALLBACK (levels_gamma_changed),
                    tool);

  tool->gamma_linear = GTK_ADJUSTMENT (gtk_adjustment_new (127, 0, 255,
                                                           0.1, 1.0, 0.0));
  g_signal_connect (tool->gamma_linear, "value-changed",
                    G_CALLBACK (levels_linear_gamma_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->input_sliders), 1,
                                  tool->gamma_linear);
  g_object_unref (tool->gamma_linear);

  /*  high input spin  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (tool, PICK_HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = gimp_spin_button_new (&data, 255, 0, 255, 1, 10, 10, 0.5, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->high_input = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->high_input, "value-changed",
                    G_CALLBACK (levels_high_input_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->input_sliders), 2,
                                  tool->high_input);

  /*  Output levels frame  */
  frame = gimp_frame_new (_("Output Levels"));
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), border);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  tool->output_bar = g_object_new (GIMP_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->output_bar, -1, GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->output_bar, FALSE, FALSE, 0);
  gtk_widget_show (tool->output_bar);

  tool->output_sliders = g_object_new (GIMP_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (tool->output_sliders, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->output_sliders, FALSE, FALSE, 0);
  gtk_widget_show (tool->output_sliders);

  g_signal_connect_swapped (tool->output_bar, "button-press-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->output_sliders)->button_press_event),
                            tool->output_sliders);

  g_signal_connect_swapped (tool->output_bar, "button-release-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->output_sliders)->button_release_event),
                            tool->output_sliders);

  g_signal_connect_swapped (tool->output_bar, "motion-notify-event",
                            G_CALLBACK (GTK_WIDGET_GET_CLASS (tool->output_sliders)->motion_notify_event),
                            tool->output_sliders);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low output spin  */
  spinbutton = gimp_spin_button_new (&data, 0, 0, 255, 1, 10, 10, 0.5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->low_output = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->low_output, "value-changed",
                    G_CALLBACK (levels_low_output_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->output_sliders), 0,
                                  tool->low_output);

  /*  high output spin  */
  spinbutton = gimp_spin_button_new (&data, 255, 0, 255, 1, 10, 10, 0.5, 0);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->high_output = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->high_output, "value-changed",
                    G_CALLBACK (levels_high_output_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->output_sliders), 2,
                                  tool->high_output);


  /*  all channels frame  */
  frame = gimp_frame_new (_("All Channels"));
  gtk_box_pack_end (GTK_BOX (image_map_tool->main_vbox), frame,
                    FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  hbbox = gtk_hbutton_box_new ();
  gtk_box_set_spacing (GTK_BOX (hbbox), 4);
  gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  gtk_box_pack_start (GTK_BOX (hbbox), image_map_tool->load_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->load_button);

  gtk_box_pack_start (GTK_BOX (hbbox), image_map_tool->save_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->save_button);

  hbbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_end (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_mnemonic (_("_Auto"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Adjust levels automatically"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_stretch_callback),
                    tool);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_LOW_INPUT | PICK_ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_GAMMA | PICK_ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_HIGH_INPUT | PICK_ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (image_map_tool->shell, "unmap",
                    G_CALLBACK (gimp_levels_tool_dialog_unmap),
                    tool);
}

static void
gimp_levels_tool_dialog_unmap (GtkWidget      *dialog,
                               GimpLevelsTool *tool)
{
  if (tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tool->active_picker),
                                  FALSE);
}

static void
gimp_levels_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (image_map_tool);

  gimp_levels_config_reset (tool->config);

  levels_init (tool->levels);
  levels_update_adjustments (tool);
}

static gboolean
gimp_levels_tool_settings_load (GimpImageMapTool  *image_map_tool,
                                gpointer           fp,
                                GError           **error)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (image_map_tool);
  FILE           *file = fp;
  gint            low_input[5];
  gint            high_input[5];
  gint            low_output[5];
  gint            high_output[5];
  gdouble         gamma[5];
  gint            i, fields;
  gchar           buf[50];
  gchar          *nptr;

  if (! fgets (buf, sizeof (buf), file) ||
      strcmp (buf, "# GIMP Levels File\n") != 0)
    {
      g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                   _("not a GIMP Levels file"));
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      fields = fscanf (file, "%d %d %d %d ",
                       &low_input[i],
                       &high_input[i],
                       &low_output[i],
                       &high_output[i]);

      if (fields != 4)
        goto error;

      if (! fgets (buf, 50, file))
        goto error;

      gamma[i] = g_ascii_strtod (buf, &nptr);

      if (buf == nptr || errno == ERANGE)
        goto error;
    }

  for (i = 0; i < 5; i++)
    {
      tool->levels->low_input[i]   = low_input[i];
      tool->levels->high_input[i]  = high_input[i];
      tool->levels->low_output[i]  = low_output[i];
      tool->levels->high_output[i] = high_output[i];
      tool->levels->gamma[i]       = gamma[i];
    }

  levels_update_adjustments (tool);

  return TRUE;

 error:
  g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
               _("parse error"));
  return FALSE;
}

static gboolean
gimp_levels_tool_settings_save (GimpImageMapTool *image_map_tool,
                                gpointer          fp)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (image_map_tool);
  FILE           *file = fp;
  gint            i;

  fprintf (file, "# GIMP Levels File\n");

  for (i = 0; i < 5; i++)
    {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

      fprintf (file, "%d %d %d %d %s\n",
               tool->levels->low_input[i],
               tool->levels->high_input[i],
               tool->levels->low_output[i],
               tool->levels->high_output[i],
               g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                                tool->levels->gamma[i]));
    }

  return TRUE;
}

static void
levels_update_adjustments (GimpLevelsTool *tool)
{
  tool->low_input->upper    = 255;
  tool->high_input->lower   = 0;
  tool->gamma_linear->lower = 0;
  tool->gamma_linear->upper = 255;

  gtk_adjustment_set_value (tool->low_input,
                            tool->levels->low_input[tool->channel]);
  gtk_adjustment_set_value (tool->gamma,
                            tool->levels->gamma[tool->channel]);
  gtk_adjustment_set_value (tool->high_input,
                            tool->levels->high_input[tool->channel]);

  tool->low_input->upper    = tool->high_input->value;
  tool->high_input->lower   = tool->low_input->value;
  tool->gamma_linear->lower = tool->low_input->value;
  tool->gamma_linear->upper = tool->high_input->value;
  gtk_adjustment_changed (tool->low_input);
  gtk_adjustment_changed (tool->high_input);
  gtk_adjustment_changed (tool->gamma_linear);

  gtk_adjustment_set_value (tool->low_output,
                            tool->levels->low_output[tool->channel]);
  gtk_adjustment_set_value (tool->high_output,
                            tool->levels->high_output[tool->channel]);

  levels_update_input_bar (tool);
}

static void
levels_update_input_bar (GimpLevelsTool *tool)
{
  GimpHistogramChannel channel = tool->channel;

  /* FIXME: hack */
  if (! tool->color && channel == 1)
    channel = GIMP_HISTOGRAM_ALPHA;

  /*  Recalculate the transfer arrays  */
  levels_calculate_transfers (tool->levels);

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:
    case GIMP_HISTOGRAM_RGB:
      gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->input_bar),
                                  tool->levels->input[tool->channel],
                                  tool->levels->input[tool->channel],
                                  tool->levels->input[tool->channel]);
      break;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->input_bar),
                                  tool->levels->input[GIMP_HISTOGRAM_RED],
                                  tool->levels->input[GIMP_HISTOGRAM_GREEN],
                                  tool->levels->input[GIMP_HISTOGRAM_BLUE]);
      break;
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
                         GimpLevelsTool *tool)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      tool->channel = value;
      gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (tool->hist_view),
                                       tool->channel);
      gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->output_bar),
                                  tool->channel);

      /* FIXME: hack */
      if (! tool->color)
        tool->channel = (tool->channel == GIMP_HISTOGRAM_ALPHA) ? 1 : 0;

      levels_update_adjustments (tool);
    }
}

static void
levels_channel_reset_callback (GtkWidget      *widget,
                               GimpLevelsTool *tool)
{
  GimpHistogramChannel channel = tool->channel;

  /* FIXME: hack */
  if (! tool->color && channel == 1)
    channel = GIMP_HISTOGRAM_ALPHA;

  gimp_levels_config_reset_channel (tool->config, channel);

  levels_channel_reset (tool->levels, tool->channel);
  levels_update_adjustments (tool);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static gboolean
levels_menu_sensitivity (gint      value,
                         gpointer  data)
{
  GimpLevelsTool       *tool    = GIMP_LEVELS_TOOL (data);
  GimpHistogramChannel  channel = value;

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return tool->color;

    case GIMP_HISTOGRAM_ALPHA:
      return tool->alpha;

    case GIMP_HISTOGRAM_RGB:
      return FALSE;
    }

  return FALSE;
}

static void
levels_stretch_callback (GtkWidget      *widget,
                         GimpLevelsTool *tool)
{
  levels_stretch (tool->levels, tool->hist, tool->color);
  levels_update_adjustments (tool);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static void
levels_linear_gamma_update (GimpLevelsTool *tool)
{
  gdouble delta, mid, tmp, value;

  delta = (tool->high_input->value - tool->low_input->value) / 2.0;
  mid   = tool->low_input->value + delta;
  tmp   = log10 (1.0 / tool->levels->gamma[tool->channel]);
  value = mid + delta * tmp;

  gtk_adjustment_set_value (tool->gamma_linear, value);
}

static void
levels_linear_gamma_changed (GtkAdjustment  *adjustment,
                             GimpLevelsTool *tool)
{
  gdouble delta, mid, tmp, value;

  delta = (tool->high_input->value - tool->low_input->value) / 2.0;

  if (delta >= 0.5)
    {
      mid   = tool->low_input->value + delta;
      tmp   = (adjustment->value - mid) / delta;
      value = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      value = floor (value * 100 + 0.5) / 100.0;

      gtk_adjustment_set_value (tool->gamma, value);
    }
}

static void
levels_low_input_changed (GtkAdjustment  *adjustment,
                          GimpLevelsTool *tool)
{
  gint value = ROUND (adjustment->value);

  tool->high_input->lower   = value;
  tool->gamma_linear->lower = value;
  gtk_adjustment_changed (tool->high_input);
  gtk_adjustment_changed (tool->gamma_linear);

  if (tool->levels->low_input[tool->channel] != value)
    {
      tool->levels->low_input[tool->channel] = value;
      levels_update_input_bar (tool);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }

  levels_linear_gamma_update (tool);
}

static void
levels_gamma_changed (GtkAdjustment  *adjustment,
                      GimpLevelsTool *tool)
{
  if (tool->levels->gamma[tool->channel] != adjustment->value)
    {
      tool->levels->gamma[tool->channel] = adjustment->value;
      levels_update_input_bar (tool);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }

  levels_linear_gamma_update (tool);
}

static void
levels_high_input_changed (GtkAdjustment  *adjustment,
                           GimpLevelsTool *tool)
{
  gint value = ROUND (adjustment->value);

  tool->low_input->upper    = value;
  tool->gamma_linear->upper = value;
  gtk_adjustment_changed (tool->low_input);
  gtk_adjustment_changed (tool->gamma_linear);

  if (tool->levels->high_input[tool->channel] != value)
    {
      tool->levels->high_input[tool->channel] = value;
      levels_update_input_bar (tool);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }

  levels_linear_gamma_update (tool);
}

static void
levels_low_output_changed (GtkAdjustment  *adjustment,
                           GimpLevelsTool *tool)
{
  gint value = ROUND (adjustment->value);

  if (tool->levels->low_output[tool->channel] != value)
    {
      tool->levels->low_output[tool->channel] = value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }
}

static void
levels_high_output_changed (GtkAdjustment  *adjustment,
                            GimpLevelsTool *tool)
{
  gint value = ROUND (adjustment->value);

  if (tool->levels->high_output[tool->channel] != value)
    {
      tool->levels->high_output[tool->channel] = value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }
}

static void
levels_input_picker_toggled (GtkWidget      *widget,
                             GimpLevelsTool *tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      if (tool->active_picker == widget)
        return;

      if (tool->active_picker)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tool->active_picker),
                                      FALSE);

      tool->active_picker = widget;

      gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                              GIMP_COLOR_TOOL_GET_OPTIONS (tool));
    }
  else if (tool->active_picker == widget)
    {
      tool->active_picker = NULL;
      gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
    }
}

static void
levels_input_adjust_by_color (Levels               *levels,
                              guint                 value,
                              GimpHistogramChannel  channel,
                              guchar               *color)
{
  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      levels_adjust_by_colors (levels, channel, color, NULL, NULL);
      break;
    case PICK_GAMMA:
      levels_adjust_by_colors (levels, channel, NULL, color, NULL);
      break;
    case PICK_HIGH_INPUT:
      levels_adjust_by_colors (levels, channel, NULL, NULL, color);
      break;
    default:
      break;
    }
}

static void
gimp_levels_tool_color_picked (GimpColorTool      *color_tool,
                               GimpColorPickState  pick_state,
                               GimpImageType       sample_type,
                               GimpRGB            *color,
                               gint                color_index)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (color_tool);
  guchar          col[5];
  guint           value;

  gimp_rgba_get_uchar (color,
                       col + RED_PIX,
                       col + GREEN_PIX,
                       col + BLUE_PIX,
                       col + ALPHA_PIX);

  value = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tool->active_picker),
                                               "pick-value"));

  if (value & PICK_ALL_CHANNELS && GIMP_IMAGE_TYPE_IS_RGB (sample_type))
    {
      GimpHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
        {
        case PICK_LOW_INPUT:
          tool->levels->low_input[GIMP_HISTOGRAM_VALUE] = 0;
          break;
        case PICK_GAMMA:
          tool->levels->gamma[GIMP_HISTOGRAM_VALUE] = 1.0;
          break;
        case PICK_HIGH_INPUT:
          tool->levels->high_input[GIMP_HISTOGRAM_VALUE] = 255;
          break;
        default:
          break;
        }

      /*  then adjust all color channels  */
      for (channel = GIMP_HISTOGRAM_RED;
           channel <= GIMP_HISTOGRAM_BLUE;
           channel++)
        {
          levels_input_adjust_by_color (tool->levels,
                                        value, channel, col);
        }
    }
  else
    {
      levels_input_adjust_by_color (tool->levels,
                                    value, tool->channel, col);
    }

  levels_update_adjustments (tool);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}
