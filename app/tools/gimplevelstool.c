/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "gegl/ligma-babl.h"

#include "operations/ligmalevelsconfig.h"
#include "operations/ligmaoperationlevels.h"

#include "core/ligma-gui.h"
#include "core/ligmaasync.h"
#include "core/ligmacancelable.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-histogram.h"
#include "core/ligmaerror.h"
#include "core/ligmahistogram.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatriviallycancelablewaitable.h"
#include "core/ligmawaitable.h"

#include "widgets/ligmacolorbar.h"
#include "widgets/ligmahandlebar.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmahistogramview.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-constructors.h"

#include "display/ligmadisplay.h"

#include "ligmahistogramoptions.h"
#include "ligmalevelstool.h"

#include "ligma-intl.h"


#define PICK_LOW_INPUT    (1 << 0)
#define PICK_GAMMA        (1 << 1)
#define PICK_HIGH_INPUT   (1 << 2)
#define PICK_ALL_CHANNELS (1 << 8)

#define HISTOGRAM_WIDTH    256
#define GRADIENT_HEIGHT     12
#define CONTROL_HEIGHT      10


/*  local function prototypes  */

static void       ligma_levels_tool_finalize       (GObject          *object);

static gboolean   ligma_levels_tool_initialize     (LigmaTool         *tool,
                                                   LigmaDisplay      *display,
                                                   GError          **error);

static gchar    * ligma_levels_tool_get_operation  (LigmaFilterTool   *filter_tool,
                                                   gchar           **description);
static void       ligma_levels_tool_dialog         (LigmaFilterTool   *filter_tool);
static void       ligma_levels_tool_reset          (LigmaFilterTool   *filter_tool);
static void       ligma_levels_tool_config_notify  (LigmaFilterTool   *filter_tool,
                                                   LigmaConfig       *config,
                                                   const GParamSpec *pspec);
static gboolean   ligma_levels_tool_settings_import(LigmaFilterTool   *filter_tool,
                                                   GInputStream     *input,
                                                   GError          **error);
static gboolean   ligma_levels_tool_settings_export(LigmaFilterTool   *filter_tool,
                                                   GOutputStream    *output,
                                                   GError          **error);
static void       ligma_levels_tool_color_picked   (LigmaFilterTool   *filter_tool,
                                                   gpointer          identifier,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   const Babl       *sample_format,
                                                   const LigmaRGB    *color);

static void       ligma_levels_tool_export_setup   (LigmaSettingsBox  *settings_box,
                                                   GtkFileChooserDialog *dialog,
                                                   gboolean          export,
                                                   LigmaLevelsTool   *tool);

static void       levels_update_input_bar         (LigmaLevelsTool   *tool);

static void       levels_channel_callback         (GtkWidget        *widget,
                                                   LigmaFilterTool   *filter_tool);
static void       levels_channel_reset_callback   (GtkWidget        *widget,
                                                   LigmaFilterTool   *filter_tool);

static gboolean   levels_menu_sensitivity         (gint              value,
                                                   gpointer          data);

static void       levels_stretch_callback         (GtkWidget        *widget,
                                                   LigmaLevelsTool   *tool);
static void       levels_linear_gamma_changed     (GtkAdjustment    *adjustment,
                                                   LigmaLevelsTool   *tool);

static void       levels_to_curves_callback       (GtkWidget        *widget,
                                                   LigmaFilterTool   *filter_tool);


G_DEFINE_TYPE (LigmaLevelsTool, ligma_levels_tool, LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_levels_tool_parent_class


void
ligma_levels_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_LEVELS_TOOL,
                LIGMA_TYPE_HISTOGRAM_OPTIONS,
                ligma_color_options_gui,
                0,
                "ligma-levels-tool",
                _("Levels"),
                _("Adjust color levels"),
                N_("_Levels..."), NULL,
                NULL, LIGMA_HELP_TOOL_LEVELS,
                LIGMA_ICON_TOOL_LEVELS,
                data);
}

static void
ligma_levels_tool_class_init (LigmaLevelsToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  object_class->finalize             = ligma_levels_tool_finalize;

  tool_class->initialize             = ligma_levels_tool_initialize;

  filter_tool_class->get_operation   = ligma_levels_tool_get_operation;
  filter_tool_class->dialog          = ligma_levels_tool_dialog;
  filter_tool_class->reset           = ligma_levels_tool_reset;
  filter_tool_class->config_notify   = ligma_levels_tool_config_notify;
  filter_tool_class->settings_import = ligma_levels_tool_settings_import;
  filter_tool_class->settings_export = ligma_levels_tool_settings_export;
  filter_tool_class->color_picked    = ligma_levels_tool_color_picked;
}

static void
ligma_levels_tool_init (LigmaLevelsTool *tool)
{
}

static void
ligma_levels_tool_finalize (GObject *object)
{
  LigmaLevelsTool *tool = LIGMA_LEVELS_TOOL (object);

  g_clear_object (&tool->histogram);
  g_clear_object (&tool->histogram_async);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_levels_tool_initialize (LigmaTool     *tool,
                             LigmaDisplay  *display,
                             GError      **error)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaLevelsTool   *l_tool      = LIGMA_LEVELS_TOOL (tool);
  LigmaImage        *image       = ligma_display_get_image (display);
  GList            *drawables;
  LigmaDrawable     *drawable;
  LigmaLevelsConfig *config;
  gdouble           scale_factor;
  gdouble           step_increment;
  gdouble           page_increment;
  gint              digits;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        ligma_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        ligma_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  config = LIGMA_LEVELS_CONFIG (filter_tool->config);

  g_clear_object (&l_tool->histogram);
  g_clear_object (&l_tool->histogram_async);
  l_tool->histogram = ligma_histogram_new (config->trc);

  l_tool->histogram_async = ligma_drawable_calculate_histogram_async
    (drawable, l_tool->histogram, FALSE);
  ligma_histogram_view_set_histogram (LIGMA_HISTOGRAM_VIEW (l_tool->histogram_view),
                                     l_tool->histogram);

  if (ligma_drawable_get_component_type (drawable) == LIGMA_COMPONENT_TYPE_U8)
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

  ligma_prop_widget_set_factor (l_tool->low_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  ligma_prop_widget_set_factor (l_tool->high_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  ligma_prop_widget_set_factor (l_tool->low_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  ligma_prop_widget_set_factor (l_tool->high_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);

  gtk_adjustment_configure (l_tool->gamma_linear,
                            scale_factor / 2.0,
                            0, scale_factor, 0.1, 1.0, 0);

  return TRUE;
}

static gchar *
ligma_levels_tool_get_operation (LigmaFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Levels"));

  return g_strdup ("ligma:levels");
}


/*******************/
/*  Levels dialog  */
/*******************/

static GtkWidget *
ligma_levels_tool_color_picker_new (LigmaLevelsTool *tool,
                                   guint           value)
{
  const gchar *icon_name;
  const gchar *help;
  gboolean     all_channels = (value & PICK_ALL_CHANNELS) != 0;

  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      icon_name = LIGMA_ICON_COLOR_PICKER_BLACK;

      if (all_channels)
        help = _("Pick black point for all channels");
      else
        help = _("Pick black point for the selected channel");
      break;

    case PICK_GAMMA:
      icon_name = LIGMA_ICON_COLOR_PICKER_GRAY;

      if (all_channels)
        help = _("Pick gray point for all channels");
      else
        help = _("Pick gray point for the selected channel");
      break;

    case PICK_HIGH_INPUT:
      icon_name = LIGMA_ICON_COLOR_PICKER_WHITE;

      if (all_channels)
        help = _("Pick white point for all channels");
      else
        help = _("Pick white point for the selected channel");
      break;

    default:
      return NULL;
    }

  return ligma_filter_tool_add_color_picker (LIGMA_FILTER_TOOL (tool),
                                            GUINT_TO_POINTER (value),
                                            icon_name,
                                            help,
                                            /* pick_abyss = */ FALSE,
                                            NULL, NULL);
}

static void
ligma_levels_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaLevelsTool   *tool         = LIGMA_LEVELS_TOOL (filter_tool);
  LigmaToolOptions  *tool_options = LIGMA_TOOL_GET_OPTIONS (filter_tool);
  LigmaLevelsConfig *config       = LIGMA_LEVELS_CONFIG (filter_tool->config);
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
                    G_CALLBACK (ligma_levels_tool_export_setup),
                    filter_tool);

  main_vbox = ligma_filter_tool_dialog_get_vbox (filter_tool);

  /*  The combo box for selecting channels  */
  main_frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, TRUE, TRUE, 0);
  gtk_widget_show (main_frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_frame_set_label_widget (GTK_FRAME (main_frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = ligma_enum_store_new_with_range (LIGMA_TYPE_HISTOGRAM_CHANNEL,
                                          LIGMA_HISTOGRAM_VALUE,
                                          LIGMA_HISTOGRAM_ALPHA);
  tool->channel_menu =
    ligma_enum_combo_box_new_with_model (LIGMA_ENUM_STORE (store));
  g_object_unref (store);

  g_object_add_weak_pointer (G_OBJECT (tool->channel_menu),
                             (gpointer) &tool->channel_menu);

  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (tool->channel_menu),
                                       "ligma-channel");
  ligma_int_combo_box_set_sensitivity (LIGMA_INT_COMBO_BOX (tool->channel_menu),
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
  hbox2 = ligma_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "ligma-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  /*  The linear/perceptual radio buttons  */
  hbox2 = ligma_prop_enum_icon_box_new (G_OBJECT (config), "trc",
                                       "ligma-color-space",
                                       -1, -1);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_show (frame_vbox);

  /*  Input levels frame  */
  frame = ligma_frame_new (_("Input Levels"));
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

  tool->histogram_view = ligma_histogram_view_new (FALSE);

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

  tool->input_bar = g_object_new (LIGMA_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->input_bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), tool->input_bar, FALSE, FALSE, 0);
  gtk_widget_show (tool->input_bar);

  bar = g_object_new (LIGMA_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), bar, FALSE, FALSE, 0);
  gtk_widget_show (bar);

  handle_bar = g_object_new (LIGMA_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox3), handle_bar, FALSE, FALSE, 0);
  gtk_widget_show (handle_bar);

  ligma_handle_bar_connect_events (LIGMA_HANDLE_BAR (handle_bar),
                                  tool->input_bar);
  ligma_handle_bar_connect_events (LIGMA_HANDLE_BAR (handle_bar),
                                  bar);

  /*  Horizontal box for input levels spinbuttons  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = ligma_levels_tool_color_picker_new (tool, PICK_LOW_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  tool->low_input_spinbutton = spinbutton =
    ligma_prop_spin_button_new (filter_tool->config, "low-input",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);

  tool->low_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (handle_bar), 0,
                                  tool->low_input);

  /*  clamp input toggle  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = ligma_prop_check_button_new (filter_tool->config, "clamp-input",
                                       _("Clamp _input"));
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

  /*  input gamma spin  */
  spinbutton = ligma_prop_spin_button_new (filter_tool->config, "gamma",
                                          0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  ligma_help_set_help_data (spinbutton, _("Gamma"), NULL);

  tool->gamma = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

  tool->gamma_linear = gtk_adjustment_new (127, 0, 255, 0.1, 1.0, 0.0);
  g_signal_connect (tool->gamma_linear, "value-changed",
                    G_CALLBACK (levels_linear_gamma_changed),
                    tool);

  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (handle_bar), 1,
                                  tool->gamma_linear);
  g_object_unref (tool->gamma_linear);

  /*  high input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = ligma_levels_tool_color_picker_new (tool, PICK_HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  spinbutton = ligma_prop_spin_button_new (filter_tool->config, "high-input",
                                          0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  tool->high_input_spinbutton = spinbutton;

  tool->high_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (handle_bar), 2,
                                  tool->high_input);

  /*  Output levels frame  */
  frame = ligma_frame_new (_("Output Levels"));
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

  tool->output_bar = g_object_new (LIGMA_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->output_bar, -1, GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->output_bar, FALSE, FALSE, 0);
  gtk_widget_show (tool->output_bar);

  handle_bar = g_object_new (LIGMA_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), handle_bar, FALSE, FALSE, 0);
  gtk_widget_show (handle_bar);

  ligma_handle_bar_connect_events (LIGMA_HANDLE_BAR (handle_bar),
                                  tool->output_bar);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low output spin  */
  tool->low_output_spinbutton = spinbutton =
    ligma_prop_spin_button_new (filter_tool->config, "low-output",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (handle_bar), 0, adjustment);

  /*  clamp output toggle  */
  button = ligma_prop_check_button_new (filter_tool->config, "clamp-output",
                                       _("Clamp outpu_t"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);

  /*  high output spin  */
  tool->high_output_spinbutton = spinbutton =
    ligma_prop_spin_button_new (filter_tool->config, "high-output",
                               0.01, 0.1, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (handle_bar), 2, adjustment);

  /*  all channels frame  */
  main_frame = ligma_frame_new (_("All Channels"));
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
  ligma_help_set_help_data (button,
                           _("Adjust levels for all channels automatically"),
                           NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_stretch_callback),
                    tool);

  button = ligma_levels_tool_color_picker_new (tool,
                                              PICK_HIGH_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ligma_levels_tool_color_picker_new (tool,
                                              PICK_GAMMA |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ligma_levels_tool_color_picker_new (tool,
                                              PICK_LOW_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ligma_icon_button_new (LIGMA_ICON_TOOL_CURVES,
                                 _("Edit these Settings as Curves"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_to_curves_callback),
                    tool);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
}

static void
ligma_levels_tool_reset (LigmaFilterTool *filter_tool)
{
  LigmaHistogramChannel channel;

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  LIGMA_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  g_object_set (filter_tool->config,
                "channel", channel,
                NULL);
}

static void
ligma_levels_tool_config_notify (LigmaFilterTool   *filter_tool,
                                LigmaConfig       *config,
                                const GParamSpec *pspec)
{
  LigmaLevelsTool   *levels_tool   = LIGMA_LEVELS_TOOL (filter_tool);
  LigmaLevelsConfig *levels_config = LIGMA_LEVELS_CONFIG (config);

  LIGMA_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! levels_tool->channel_menu ||
      ! levels_tool->histogram_view)
    return;

  if (! strcmp (pspec->name, "trc"))
    {
      g_clear_object (&levels_tool->histogram);
      g_clear_object (&levels_tool->histogram_async);
      levels_tool->histogram = ligma_histogram_new (levels_config->trc);

      levels_tool->histogram_async = ligma_drawable_calculate_histogram_async
        (LIGMA_TOOL (filter_tool)->drawables->data, levels_tool->histogram, FALSE);
      ligma_histogram_view_set_histogram (LIGMA_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                         levels_tool->histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      ligma_histogram_view_set_channel (LIGMA_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                       levels_config->channel);
      ligma_color_bar_set_channel (LIGMA_COLOR_BAR (levels_tool->output_bar),
                                  levels_config->channel);
      ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (levels_tool->channel_menu),
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
ligma_levels_tool_settings_import (LigmaFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  LigmaLevelsConfig *config = LIGMA_LEVELS_CONFIG (filter_tool->config);
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

  if (g_str_has_prefix (header, "# LIGMA Levels File\n"))
    return ligma_levels_config_load_cruft (config, input, error);

  return LIGMA_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
ligma_levels_tool_settings_export (LigmaFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  LigmaLevelsTool   *tool   = LIGMA_LEVELS_TOOL (filter_tool);
  LigmaLevelsConfig *config = LIGMA_LEVELS_CONFIG (filter_tool->config);

  if (tool->export_old_format)
    return ligma_levels_config_save_cruft (config, output, error);

  return LIGMA_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
}

static void
levels_input_adjust_by_color (LigmaLevelsConfig     *config,
                              guint                 value,
                              LigmaHistogramChannel  channel,
                              const LigmaRGB        *color)
{
  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      ligma_levels_config_adjust_by_colors (config, channel, color, NULL, NULL);
      break;
    case PICK_GAMMA:
      ligma_levels_config_adjust_by_colors (config, channel, NULL, color, NULL);
      break;
    case PICK_HIGH_INPUT:
      ligma_levels_config_adjust_by_colors (config, channel, NULL, NULL, color);
      break;
    default:
      break;
    }
}

static void
ligma_levels_tool_color_picked (LigmaFilterTool *color_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               const LigmaRGB  *color)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (color_tool);
  LigmaLevelsConfig *config      = LIGMA_LEVELS_CONFIG (filter_tool->config);
  LigmaRGB           rgb         = *color;
  guint             value       = GPOINTER_TO_UINT (identifier);

  if (config->trc == LIGMA_TRC_LINEAR)
    babl_process (babl_fish (babl_format ("R'G'B'A double"),
                             babl_format ("RGBA double")),
                  &rgb, &rgb, 1);

  if (value & PICK_ALL_CHANNELS &&
      ligma_babl_format_get_base_type (sample_format) == LIGMA_RGB)
    {
      LigmaHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
        {
        case PICK_LOW_INPUT:
          config->low_input[LIGMA_HISTOGRAM_VALUE] = 0.0;
          break;
        case PICK_GAMMA:
          config->gamma[LIGMA_HISTOGRAM_VALUE] = 1.0;
          break;
        case PICK_HIGH_INPUT:
          config->high_input[LIGMA_HISTOGRAM_VALUE] = 1.0;
          break;
        default:
          break;
        }

      /*  then adjust all color channels  */
      for (channel = LIGMA_HISTOGRAM_RED;
           channel <= LIGMA_HISTOGRAM_BLUE;
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
ligma_levels_tool_export_setup (LigmaSettingsBox      *settings_box,
                               GtkFileChooserDialog *dialog,
                               gboolean              export,
                               LigmaLevelsTool       *tool)
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
                    G_CALLBACK (ligma_toggle_button_update),
                    &tool->export_old_format);
}

static void
levels_update_input_bar (LigmaLevelsTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaLevelsConfig *config      = LIGMA_LEVELS_CONFIG (filter_tool->config);

  switch (config->channel)
    {
      gdouble value;

    case LIGMA_HISTOGRAM_VALUE:
    case LIGMA_HISTOGRAM_ALPHA:
    case LIGMA_HISTOGRAM_RGB:
    case LIGMA_HISTOGRAM_LUMINANCE:
      {
        guchar v[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            value = ligma_operation_levels_map_input (config,
                                                     config->channel,
                                                     i / 255.0);
            v[i] = CLAMP (value, 0.0, 1.0) * 255.999;
          }

        ligma_color_bar_set_buffers (LIGMA_COLOR_BAR (tool->input_bar),
                                    v, v, v);
      }
      break;

    case LIGMA_HISTOGRAM_RED:
    case LIGMA_HISTOGRAM_GREEN:
    case LIGMA_HISTOGRAM_BLUE:
      {
        guchar r[256];
        guchar g[256];
        guchar b[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            value = ligma_operation_levels_map_input (config,
                                                     LIGMA_HISTOGRAM_RED,
                                                     i / 255.0);
            r[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = ligma_operation_levels_map_input (config,
                                                     LIGMA_HISTOGRAM_GREEN,
                                                     i / 255.0);
            g[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = ligma_operation_levels_map_input (config,
                                                     LIGMA_HISTOGRAM_BLUE,
                                                     i / 255.0);
            b[i] = CLAMP (value, 0.0, 1.0) * 255.999;
          }

        ligma_color_bar_set_buffers (LIGMA_COLOR_BAR (tool->input_bar),
                                    r, g, b);
      }
      break;
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
                         LigmaFilterTool *filter_tool)
{
  LigmaLevelsConfig *config = LIGMA_LEVELS_CONFIG (filter_tool->config);
  gint              value;

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &value) &&
      config->channel != value)
    {
      g_object_set (config,
                    "channel", value,
                    NULL);
    }
}

static void
levels_channel_reset_callback (GtkWidget      *widget,
                               LigmaFilterTool *filter_tool)
{
  ligma_levels_config_reset_channel (LIGMA_LEVELS_CONFIG (filter_tool->config));
}

static gboolean
levels_menu_sensitivity (gint      value,
                         gpointer  data)
{
  LigmaDrawable         *drawable;
  LigmaHistogramChannel  channel  = value;

  if (! LIGMA_TOOL (data)->drawables)
    return FALSE;

  drawable = LIGMA_TOOL (data)->drawables->data;

  switch (channel)
    {
    case LIGMA_HISTOGRAM_VALUE:
      return TRUE;

    case LIGMA_HISTOGRAM_RED:
    case LIGMA_HISTOGRAM_GREEN:
    case LIGMA_HISTOGRAM_BLUE:
      return ligma_drawable_is_rgb (drawable);

    case LIGMA_HISTOGRAM_ALPHA:
      return ligma_drawable_has_alpha (drawable);

    case LIGMA_HISTOGRAM_RGB:
      return FALSE;

    case LIGMA_HISTOGRAM_LUMINANCE:
      return FALSE;
    }

  return FALSE;
}

static void
levels_stretch_callback (GtkWidget      *widget,
                         LigmaLevelsTool *levels_tool)
{
  LigmaTool       *tool        = LIGMA_TOOL (levels_tool);
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (levels_tool);
  LigmaWaitable   *waitable;

  waitable = ligma_trivially_cancelable_waitable_new (
    LIGMA_WAITABLE (levels_tool->histogram_async));

  ligma_wait (tool->tool_info->ligma, waitable, _("Calculating histogram..."));

  g_object_unref (waitable);

  if (ligma_async_is_synced   (levels_tool->histogram_async) &&
      ligma_async_is_finished (levels_tool->histogram_async))
    {
      ligma_levels_config_stretch (LIGMA_LEVELS_CONFIG (filter_tool->config),
                                  levels_tool->histogram,
                                  ligma_drawable_is_rgb (tool->drawables->data));
    }
}

static void
levels_linear_gamma_changed (GtkAdjustment  *adjustment,
                             LigmaLevelsTool *tool)
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
                           LigmaFilterTool *filter_tool)
{
  LigmaLevelsConfig *config = LIGMA_LEVELS_CONFIG (filter_tool->config);
  LigmaCurvesConfig *curves;

  curves = ligma_levels_config_to_curves_config (config);

  ligma_filter_tool_edit_as (filter_tool,
                            "ligma-curves-tool",
                            LIGMA_CONFIG (curves));

  g_object_unref (curves);
}
