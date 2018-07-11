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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gegl/gimp-babl.h"

#include "operations/gimplevelsconfig.h"
#include "operations/gimpoperationlevels.h"

#include "core/gimp-gui.h"
#include "core/gimpasync.h"
#include "core/gimpcancelable.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimperror.h"
#include "core/gimphistogram.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimptriviallycancelablewaitable.h"
#include "core/gimpwaitable.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimphandlebar.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogramview.h"
#include "widgets/gimppropwidgets.h"
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

static void       gimp_levels_tool_finalize       (GObject          *object);

static gboolean   gimp_levels_tool_initialize     (GimpTool         *tool,
                                                   GimpDisplay      *display,
                                                   GError          **error);

static gchar    * gimp_levels_tool_get_operation  (GimpFilterTool   *filter_tool,
                                                   gchar           **description);
static void       gimp_levels_tool_dialog         (GimpFilterTool   *filter_tool);
static void       gimp_levels_tool_reset          (GimpFilterTool   *filter_tool);
static void       gimp_levels_tool_config_notify  (GimpFilterTool   *filter_tool,
                                                   GimpConfig       *config,
                                                   const GParamSpec *pspec);
static gboolean   gimp_levels_tool_settings_import(GimpFilterTool   *filter_tool,
                                                   GInputStream     *input,
                                                   GError          **error);
static gboolean   gimp_levels_tool_settings_export(GimpFilterTool   *filter_tool,
                                                   GOutputStream    *output,
                                                   GError          **error);
static void       gimp_levels_tool_color_picked   (GimpFilterTool   *filter_tool,
                                                   gpointer          identifier,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   const Babl       *sample_format,
                                                   const GimpRGB    *color);

static void       gimp_levels_tool_export_setup   (GimpSettingsBox  *settings_box,
                                                   GtkFileChooserDialog *dialog,
                                                   gboolean          export,
                                                   GimpLevelsTool   *tool);

static void       levels_update_input_bar         (GimpLevelsTool   *tool);

static void       levels_channel_callback         (GtkWidget        *widget,
                                                   GimpFilterTool   *filter_tool);
static void       levels_channel_reset_callback   (GtkWidget        *widget,
                                                   GimpFilterTool   *filter_tool);

static gboolean   levels_menu_sensitivity         (gint              value,
                                                   gpointer          data);

static void       levels_stretch_callback         (GtkWidget        *widget,
                                                   GimpLevelsTool   *tool);
static void       levels_linear_gamma_changed     (GtkAdjustment    *adjustment,
                                                   GimpLevelsTool   *tool);

static void       levels_to_curves_callback       (GtkWidget        *widget,
                                                   GimpFilterTool   *filter_tool);


G_DEFINE_TYPE (GimpLevelsTool, gimp_levels_tool, GIMP_TYPE_FILTER_TOOL)

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
                _("Adjust color levels"),
                N_("_Levels..."), NULL,
                NULL, GIMP_HELP_TOOL_LEVELS,
                GIMP_ICON_TOOL_LEVELS,
                data);
}

static void
gimp_levels_tool_class_init (GimpLevelsToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  object_class->finalize             = gimp_levels_tool_finalize;

  tool_class->initialize             = gimp_levels_tool_initialize;

  filter_tool_class->get_operation   = gimp_levels_tool_get_operation;
  filter_tool_class->dialog          = gimp_levels_tool_dialog;
  filter_tool_class->reset           = gimp_levels_tool_reset;
  filter_tool_class->config_notify   = gimp_levels_tool_config_notify;
  filter_tool_class->settings_import = gimp_levels_tool_settings_import;
  filter_tool_class->settings_export = gimp_levels_tool_settings_export;
  filter_tool_class->color_picked    = gimp_levels_tool_color_picked;
}

static void
gimp_levels_tool_init (GimpLevelsTool *tool)
{
}

static void
gimp_levels_tool_finalize (GObject *object)
{
  GimpLevelsTool *tool = GIMP_LEVELS_TOOL (object);

  g_clear_object (&tool->histogram);
  g_clear_object (&tool->histogram_async);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_levels_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpLevelsTool   *l_tool      = GIMP_LEVELS_TOOL (tool);
  GimpImage        *image       = gimp_display_get_image (display);
  GimpDrawable     *drawable    = gimp_image_get_active_drawable (image);
  GimpLevelsConfig *config;
  gdouble           scale_factor;
  gdouble           step_increment;
  gdouble           page_increment;
  gint              digits;

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  config = GIMP_LEVELS_CONFIG (filter_tool->config);

  gegl_node_set (filter_tool->operation,
                 "linear", config->linear,
                 NULL);

  g_clear_object (&l_tool->histogram);
  g_clear_object (&l_tool->histogram_async);
  l_tool->histogram = gimp_histogram_new (config->linear);

  l_tool->histogram_async = gimp_drawable_calculate_histogram_async (
    drawable, l_tool->histogram, FALSE);
  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (l_tool->histogram_view),
                                     l_tool->histogram);

  if (gimp_drawable_get_component_type (drawable) == GIMP_COMPONENT_TYPE_U8)
    {
      scale_factor   = 255.0;
      step_increment = 1.0;
      page_increment = 8.0;
      digits         = 0;
    }
  else
    {
      scale_factor   = 100;
      step_increment = 0.01;
      page_increment = 1.0;
      digits         = 2;
    }

  gimp_prop_widget_set_factor (l_tool->low_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  gimp_prop_widget_set_factor (l_tool->high_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  gimp_prop_widget_set_factor (l_tool->low_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  gimp_prop_widget_set_factor (l_tool->high_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);

  gtk_adjustment_configure (l_tool->gamma_linear,
                            scale_factor / 2.0,
                            0, scale_factor, 0.1, 1.0, 0);

  return TRUE;
}

static gchar *
gimp_levels_tool_get_operation (GimpFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Levels"));

  return g_strdup ("gimp:levels");
}


/*******************/
/*  Levels dialog  */
/*******************/

static GtkWidget *
gimp_levels_tool_color_picker_new (GimpLevelsTool *tool,
                                   guint           value)
{
  const gchar *icon_name;
  const gchar *help;
  gboolean     all_channels = (value & PICK_ALL_CHANNELS) != 0;

  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      icon_name = GIMP_ICON_COLOR_PICKER_BLACK;

      if (all_channels)
        help = _("Pick black point for all channels");
      else
        help = _("Pick black point for the selected channel");
      break;

    case PICK_GAMMA:
      icon_name = GIMP_ICON_COLOR_PICKER_GRAY;

      if (all_channels)
        help = _("Pick gray point for all channels");
      else
        help = _("Pick gray point for the selected channel");
      break;

    case PICK_HIGH_INPUT:
      icon_name = GIMP_ICON_COLOR_PICKER_WHITE;

      if (all_channels)
        help = _("Pick white point for all channels");
      else
        help = _("Pick white point for the selected channel");
      break;

    default:
      return NULL;
    }

  return gimp_filter_tool_add_color_picker (GIMP_FILTER_TOOL (tool),
                                            GUINT_TO_POINTER (value),
                                            icon_name,
                                            help,
                                            /* pick_abyss = */ FALSE,
                                            NULL, NULL);
}

static void
gimp_levels_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpLevelsTool   *tool         = GIMP_LEVELS_TOOL (filter_tool);
  GimpToolOptions  *tool_options = GIMP_TOOL_GET_OPTIONS (filter_tool);
  GimpLevelsConfig *config       = GIMP_LEVELS_CONFIG (filter_tool->config);
  GtkListStore     *store;
  GtkWidget        *main_vbox;
  GtkWidget        *frame_vbox;
  GtkWidget        *vbox;
  GtkWidget        *vbox2;
  GtkWidget        *vbox3;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *main_frame;
  GtkWidget        *frame;
  GtkWidget        *button;
  GtkWidget        *spinbutton;
  GtkAdjustment    *adjustment;
  GtkWidget        *bar;
  GtkWidget        *handle_bar;
  gint              border;

  g_signal_connect (filter_tool->settings_box, "file-dialog-setup",
                    G_CALLBACK (gimp_levels_tool_export_setup),
                    filter_tool);

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  /*  The combo box for selecting channels  */
  main_frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, TRUE, TRUE, 0);
  gtk_widget_show (main_frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_frame_set_label_widget (GTK_FRAME (main_frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gimp_enum_store_new_with_range (GIMP_TYPE_HISTOGRAM_CHANNEL,
                                          GIMP_HISTOGRAM_VALUE,
                                          GIMP_HISTOGRAM_ALPHA);
  tool->channel_menu =
    gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));
  g_object_unref (store);

  g_object_add_weak_pointer (G_OBJECT (tool->channel_menu),
                             (gpointer) &tool->channel_menu);

  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (tool->channel_menu),
                                       "gimp-channel");
  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                      levels_menu_sensitivity, filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tool->channel_menu, FALSE, FALSE, 0);
  gtk_widget_show (tool->channel_menu);

  g_signal_connect (tool->channel_menu, "changed",
                    G_CALLBACK (levels_channel_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->channel_menu);

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_channel_reset_callback),
                    tool);

  /*  The histogram scale radio buttons  */
  hbox2 = gimp_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  /*  The linear/perceptual radio buttons  */
  hbox2 = gimp_prop_boolean_icon_box_new (G_OBJECT (config),
                                          "linear",
                                          GIMP_ICON_COLOR_SPACE_LINEAR,
                                          GIMP_ICON_COLOR_SPACE_PERCEPTUAL,
                                          _("Adjust levels in linear light"),
                                          _("Adjust levels perceptually"));
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_show (frame_vbox);

  /*  Input levels frame  */
  frame = gimp_frame_new (_("Input Levels"));
  gtk_box_pack_start (GTK_BOX (frame_vbox), frame, TRUE, TRUE, 0);
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

  g_object_add_weak_pointer (G_OBJECT (tool->histogram_view),
                             (gpointer) &tool->histogram_view);

  gtk_box_pack_start (GTK_BOX (vbox2), tool->histogram_view, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (tool->histogram_view));

  g_object_bind_property (G_OBJECT (tool_options),         "histogram-scale",
                          G_OBJECT (tool->histogram_view), "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

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

  handle_bar = g_object_new (GIMP_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox3), handle_bar, FALSE, FALSE, 0);
  gtk_widget_show (handle_bar);

  gimp_handle_bar_connect_events (GIMP_HANDLE_BAR (handle_bar),
                                  tool->input_bar);
  gimp_handle_bar_connect_events (GIMP_HANDLE_BAR (handle_bar),
                                  bar);

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

  tool->low_input_spinbutton = spinbutton =
    gimp_prop_spin_button_new (filter_tool->config, "low-input",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  tool->low_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 0,
                                  tool->low_input);

  /*  clamp input toggle  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_prop_check_button_new (filter_tool->config, "clamp-input",
                                       _("Clamp _input"));
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  input gamma spin  */
  spinbutton = gimp_prop_spin_button_new (filter_tool->config, "gamma",
                                          0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gimp_help_set_help_data (spinbutton, _("Gamma"), NULL);
  gtk_widget_show (spinbutton);

  tool->gamma = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

  tool->gamma_linear = GTK_ADJUSTMENT (gtk_adjustment_new (127, 0, 255,
                                                           0.1, 1.0, 0.0));
  g_signal_connect (tool->gamma_linear, "value-changed",
                    G_CALLBACK (levels_linear_gamma_changed),
                    tool);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 1,
                                  tool->gamma_linear);
  g_object_unref (tool->gamma_linear);

  /*  high input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (tool, PICK_HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = gimp_prop_spin_button_new (filter_tool->config, "high-input",
                                          0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);
  tool->high_input_spinbutton = spinbutton;

  tool->high_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 2,
                                  tool->high_input);

  /*  Output levels frame  */
  frame = gimp_frame_new (_("Output Levels"));
  gtk_box_pack_start (GTK_BOX (frame_vbox), frame, FALSE, FALSE, 0);
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

  handle_bar = g_object_new (GIMP_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), handle_bar, FALSE, FALSE, 0);
  gtk_widget_show (handle_bar);

  gimp_handle_bar_connect_events (GIMP_HANDLE_BAR (handle_bar),
                                  tool->output_bar);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low output spin  */
  tool->low_output_spinbutton = spinbutton =
    gimp_prop_spin_button_new (filter_tool->config, "low-output",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 0, adjustment);

  /*  clamp output toggle  */
  button = gimp_prop_check_button_new (filter_tool->config, "clamp-output",
                                       _("Clamp outpu_t"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_widget_show (button);

  /*  high output spin  */
  tool->high_output_spinbutton = spinbutton =
    gimp_prop_spin_button_new (filter_tool->config, "high-output",
                               0.01, 0.1, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 2, adjustment);

  /*  all channels frame  */
  main_frame = gimp_frame_new (_("All Channels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, FALSE, FALSE, 0);
  gtk_widget_show (main_frame);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_show (frame_vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Auto Input Levels"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button,
                           _("Adjust levels for all channels automatically"),
                           NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_stretch_callback),
                    tool);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_HIGH_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_GAMMA |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (tool,
                                              PICK_LOW_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_icon_button_new (GIMP_ICON_TOOL_CURVES,
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
gimp_levels_tool_reset (GimpFilterTool *filter_tool)
{
  GimpHistogramChannel channel;

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  GIMP_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  g_object_set (filter_tool->config,
                "channel", channel,
                NULL);
}

static void
gimp_levels_tool_config_notify (GimpFilterTool   *filter_tool,
                                GimpConfig       *config,
                                const GParamSpec *pspec)
{
  GimpLevelsTool   *levels_tool   = GIMP_LEVELS_TOOL (filter_tool);
  GimpLevelsConfig *levels_config = GIMP_LEVELS_CONFIG (config);

  GIMP_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! levels_tool->channel_menu ||
      ! levels_tool->histogram_view)
    return;

  if (! strcmp (pspec->name, "linear"))
    {
      gegl_node_set (filter_tool->operation,
                     "linear", levels_config->linear,
                     NULL);

      g_clear_object (&levels_tool->histogram);
      g_clear_object (&levels_tool->histogram_async);
      levels_tool->histogram = gimp_histogram_new (levels_config->linear);

      levels_tool->histogram_async = gimp_drawable_calculate_histogram_async (
        GIMP_TOOL (filter_tool)->drawable, levels_tool->histogram, FALSE);
      gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                         levels_tool->histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                       levels_config->channel);
      gimp_color_bar_set_channel (GIMP_COLOR_BAR (levels_tool->output_bar),
                                  levels_config->channel);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (levels_tool->channel_menu),
                                     levels_config->channel);
    }
  else if (! strcmp (pspec->name, "gamma")     ||
           ! strcmp (pspec->name, "low-input") ||
           ! strcmp (pspec->name, "high-input"))
    {
      gdouble low  = gtk_adjustment_get_value (levels_tool->low_input);
      gdouble high = gtk_adjustment_get_value (levels_tool->high_input);
      gdouble delta, mid, tmp, value;

      gtk_adjustment_set_lower (levels_tool->high_input,   low);
      gtk_adjustment_set_lower (levels_tool->gamma_linear, low);

      gtk_adjustment_set_upper (levels_tool->low_input,    high);
      gtk_adjustment_set_upper (levels_tool->gamma_linear, high);

      levels_update_input_bar (levels_tool);

      delta = (high - low) / 2.0;
      mid   = low + delta;
      tmp   = log10 (1.0 / levels_config->gamma[levels_config->channel]);
      value = mid + delta * tmp;

      gtk_adjustment_set_value (levels_tool->gamma_linear, value);
    }
}

static gboolean
gimp_levels_tool_settings_import (GimpFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  GimpLevelsConfig *config = GIMP_LEVELS_CONFIG (filter_tool->config);
  gchar             header[64];
  gsize             bytes_read;

  if (! g_input_stream_read_all (input, header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      g_prefix_error (error, _("Could not read header: "));
      return FALSE;
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  if (g_str_has_prefix (header, "# GIMP Levels File\n"))
    return gimp_levels_config_load_cruft (config, input, error);

  return GIMP_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
gimp_levels_tool_settings_export (GimpFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  GimpLevelsTool   *tool   = GIMP_LEVELS_TOOL (filter_tool);
  GimpLevelsConfig *config = GIMP_LEVELS_CONFIG (filter_tool->config);

  if (tool->export_old_format)
    return gimp_levels_config_save_cruft (config, output, error);

  return GIMP_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
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
gimp_levels_tool_color_picked (GimpFilterTool *color_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               const GimpRGB  *color)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (color_tool);
  GimpLevelsConfig *config      = GIMP_LEVELS_CONFIG (filter_tool->config);
  GimpRGB           rgb         = *color;
  guint             value       = GPOINTER_TO_UINT (identifier);

  if (config->linear)
    babl_process (babl_fish (babl_format ("R'G'B'A double"),
                             babl_format ("RGBA double")),
                  &rgb, &rgb, 1);

  if (value & PICK_ALL_CHANNELS &&
      gimp_babl_format_get_base_type (sample_format) == GIMP_RGB)
    {
      GimpHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
        {
        case PICK_LOW_INPUT:
          config->low_input[GIMP_HISTOGRAM_VALUE] = 0.0;
          break;
        case PICK_GAMMA:
          config->gamma[GIMP_HISTOGRAM_VALUE] = 1.0;
          break;
        case PICK_HIGH_INPUT:
          config->high_input[GIMP_HISTOGRAM_VALUE] = 1.0;
          break;
        default:
          break;
        }

      /*  then adjust all color channels  */
      for (channel = GIMP_HISTOGRAM_RED;
           channel <= GIMP_HISTOGRAM_BLUE;
           channel++)
        {
          levels_input_adjust_by_color (config, value, channel, &rgb);
        }
    }
  else
    {
      levels_input_adjust_by_color (config, value, config->channel, &rgb);
    }
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
levels_update_input_bar (GimpLevelsTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpLevelsConfig *config      = GIMP_LEVELS_CONFIG (filter_tool->config);

  switch (config->channel)
    {
      gdouble value;

    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:
    case GIMP_HISTOGRAM_RGB:
    case GIMP_HISTOGRAM_LUMINANCE:
      {
        guchar v[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            value = gimp_operation_levels_map_input (config,
                                                     config->channel,
                                                     i / 255.0);
            v[i] = CLAMP (value, 0.0, 1.0) * 255.999;
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
            value = gimp_operation_levels_map_input (config,
                                                     GIMP_HISTOGRAM_RED,
                                                     i / 255.0);
            r[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = gimp_operation_levels_map_input (config,
                                                     GIMP_HISTOGRAM_GREEN,
                                                     i / 255.0);
            g[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = gimp_operation_levels_map_input (config,
                                                     GIMP_HISTOGRAM_BLUE,
                                                     i / 255.0);
            b[i] = CLAMP (value, 0.0, 1.0) * 255.999;
          }

        gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->input_bar),
                                    r, g, b);
      }
      break;
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
                         GimpFilterTool *filter_tool)
{
  GimpLevelsConfig *config = GIMP_LEVELS_CONFIG (filter_tool->config);
  gint              value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value) &&
      config->channel != value)
    {
      g_object_set (config,
                    "channel", value,
                    NULL);
    }
}

static void
levels_channel_reset_callback (GtkWidget      *widget,
                               GimpFilterTool *filter_tool)
{
  gimp_levels_config_reset_channel (GIMP_LEVELS_CONFIG (filter_tool->config));
}

static gboolean
levels_menu_sensitivity (gint      value,
                         gpointer  data)
{
  GimpDrawable         *drawable = GIMP_TOOL (data)->drawable;
  GimpHistogramChannel  channel  = value;

  if (!drawable)
    return FALSE;

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

    case GIMP_HISTOGRAM_LUMINANCE:
      return FALSE;
    }

  return FALSE;
}

static void
levels_stretch_callback (GtkWidget      *widget,
                         GimpLevelsTool *levels_tool)
{
  GimpTool       *tool        = GIMP_TOOL (levels_tool);
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (levels_tool);
  GimpWaitable   *waitable;

  waitable = gimp_trivially_cancelable_waitable_new (
    GIMP_WAITABLE (levels_tool->histogram_async));

  gimp_wait (tool->tool_info->gimp, waitable, _("Calculating histogram..."));

  g_object_unref (waitable);

  if (gimp_async_is_synced   (levels_tool->histogram_async) &&
      gimp_async_is_finished (levels_tool->histogram_async))
    {
      gimp_levels_config_stretch (GIMP_LEVELS_CONFIG (filter_tool->config),
                                  levels_tool->histogram,
                                  gimp_drawable_is_rgb (tool->drawable));
    }
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
levels_to_curves_callback (GtkWidget      *widget,
                           GimpFilterTool *filter_tool)
{
  GimpLevelsConfig *config = GIMP_LEVELS_CONFIG (filter_tool->config);
  GimpCurvesConfig *curves;

  curves = gimp_levels_config_to_curves_config (config);

  gimp_filter_tool_edit_as (filter_tool,
                            "gimp-curves-tool",
                            GIMP_CONFIG (curves));

  g_object_unref (curves);
}
