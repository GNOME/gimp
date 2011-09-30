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

#include <errno.h>

#include <glib/gstdio.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
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
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimphandlebar.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogramview.h"
#include "widgets/gimpwidgets-constructors.h"

#include "display/gimpdisplay.h"

#include "gimphistogramoptions.h"
#include "gimplevelstool.h"

#include "gimp-intl.h"


#define PICK_LOW_INPUT    (1 << 0)
#define PICK_GAMMA        (1 << 1)
#define PICK_HIGH_INPUT   (1 << 2)
#define PICK_ALL_CHANNELS (1 << 8)

#define HISTOGRAM_WIDTH    256
#define GRADIENT_HEIGHT     12
#define CONTROL_HEIGHT      10


/*  local function prototypes  */

static void       gimp_levels_tool_finalize       (GObject           *object);

static gboolean   gimp_levels_tool_initialize     (GimpTool          *tool,
                                                   GimpDisplay       *display,
                                                   GError           **error);

static void       gimp_levels_tool_color_picked   (GimpColorTool     *color_tool,
                                                   GimpColorPickState pick_state,
                                                   GimpImageType      sample_type,
                                                   const GimpRGB     *color,
                                                   gint               color_index);

static GeglNode * gimp_levels_tool_get_operation  (GimpImageMapTool  *im_tool,
                                                   GObject          **config);
static void       gimp_levels_tool_map            (GimpImageMapTool  *im_tool);
static void       gimp_levels_tool_dialog         (GimpImageMapTool  *im_tool);
static void       gimp_levels_tool_dialog_unmap   (GtkWidget         *dialog,
                                                   GimpLevelsTool    *tool);
static void       gimp_levels_tool_reset          (GimpImageMapTool  *im_tool);
static gboolean   gimp_levels_tool_settings_import(GimpImageMapTool  *im_tool,
                                                   const gchar       *filename,
                                                   GError           **error);
static gboolean   gimp_levels_tool_settings_export(GimpImageMapTool  *im_tool,
                                                   const gchar       *filename,
                                                   GError           **error);

static void       gimp_levels_tool_export_setup   (GimpSettingsBox      *settings_box,
                                                   GtkFileChooserDialog *dialog,
                                                   gboolean              export,
                                                   GimpLevelsTool       *tool);
static void       gimp_levels_tool_config_notify  (GObject           *object,
                                                   GParamSpec        *pspec,
                                                   GimpLevelsTool    *tool);

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

static void       levels_to_curves_callback       (GtkWidget         *widget,
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

  object_class->finalize             = gimp_levels_tool_finalize;

  tool_class->initialize             = gimp_levels_tool_initialize;

  color_tool_class->picked           = gimp_levels_tool_color_picked;

  im_tool_class->dialog_desc         = _("Adjust Color Levels");
  im_tool_class->settings_name       = "levels";
  im_tool_class->import_dialog_title = _("Import Levels");
  im_tool_class->export_dialog_title = _("Export Levels");

  im_tool_class->get_operation       = gimp_levels_tool_get_operation;
  im_tool_class->map                 = gimp_levels_tool_map;
  im_tool_class->dialog              = gimp_levels_tool_dialog;
  im_tool_class->reset               = gimp_levels_tool_reset;
  im_tool_class->settings_import     = gimp_levels_tool_settings_import;
  im_tool_class->settings_export     = gimp_levels_tool_settings_export;
}

static void
gimp_levels_tool_init (GimpLevelsTool *tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  tool->lut           = gimp_lut_new ();
  tool->histogram     = gimp_histogram_new ();
  tool->active_picker = NULL;

  im_tool->apply_func = (GimpImageMapApplyFunc) gimp_lut_process;
  im_tool->apply_data = tool->lut;
}

static void
gimp_levels_tool_finalize (GObject *object)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (object);

  gimp_lut_free (tool->lut);

  if (tool->histogram)
    {
      gimp_histogram_unref (tool->histogram);
      tool->histogram = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_levels_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpLevelsTool *l_tool   = GIMP_LEVELS_TOOL (tool);
  GimpImage      *image    = gimp_display_get_image (display);
  GimpDrawable   *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Levels does not operate on indexed layers."));
      return FALSE;
    }

  gimp_config_reset (GIMP_CONFIG (l_tool->config));

  if (l_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (l_tool->active_picker),
                                  FALSE);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (l_tool->channel_menu),
                                      levels_menu_sensitivity, drawable, NULL);

  gimp_drawable_calculate_histogram (drawable, l_tool->histogram);
  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (l_tool->histogram_view),
                                     l_tool->histogram);

  return TRUE;
}

static GeglNode *
gimp_levels_tool_get_operation (GimpImageMapTool  *im_tool,
                                GObject          **config)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (im_tool);
  GeglNode       *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:levels",
                       NULL);

  tool->config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  *config = G_OBJECT (tool->config);

  g_signal_connect_object (tool->config, "notify",
                           G_CALLBACK (gimp_levels_tool_config_notify),
                           G_OBJECT (tool), 0);

  gegl_node_set (node,
                 "config", tool->config,
                 NULL);

  return node;
}

static void
gimp_levels_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool *tool     = GIMP_LEVELS_TOOL (image_map_tool);
  GimpDrawable   *drawable = image_map_tool->drawable;
  Levels          levels;

  gimp_levels_config_to_cruft (tool->config, &levels,
                               gimp_drawable_is_rgb (drawable));

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) levels_lut_func,
                  &levels,
                  gimp_drawable_bytes (drawable));
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
  GimpLevelsTool   *tool         = GIMP_LEVELS_TOOL (image_map_tool);
  GimpToolOptions  *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GimpLevelsConfig *config       = tool->config;
  GtkListStore     *store;
  GtkSizeGroup     *label_group;
  GtkWidget        *main_vbox;
  GtkWidget        *vbox;
  GtkWidget        *vbox2;
  GtkWidget        *vbox3;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *menu;
  GtkWidget        *frame;
  GtkWidget        *hbbox;
  GtkWidget        *button;
  GtkWidget        *spinbutton;
  GtkWidget        *bar;
  GtkObject        *data;
  gint              border;

  g_signal_connect (image_map_tool->settings_box, "file-dialog-setup",
                    G_CALLBACK (gimp_levels_tool_export_setup),
                    image_map_tool);

  main_vbox   = gimp_image_map_tool_dialog_get_vbox (image_map_tool);
  label_group = gimp_image_map_tool_dialog_get_label_group (image_map_tool);

  /*  The option menu for selecting channels  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_size_group_add_widget (label_group, label);

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
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  tool->histogram_view = gimp_histogram_view_new (FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->histogram_view, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (tool->histogram_view));

  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       GIMP_HISTOGRAM_VIEW (tool->histogram_view));

  g_object_get (tool->histogram_view, "border-width", &border, NULL);

  vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
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
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (tool, PICK_LOW_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = gimp_spin_button_new (&data,
                                     config->low_input[config->channel] * 255.0,
                                     0, 255, 1, 10, 0, 0.5, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->low_input = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->low_input, "value-changed",
                    G_CALLBACK (levels_low_input_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->input_sliders), 0,
                                  tool->low_input);

  /*  input gamma spin  */
  spinbutton = gimp_spin_button_new (&data,
                                     config->gamma[config->channel],
                                     0.1, 10, 0.01, 0.1, 0, 0.5, 2);
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
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (tool, PICK_HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = gimp_spin_button_new (&data,
                                     config->high_input[config->channel] * 255.0,
                                     0, 255, 1, 10, 0, 0.5, 0);
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
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
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
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low output spin  */
  spinbutton = gimp_spin_button_new (&data,
                                     config->low_output[config->channel] * 255.0,
                                     0, 255, 1, 10, 0, 0.5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->low_output = GTK_ADJUSTMENT (data);
  g_signal_connect (tool->low_output, "value-changed",
                    G_CALLBACK (levels_low_output_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (tool->output_sliders), 0,
                                  tool->low_output);

  /*  high output spin  */
  spinbutton = gimp_spin_button_new (&data,
                                     config->high_output[config->channel] * 255.0,
                                     0, 255, 1, 10, 0, 0.5, 0);
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
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  hbbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
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

  g_signal_connect (image_map_tool->dialog, "unmap",
                    G_CALLBACK (gimp_levels_tool_dialog_unmap),
                    tool);

  button = gimp_stock_button_new (GIMP_STOCK_TOOL_CURVES,
                                  _("Edit these Settings as Curves"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_to_curves_callback),
                    tool);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
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
  GimpLevelsTool       *tool    = GIMP_LEVELS_TOOL (image_map_tool);
  GimpHistogramChannel  channel = tool->config->channel;

  g_object_freeze_notify (image_map_tool->config);

  if (image_map_tool->default_config)
    {
      gimp_config_copy (GIMP_CONFIG (image_map_tool->default_config),
                        GIMP_CONFIG (image_map_tool->config),
                        0);
    }
  else
    {
      gimp_config_reset (GIMP_CONFIG (image_map_tool->config));
    }

  g_object_set (tool->config,
                "channel", channel,
                NULL);

  g_object_thaw_notify (image_map_tool->config);
}

static gboolean
gimp_levels_tool_settings_import (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (image_map_tool);
  FILE           *file;
  gchar           header[64];

  file = g_fopen (filename, "rt");

  if (! file)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename),
                   g_strerror (errno));
      return FALSE;
    }

  if (! fgets (header, sizeof (header), file))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not read header from '%s': %s"),
                   gimp_filename_to_utf8 (filename),
                   g_strerror (errno));
      fclose (file);
      return FALSE;
    }

  if (g_str_has_prefix (header, "# GIMP Levels File\n"))
    {
      gboolean success;

      rewind (file);

      success = gimp_levels_config_load_cruft (tool->config, file, error);

      fclose (file);

      return success;
    }

  fclose (file);

  return GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->settings_import (image_map_tool,
                                                                    filename,
                                                                    error);
}

static gboolean
gimp_levels_tool_settings_export (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (image_map_tool);

  if (tool->export_old_format)
    {
      FILE     *file;
      gboolean  success;

      file = g_fopen (filename, "wt");

      if (! file)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for writing: %s"),
                       gimp_filename_to_utf8 (filename),
                       g_strerror (errno));
          return FALSE;
        }

      success = gimp_levels_config_save_cruft (tool->config, file, error);

      fclose (file);

      return success;
    }

  return GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->settings_export (image_map_tool,
                                                                    filename,
                                                                    error);
}

static void
gimp_levels_tool_export_setup (GimpSettingsBox      *settings_box,
                               GtkFileChooserDialog *dialog,
                               gboolean              export,
                               GimpLevelsTool       *tool)
{
  GtkWidget *button;

  if (! export)
    return;

  button = gtk_check_button_new_with_mnemonic (_("Use _old levels file format"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                tool->export_old_format);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tool->export_old_format);
}

static void
gimp_levels_tool_config_notify (GObject        *object,
                                GParamSpec     *pspec,
                                GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = GIMP_LEVELS_CONFIG (object);

  if (! tool->low_input)
    return;

  if (! strcmp (pspec->name, "channel"))
    {
      gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (tool->histogram_view),
                                       config->channel);
      gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->output_bar),
                                  config->channel);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                     config->channel);
    }
  else if (! strcmp (pspec->name, "gamma")     ||
           ! strcmp (pspec->name, "low-input") ||
           ! strcmp (pspec->name, "high-input"))
    {
      g_object_freeze_notify (G_OBJECT (tool->low_input));
      g_object_freeze_notify (G_OBJECT (tool->high_input));
      g_object_freeze_notify (G_OBJECT (tool->gamma_linear));

      gtk_adjustment_set_upper (tool->low_input,  255);
      gtk_adjustment_set_lower (tool->high_input, 0);

      gtk_adjustment_set_lower (tool->gamma_linear, 0);
      gtk_adjustment_set_upper (tool->gamma_linear, 255);

      gtk_adjustment_set_value (tool->low_input,
                                config->low_input[config->channel]  * 255.0);
      gtk_adjustment_set_value (tool->gamma,
                                config->gamma[config->channel]);
      gtk_adjustment_set_value (tool->high_input,
                                config->high_input[config->channel] * 255.0);

      gtk_adjustment_set_upper (tool->low_input,
                                gtk_adjustment_get_value (tool->high_input));
      gtk_adjustment_set_lower (tool->high_input,
                                gtk_adjustment_get_value (tool->low_input));

      gtk_adjustment_set_lower (tool->gamma_linear,
                                gtk_adjustment_get_value (tool->low_input));
      gtk_adjustment_set_upper (tool->gamma_linear,
                                gtk_adjustment_get_value (tool->high_input));

      g_object_thaw_notify (G_OBJECT (tool->low_input));
      g_object_thaw_notify (G_OBJECT (tool->high_input));
      g_object_thaw_notify (G_OBJECT (tool->gamma_linear));

      levels_update_input_bar (tool);
    }
  else if (! strcmp (pspec->name, "low-output"))
    {
      gtk_adjustment_set_value (tool->low_output,
                                config->low_output[config->channel] * 255.0);
    }
  else if (! strcmp (pspec->name, "high-output"))
    {
      gtk_adjustment_set_value (tool->high_output,
                                config->high_output[config->channel] * 255.0);
    }

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static void
levels_update_input_bar (GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = tool->config;

  switch (config->channel)
    {
    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:
    case GIMP_HISTOGRAM_RGB:
      {
        guchar v[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            v[i] = gimp_operation_levels_map_input (config,
                                                    config->channel,
                                                    i / 255.0) * 255.999;
          }

        gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->input_bar),
                                    v, v, v);
      }
      break;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      {
        guchar r[256];
        guchar g[256];
        guchar b[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            r[i] = gimp_operation_levels_map_input (config,
                                                    GIMP_HISTOGRAM_RED,
                                                    i / 255.0) * 255.999;
            g[i] = gimp_operation_levels_map_input (config,
                                                    GIMP_HISTOGRAM_GREEN,
                                                    i / 255.0) * 255.999;
            b[i] = gimp_operation_levels_map_input (config,
                                                    GIMP_HISTOGRAM_BLUE,
                                                    i / 255.0) * 255.999;
          }

        gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->input_bar),
                                    r, g, b);
      }
      break;
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
                         GimpLevelsTool *tool)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value) &&
      tool->config->channel != value)
    {
      g_object_set (tool->config,
                    "channel", value,
                    NULL);
    }
}

static void
levels_channel_reset_callback (GtkWidget      *widget,
                               GimpLevelsTool *tool)
{
  gimp_levels_config_reset_channel (tool->config);
}

static gboolean
levels_menu_sensitivity (gint      value,
                         gpointer  data)
{
  GimpDrawable         *drawable = GIMP_DRAWABLE (data);
  GimpHistogramChannel  channel  = value;

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return gimp_drawable_is_rgb (drawable);

    case GIMP_HISTOGRAM_ALPHA:
      return gimp_drawable_has_alpha (drawable);

    case GIMP_HISTOGRAM_RGB:
      return FALSE;
    }

  return FALSE;
}

static void
levels_stretch_callback (GtkWidget      *widget,
                         GimpLevelsTool *tool)
{
  GimpDrawable *drawable = GIMP_IMAGE_MAP_TOOL (tool)->drawable;

  gimp_levels_config_stretch (tool->config, tool->histogram,
                              gimp_drawable_is_rgb (drawable));
}

static void
levels_linear_gamma_update (GimpLevelsTool *tool)
{
  gdouble low_input  = gtk_adjustment_get_value (tool->low_input);
  gdouble high_input = gtk_adjustment_get_value (tool->high_input);
  gdouble delta, mid, tmp, value;

  delta = (high_input - low_input) / 2.0;
  mid   = low_input + delta;
  tmp   = log10 (1.0 / tool->config->gamma[tool->config->channel]);
  value = mid + delta * tmp;

  gtk_adjustment_set_value (tool->gamma_linear, value);
}

static void
levels_linear_gamma_changed (GtkAdjustment  *adjustment,
                             GimpLevelsTool *tool)
{
  gdouble low_input  = gtk_adjustment_get_value (tool->low_input);
  gdouble high_input = gtk_adjustment_get_value (tool->high_input);
  gdouble delta, mid, tmp, value;

  delta = (high_input - low_input) / 2.0;

  if (delta >= 0.5)
    {
      mid   = low_input + delta;
      tmp   = (gtk_adjustment_get_value (adjustment) - mid) / delta;
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
  GimpLevelsConfig *config = tool->config;
  gint              value  = ROUND (gtk_adjustment_get_value (adjustment));

  gtk_adjustment_set_lower (tool->high_input, value);
  gtk_adjustment_set_lower (tool->gamma_linear, value);

  if (config->low_input[config->channel] != value / 255.0)
    {
      g_object_set (config,
                    "low-input", value / 255.0,
                    NULL);
    }

  levels_linear_gamma_update (tool);
}

static void
levels_gamma_changed (GtkAdjustment  *adjustment,
                      GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = tool->config;
  gdouble           value  = gtk_adjustment_get_value (adjustment);

  if (config->gamma[config->channel] != value)
    {
      g_object_set (config,
                    "gamma", value,
                    NULL);
    }

  levels_linear_gamma_update (tool);
}

static void
levels_high_input_changed (GtkAdjustment  *adjustment,
                           GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = tool->config;
  gint              value  = ROUND (gtk_adjustment_get_value (adjustment));

  gtk_adjustment_set_upper (tool->low_input, value);
  gtk_adjustment_set_upper (tool->gamma_linear, value);

  if (config->high_input[config->channel] != value / 255.0)
    {
      g_object_set (config,
                    "high-input", value / 255.0,
                    NULL);
    }

  levels_linear_gamma_update (tool);
}

static void
levels_low_output_changed (GtkAdjustment  *adjustment,
                           GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = tool->config;
  gint              value  = ROUND (gtk_adjustment_get_value (adjustment));

  if (config->low_output[config->channel] != value / 255.0)
    {
      g_object_set (config,
                    "low-output", value / 255.0,
                    NULL);
    }
}

static void
levels_high_output_changed (GtkAdjustment  *adjustment,
                            GimpLevelsTool *tool)
{
  GimpLevelsConfig *config = tool->config;
  gint              value  = ROUND (gtk_adjustment_get_value (adjustment));

  if (config->high_output[config->channel] != value / 255.0)
    {
      g_object_set (config,
                    "high-output", value / 255.0,
                    NULL);
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
levels_input_adjust_by_color (GimpLevelsConfig     *config,
                              guint                 value,
                              GimpHistogramChannel  channel,
                              const GimpRGB        *color)
{
  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      gimp_levels_config_adjust_by_colors (config, channel, color, NULL, NULL);
      break;
    case PICK_GAMMA:
      gimp_levels_config_adjust_by_colors (config, channel, NULL, color, NULL);
      break;
    case PICK_HIGH_INPUT:
      gimp_levels_config_adjust_by_colors (config, channel, NULL, NULL, color);
      break;
    default:
      break;
    }
}

static void
gimp_levels_tool_color_picked (GimpColorTool      *color_tool,
                               GimpColorPickState  pick_state,
                               GimpImageType       sample_type,
                               const GimpRGB      *color,
                               gint                color_index)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (color_tool);
  guint           value;

  value = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tool->active_picker),
                                               "pick-value"));

  if (value & PICK_ALL_CHANNELS && GIMP_IMAGE_TYPE_IS_RGB (sample_type))
    {
      GimpHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
        {
        case PICK_LOW_INPUT:
          tool->config->low_input[GIMP_HISTOGRAM_VALUE] = 0.0;
          break;
        case PICK_GAMMA:
          tool->config->gamma[GIMP_HISTOGRAM_VALUE] = 1.0;
          break;
        case PICK_HIGH_INPUT:
          tool->config->high_input[GIMP_HISTOGRAM_VALUE] = 1.0;
          break;
        default:
          break;
        }

      /*  then adjust all color channels  */
      for (channel = GIMP_HISTOGRAM_RED;
           channel <= GIMP_HISTOGRAM_BLUE;
           channel++)
        {
          levels_input_adjust_by_color (tool->config,
                                        value, channel, color);
        }
    }
  else
    {
      levels_input_adjust_by_color (tool->config,
                                    value, tool->config->channel, color);
    }
}

static void
levels_to_curves_callback (GtkWidget      *widget,
                           GimpLevelsTool *tool)
{
  GimpCurvesConfig *curves;

  curves = gimp_levels_config_to_curves_config (tool->config);

  gimp_image_map_tool_edit_as (GIMP_IMAGE_MAP_TOOL (tool),
                               "gimp-curves-tool",
                               GIMP_CONFIG (curves));

  g_object_unref (curves);
}
