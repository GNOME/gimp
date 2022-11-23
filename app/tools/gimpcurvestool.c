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

#include <errno.h>

#include <glib/gstdio.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "operations/ligmacurvesconfig.h"
#include "operations/ligmaoperationcurves.h"

#include "core/ligma.h"
#include "core/ligmacurve.h"
#include "core/ligmacurve-map.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-histogram.h"
#include "core/ligmaerror.h"
#include "core/ligmahistogram.h"
#include "core/ligmaimage.h"

#include "widgets/ligmacolorbar.h"
#include "widgets/ligmacurveview.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"

#include "ligmacurvestool.h"
#include "ligmahistogramoptions.h"

#include "ligma-intl.h"


#define GRAPH_SIZE 256
#define BAR_SIZE    12
#define RADIUS       4


/*  local function prototypes  */

static gboolean   ligma_curves_tool_initialize      (LigmaTool             *tool,
                                                    LigmaDisplay          *display,
                                                    GError              **error);
static void       ligma_curves_tool_button_release  (LigmaTool             *tool,
                                                    const LigmaCoords     *coords,
                                                    guint32               time,
                                                    GdkModifierType       state,
                                                    LigmaButtonReleaseType release_type,
                                                    LigmaDisplay          *display);
static gboolean   ligma_curves_tool_key_press       (LigmaTool             *tool,
                                                    GdkEventKey          *kevent,
                                                    LigmaDisplay          *display);
static void       ligma_curves_tool_oper_update     (LigmaTool             *tool,
                                                    const LigmaCoords     *coords,
                                                    GdkModifierType       state,
                                                    gboolean              proximity,
                                                    LigmaDisplay          *display);

static gchar    * ligma_curves_tool_get_operation   (LigmaFilterTool       *filter_tool,
                                                    gchar               **description);
static void       ligma_curves_tool_dialog          (LigmaFilterTool       *filter_tool);
static void       ligma_curves_tool_reset           (LigmaFilterTool       *filter_tool);
static void       ligma_curves_tool_config_notify   (LigmaFilterTool       *filter_tool,
                                                    LigmaConfig           *config,
                                                    const GParamSpec     *pspec);
static gboolean   ligma_curves_tool_settings_import (LigmaFilterTool       *filter_tool,
                                                    GInputStream         *input,
                                                    GError              **error);
static gboolean   ligma_curves_tool_settings_export (LigmaFilterTool       *filter_tool,
                                                    GOutputStream        *output,
                                                    GError              **error);
static void       ligma_curves_tool_color_picked    (LigmaFilterTool       *filter_tool,
                                                    gpointer              identifier,
                                                    gdouble               x,
                                                    gdouble               y,
                                                    const Babl           *sample_format,
                                                    const LigmaRGB        *color);

static void       ligma_curves_tool_export_setup    (LigmaSettingsBox      *settings_box,
                                                    GtkFileChooserDialog *dialog,
                                                    gboolean              export,
                                                    LigmaCurvesTool       *tool);
static void       ligma_curves_tool_update_channel  (LigmaCurvesTool       *tool);
static void       ligma_curves_tool_update_point    (LigmaCurvesTool       *tool);

static void       curves_curve_dirty_callback      (LigmaCurve            *curve,
                                                    LigmaCurvesTool       *tool);

static void       curves_channel_callback          (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);
static void       curves_channel_reset_callback    (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);

static gboolean   curves_menu_sensitivity          (gint                  value,
                                                    gpointer              data);

static void       curves_graph_selection_callback  (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);

static void       curves_point_coords_callback     (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);
static void       curves_point_type_callback       (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);

static void       curves_curve_type_callback       (GtkWidget            *widget,
                                                    LigmaCurvesTool       *tool);

static gboolean   curves_get_channel_color         (GtkWidget            *widget,
                                                    LigmaHistogramChannel  channel,
                                                    LigmaRGB              *color);


G_DEFINE_TYPE (LigmaCurvesTool, ligma_curves_tool, LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_curves_tool_parent_class


/*  public functions  */

void
ligma_curves_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_CURVES_TOOL,
                LIGMA_TYPE_HISTOGRAM_OPTIONS,
                ligma_color_options_gui,
                0,
                "ligma-curves-tool",
                _("Curves"),
                _("Adjust color curves"),
                N_("_Curves..."), NULL,
                NULL, LIGMA_HELP_TOOL_CURVES,
                LIGMA_ICON_TOOL_CURVES,
                data);
}


/*  private functions  */

static void
ligma_curves_tool_class_init (LigmaCurvesToolClass *klass)
{
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  tool_class->initialize             = ligma_curves_tool_initialize;
  tool_class->button_release         = ligma_curves_tool_button_release;
  tool_class->key_press              = ligma_curves_tool_key_press;
  tool_class->oper_update            = ligma_curves_tool_oper_update;

  filter_tool_class->get_operation   = ligma_curves_tool_get_operation;
  filter_tool_class->dialog          = ligma_curves_tool_dialog;
  filter_tool_class->reset           = ligma_curves_tool_reset;
  filter_tool_class->config_notify   = ligma_curves_tool_config_notify;
  filter_tool_class->settings_import = ligma_curves_tool_settings_import;
  filter_tool_class->settings_export = ligma_curves_tool_settings_export;
  filter_tool_class->color_picked    = ligma_curves_tool_color_picked;
}

static void
ligma_curves_tool_init (LigmaCurvesTool *tool)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (tool->picked_color); i++)
    tool->picked_color[i] = -1.0;
}

static gboolean
ligma_curves_tool_initialize (LigmaTool     *tool,
                             LigmaDisplay  *display,
                             GError      **error)
{
  LigmaFilterTool       *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesTool       *c_tool      = LIGMA_CURVES_TOOL (tool);
  LigmaImage            *image       = ligma_display_get_image (display);
  GList                *drawables;
  LigmaDrawable         *drawable;
  LigmaCurvesConfig     *config;
  LigmaHistogram        *histogram;
  LigmaHistogramChannel  channel;

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

  config = LIGMA_CURVES_CONFIG (filter_tool->config);

  histogram = ligma_histogram_new (config->trc);
  g_object_unref (ligma_drawable_calculate_histogram_async (drawable, histogram,
                                                           FALSE));
  ligma_histogram_view_set_background (LIGMA_HISTOGRAM_VIEW (c_tool->graph),
                                      histogram);
  g_object_unref (histogram);

  if (ligma_drawable_get_component_type (drawable) == LIGMA_COMPONENT_TYPE_U8)
    {
      c_tool->scale = 255.0;

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_input),  0);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_output), 0);

      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_input),  3);
      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_output), 3);
    }
  else
    {
      c_tool->scale = 100.0;

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_input),  2);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_output), 2);

      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_input),  6);
      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_output), 6);
    }

  ligma_curve_view_set_range_x (LIGMA_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);
  ligma_curve_view_set_range_y (LIGMA_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (c_tool->point_output),
                             0, c_tool->scale);

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      g_signal_connect (config->curve[channel], "dirty",
                        G_CALLBACK (curves_curve_dirty_callback),
                        tool);
    }

  ligma_curves_tool_update_point (c_tool);

  /*  always pick colors  */
  ligma_filter_tool_enable_color_picking (filter_tool, NULL, FALSE);

  return TRUE;
}

static void
ligma_curves_tool_button_release (LigmaTool              *tool,
                                 const LigmaCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 LigmaButtonReleaseType  release_type,
                                 LigmaDisplay           *display)
{
  LigmaCurvesTool   *c_tool      = LIGMA_CURVES_TOOL (tool);
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);

  if (state & ligma_get_extend_selection_mask ())
    {
      LigmaCurve *curve = config->curve[config->channel];
      gdouble    value = c_tool->picked_color[config->channel];
      gint       point;

      point = ligma_curve_get_point_at (curve, value);

      if (point < 0)
        {
          LigmaCurvePointType type = LIGMA_CURVE_POINT_SMOOTH;

          point = ligma_curve_view_get_selected (
            LIGMA_CURVE_VIEW (c_tool->graph));

          if (point >= 0)
            type = ligma_curve_get_point_type (curve, point);

          point = ligma_curve_add_point (
            curve,
            value, ligma_curve_map_value (curve, value));

          ligma_curve_set_point_type (curve, point, type);
        }

      ligma_curve_view_set_selected (LIGMA_CURVE_VIEW (c_tool->graph), point);
    }
  else if (state & ligma_get_toggle_behavior_mask ())
    {
      LigmaHistogramChannel channel;
      LigmaCurvePointType   type = LIGMA_CURVE_POINT_SMOOTH;
      gint                 point;

      point = ligma_curve_view_get_selected (LIGMA_CURVE_VIEW (c_tool->graph));

      if (point >= 0)
        {
          type = ligma_curve_get_point_type (config->curve[config->channel],
                                            point);
        }

      for (channel = LIGMA_HISTOGRAM_VALUE;
           channel <= LIGMA_HISTOGRAM_ALPHA;
           channel++)
        {
          LigmaCurve *curve = config->curve[channel];
          gdouble    value = c_tool->picked_color[channel];

          if (value != -1)
            {
              point = ligma_curve_get_point_at (curve, value);

              if (point < 0)
                {
                  point = ligma_curve_add_point (
                    curve,
                    value, ligma_curve_map_value (curve, value));

                  ligma_curve_set_point_type (curve, point, type);
                }

              if (channel == config->channel)
                {
                  ligma_curve_view_set_selected (LIGMA_CURVE_VIEW (c_tool->graph),
                                                point);
                }
            }
        }
    }

  /*  chain up to halt the tool */
  LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static gboolean
ligma_curves_tool_key_press (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  LigmaCurvesTool *c_tool = LIGMA_CURVES_TOOL (tool);

  if (tool->display && c_tool->graph)
    {
      if (gtk_widget_event (c_tool->graph, (GdkEvent *) kevent))
        return TRUE;
    }

  return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
ligma_curves_tool_oper_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              LigmaDisplay      *display)
{
  if (ligma_filter_tool_on_guide (LIGMA_FILTER_TOOL (tool),
                                 coords, display))
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                                   display);
    }
  else
    {
      LigmaColorPickTarget  target;
      gchar               *status      = NULL;
      GdkModifierType      extend_mask = ligma_get_extend_selection_mask ();
      GdkModifierType      toggle_mask = ligma_get_toggle_behavior_mask ();

      ligma_tool_pop_status (tool, display);

      if (state & extend_mask)
        {
          target = LIGMA_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add a control point"));
        }
      else if (state & toggle_mask)
        {
          target = LIGMA_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add control points to all channels"));
        }
      else
        {
          target = LIGMA_COLOR_PICK_TARGET_NONE;
          status = ligma_suggest_modifiers (_("Click to locate on curve"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: add control point"),
                                           _("%s: add control points to all channels"),
                                           NULL);
        }

      LIGMA_COLOR_TOOL (tool)->pick_target = target;

      if (proximity)
        ligma_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
}

static gchar *
ligma_curves_tool_get_operation (LigmaFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Curves"));

  return g_strdup ("ligma:curves");
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
ligma_curves_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaCurvesTool   *tool         = LIGMA_CURVES_TOOL (filter_tool);
  LigmaToolOptions  *tool_options = LIGMA_TOOL_GET_OPTIONS (filter_tool);
  LigmaCurvesConfig *config       = LIGMA_CURVES_CONFIG (filter_tool->config);
  GtkListStore     *store;
  GtkWidget        *main_vbox;
  GtkWidget        *frame_vbox;
  GtkWidget        *vbox;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *main_frame;
  GtkWidget        *frame;
  GtkWidget        *grid;
  GtkWidget        *button;
  GtkWidget        *bar;
  GtkWidget        *combo;

  g_signal_connect (filter_tool->settings_box, "file-dialog-setup",
                    G_CALLBACK (ligma_curves_tool_export_setup),
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

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (tool->channel_menu),
                                       "ligma-channel");
  ligma_int_combo_box_set_sensitivity (LIGMA_INT_COMBO_BOX (tool->channel_menu),
                                      curves_menu_sensitivity, filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tool->channel_menu, FALSE, FALSE, 0);
  gtk_widget_show (tool->channel_menu);

  g_signal_connect (tool->channel_menu, "changed",
                    G_CALLBACK (curves_channel_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->channel_menu);

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (curves_channel_reset_callback),
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

  /*  The grid for the color bars and the graph  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (frame_vbox), grid, TRUE, TRUE, 0);

  /*  The left color bar  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_grid_attach (GTK_GRID (grid), vbox, 0, 0, 1, 1);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, RADIUS);
  gtk_widget_show (frame);

  tool->yrange = ligma_color_bar_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request (tool->yrange, BAR_SIZE, -1);
  gtk_container_add (GTK_CONTAINER (frame), tool->yrange);
  gtk_widget_show (tool->yrange);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_widget_set_vexpand (frame, TRUE);
  gtk_grid_attach (GTK_GRID (grid), frame, 1, 0, 1, 1);
  gtk_widget_show (frame);

  tool->graph = ligma_curve_view_new ();

  g_object_add_weak_pointer (G_OBJECT (tool->graph),
                             (gpointer) &tool->graph);

  ligma_curve_view_set_range_x (LIGMA_CURVE_VIEW (tool->graph), 0, 255);
  ligma_curve_view_set_range_y (LIGMA_CURVE_VIEW (tool->graph), 0, 255);
  gtk_widget_set_size_request (tool->graph,
                               GRAPH_SIZE + RADIUS * 2,
                               GRAPH_SIZE + RADIUS * 2);
  g_object_set (tool->graph,
                "border-width", RADIUS,
                "subdivisions", 1,
                NULL);
  gtk_container_add (GTK_CONTAINER (frame), tool->graph);
  gtk_widget_show (tool->graph);

  g_object_bind_property (G_OBJECT (tool_options), "histogram-scale",
                          G_OBJECT (tool->graph),  "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_signal_connect (tool->graph, "selection-changed",
                    G_CALLBACK (curves_graph_selection_callback),
                    tool);

  /*  The bottom color bar  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox2, 1, 1, 1, 1);
  gtk_widget_show (hbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, RADIUS);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  tool->xrange = ligma_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (tool->xrange, -1, BAR_SIZE / 2);
  gtk_box_pack_start (GTK_BOX (vbox), tool->xrange, TRUE, TRUE, 0);
  gtk_widget_show (tool->xrange);

  bar = ligma_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), bar, TRUE, TRUE, 0);
  gtk_widget_show (bar);

  gtk_widget_show (grid);

  /*  The point properties box  */
  tool->point_box = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_box);

  label = gtk_label_new_with_mnemonic (_("_Input:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  tool->point_input = ligma_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_input, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_input);

  g_signal_connect (tool->point_input, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_input);

  label = gtk_label_new_with_mnemonic (_("O_utput:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  tool->point_output = ligma_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_output, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_output);

  g_signal_connect (tool->point_output, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_output);

  label = gtk_label_new_with_mnemonic (_("T_ype:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  hbox2 = ligma_enum_icon_box_new (LIGMA_TYPE_CURVE_POINT_TYPE,
                                  "ligma-curve-point",
                                  GTK_ICON_SIZE_MENU,
                                  G_CALLBACK (curves_point_type_callback),
                                  tool, NULL,
                                  &tool->point_type);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_type);

  label = gtk_label_new_with_mnemonic (_("Curve _type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  The curve-type combo  */
  tool->curve_type = combo = ligma_enum_combo_box_new (LIGMA_TYPE_CURVE_TYPE);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (combo),
                                       "ligma-curve");
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo), 0,
                              G_CALLBACK (curves_curve_type_callback),
                              tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  ligma_curves_tool_update_channel (tool);
}

static void
ligma_curves_tool_reset (LigmaFilterTool *filter_tool)
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
ligma_curves_tool_config_notify (LigmaFilterTool   *filter_tool,
                                LigmaConfig       *config,
                                const GParamSpec *pspec)
{
  LigmaCurvesTool   *curves_tool   = LIGMA_CURVES_TOOL (filter_tool);
  LigmaCurvesConfig *curves_config = LIGMA_CURVES_CONFIG (config);
  LigmaCurve        *curve         = curves_config->curve[curves_config->channel];

  LIGMA_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! curves_tool->channel_menu ||
      ! curves_tool->graph)
    return;

  if (! strcmp (pspec->name, "trc"))
    {
      LigmaHistogram *histogram;

      histogram = ligma_histogram_new (curves_config->trc);
      g_object_unref (ligma_drawable_calculate_histogram_async
                      (LIGMA_TOOL (filter_tool)->drawables->data, histogram, FALSE));
      ligma_histogram_view_set_background (LIGMA_HISTOGRAM_VIEW (curves_tool->graph),
                                          histogram);
      g_object_unref (histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      ligma_curves_tool_update_channel (LIGMA_CURVES_TOOL (filter_tool));
    }
  else if (! strcmp (pspec->name, "curve"))
    {
      ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (curves_tool->curve_type),
                                     curve->curve_type);
    }
}

static gboolean
ligma_curves_tool_settings_import (LigmaFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  LigmaCurvesConfig *config = LIGMA_CURVES_CONFIG (filter_tool->config);
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

  if (g_str_has_prefix (header, "# LIGMA Curves File\n"))
    return ligma_curves_config_load_cruft (config, input, error);

  return LIGMA_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
ligma_curves_tool_settings_export (LigmaFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  LigmaCurvesTool   *tool   = LIGMA_CURVES_TOOL (filter_tool);
  LigmaCurvesConfig *config = LIGMA_CURVES_CONFIG (filter_tool->config);

  if (tool->export_old_format)
    return ligma_curves_config_save_cruft (config, output, error);

  return LIGMA_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
}

static void
ligma_curves_tool_color_picked (LigmaFilterTool *filter_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               const LigmaRGB  *color)
{
  LigmaCurvesTool   *tool     = LIGMA_CURVES_TOOL (filter_tool);
  LigmaCurvesConfig *config   = LIGMA_CURVES_CONFIG (filter_tool->config);
  LigmaDrawable     *drawable = LIGMA_TOOL (tool)->drawables->data;
  LigmaRGB           rgb      = *color;

  if (config->trc == LIGMA_TRC_LINEAR)
    babl_process (babl_fish (babl_format ("R'G'B'A double"),
                             babl_format ("RGBA double")),
                  &rgb, &rgb, 1);

  tool->picked_color[LIGMA_HISTOGRAM_RED]   = rgb.r;
  tool->picked_color[LIGMA_HISTOGRAM_GREEN] = rgb.g;
  tool->picked_color[LIGMA_HISTOGRAM_BLUE]  = rgb.b;

  if (ligma_drawable_has_alpha (drawable))
    tool->picked_color[LIGMA_HISTOGRAM_ALPHA] = rgb.a;
  else
    tool->picked_color[LIGMA_HISTOGRAM_ALPHA] = -1;

  tool->picked_color[LIGMA_HISTOGRAM_VALUE] = MAX (MAX (rgb.r, rgb.g), rgb.b);

  ligma_curve_view_set_xpos (LIGMA_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);
}

static void
ligma_curves_tool_export_setup (LigmaSettingsBox      *settings_box,
                               GtkFileChooserDialog *dialog,
                               gboolean              export,
                               LigmaCurvesTool       *tool)
{
  GtkWidget *button;

  if (! export)
    return;

  button = gtk_check_button_new_with_mnemonic (_("Use _old curves file format"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                tool->export_old_format);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &tool->export_old_format);
}

static void
ligma_curves_tool_update_channel (LigmaCurvesTool *tool)
{
  LigmaFilterTool       *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig     *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
  LigmaCurve            *curve       = config->curve[config->channel];
  LigmaHistogramChannel  channel;

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);

  switch (config->channel)
    {
      guchar r[256];
      guchar g[256];
      guchar b[256];

    case LIGMA_HISTOGRAM_VALUE:
    case LIGMA_HISTOGRAM_ALPHA:
    case LIGMA_HISTOGRAM_RGB:
    case LIGMA_HISTOGRAM_LUMINANCE:
      ligma_curve_get_uchar (curve, sizeof (r), r);

      ligma_color_bar_set_buffers (LIGMA_COLOR_BAR (tool->xrange),
                                  r, r, r);
      break;

    case LIGMA_HISTOGRAM_RED:
    case LIGMA_HISTOGRAM_GREEN:
    case LIGMA_HISTOGRAM_BLUE:
      ligma_curve_get_uchar (config->curve[LIGMA_HISTOGRAM_RED],
                            sizeof (r), r);
      ligma_curve_get_uchar (config->curve[LIGMA_HISTOGRAM_GREEN],
                            sizeof (g), g);
      ligma_curve_get_uchar (config->curve[LIGMA_HISTOGRAM_BLUE],
                            sizeof (b), b);

      ligma_color_bar_set_buffers (LIGMA_COLOR_BAR (tool->xrange),
                                  r, g, b);
      break;
    }

  ligma_histogram_view_set_channel (LIGMA_HISTOGRAM_VIEW (tool->graph),
                                   config->channel);
  ligma_curve_view_set_xpos (LIGMA_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);

  ligma_color_bar_set_channel (LIGMA_COLOR_BAR (tool->yrange),
                              config->channel);

  ligma_curve_view_remove_all_backgrounds (LIGMA_CURVE_VIEW (tool->graph));

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      LigmaRGB  curve_color;
      gboolean has_color;

      has_color = curves_get_channel_color (tool->graph, channel, &curve_color);

      if (channel == config->channel)
        {
          ligma_curve_view_set_curve (LIGMA_CURVE_VIEW (tool->graph), curve,
                                     has_color ? &curve_color : NULL);
        }
      else
        {
          ligma_curve_view_add_background (LIGMA_CURVE_VIEW (tool->graph),
                                          config->curve[channel],
                                          has_color ? &curve_color : NULL);
        }
    }

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (tool->curve_type),
                                 curve->curve_type);

  ligma_curves_tool_update_point (tool);
}

static void
ligma_curves_tool_update_point (LigmaCurvesTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
  LigmaCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = ligma_curve_view_get_selected (LIGMA_CURVE_VIEW (tool->graph));

  gtk_widget_set_sensitive (tool->point_box, point >= 0);

  if (point >= 0)
    {
      gdouble min = 0.0;
      gdouble max = 1.0;
      gdouble x;
      gdouble y;

      if (point > 0)
        ligma_curve_get_point (curve, point - 1, &min, NULL);

      if (point < ligma_curve_get_n_points (curve) - 1)
        ligma_curve_get_point (curve, point + 1, &max, NULL);

      ligma_curve_get_point (curve, point, &x, &y);

      x   *= tool->scale;
      y   *= tool->scale;
      min *= tool->scale;
      max *= tool->scale;

      g_signal_handlers_block_by_func (tool->point_input,
                                       curves_point_coords_callback,
                                       tool);
      g_signal_handlers_block_by_func (tool->point_output,
                                       curves_point_coords_callback,
                                       tool);

      gtk_spin_button_set_range (GTK_SPIN_BUTTON (tool->point_input), min, max);

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->point_input),  x);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->point_output), y);


      g_signal_handlers_unblock_by_func (tool->point_input,
                                         curves_point_coords_callback,
                                         tool);
      g_signal_handlers_unblock_by_func (tool->point_output,
                                         curves_point_coords_callback,
                                         tool);

      g_signal_handlers_block_by_func (tool->point_type,
                                       curves_point_type_callback,
                                       tool);

      ligma_int_radio_group_set_active (
        GTK_RADIO_BUTTON (tool->point_type),
        ligma_curve_get_point_type (curve, point));

      g_signal_handlers_unblock_by_func (tool->point_type,
                                         curves_point_type_callback,
                                         tool);
    }
}

static void
curves_curve_dirty_callback (LigmaCurve      *curve,
                             LigmaCurvesTool *tool)
{
  if (tool->graph &&
      ligma_curve_view_get_curve (LIGMA_CURVE_VIEW (tool->graph)) == curve)
    {
      ligma_curves_tool_update_point (tool);
    }
}

static void
curves_channel_callback (GtkWidget      *widget,
                         LigmaCurvesTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
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
curves_channel_reset_callback (GtkWidget      *widget,
                               LigmaCurvesTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);

  ligma_curve_reset (config->curve[config->channel], FALSE);
}

static gboolean
curves_menu_sensitivity (gint      value,
                         gpointer  data)
{
  LigmaDrawable         *drawable = LIGMA_TOOL (data)->drawables->data;
  LigmaHistogramChannel  channel  = value;

  if (!drawable)
    return FALSE;

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
curves_graph_selection_callback (GtkWidget      *widget,
                                 LigmaCurvesTool *tool)
{
  ligma_curves_tool_update_point (tool);
}

static void
curves_point_coords_callback (GtkWidget      *widget,
                              LigmaCurvesTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
  LigmaCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = ligma_curve_view_get_selected (LIGMA_CURVE_VIEW (tool->graph));

  if (point >= 0)
    {
      gdouble x;
      gdouble y;

      x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_input));
      y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_output));

      x /= tool->scale;
      y /= tool->scale;

      ligma_curve_set_point (curve, point, x, y);
    }
}

static void
curves_point_type_callback (GtkWidget      *widget,
                            LigmaCurvesTool *tool)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
  LigmaCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = ligma_curve_view_get_selected (LIGMA_CURVE_VIEW (tool->graph));

  if (point >= 0 && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      LigmaCurvePointType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                 "ligma-item-data"));

      ligma_curve_set_point_type (curve, point, type);
    }
}

static void
curves_curve_type_callback (GtkWidget      *widget,
                            LigmaCurvesTool *tool)
{
  gint value;

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &value))
    {
      LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (tool);
      LigmaCurvesConfig *config      = LIGMA_CURVES_CONFIG (filter_tool->config);
      LigmaCurveType     curve_type  = value;

      if (config->curve[config->channel]->curve_type != curve_type)
        ligma_curve_set_curve_type (config->curve[config->channel], curve_type);
    }
}

static gboolean
curves_get_channel_color (GtkWidget            *widget,
                          LigmaHistogramChannel  channel,
                          LigmaRGB              *color)
{
  static const LigmaRGB channel_colors[LIGMA_HISTOGRAM_RGB] =
  {
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.5, 0.5, 0.5, 1.0 }
  };

  GdkRGBA rgba;

  if (channel == LIGMA_HISTOGRAM_VALUE)
    return FALSE;

  if (channel == LIGMA_HISTOGRAM_ALPHA)
    {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      gdouble          lum;

      gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                                   &rgba);

      lum = LIGMA_RGB_LUMINANCE (rgba.red, rgba.green, rgba.blue);

      if (lum > 0.5)
        {
          ligma_rgba_set (color, lum - 0.3, lum - 0.3, lum - 0.3, 1.0);
        }
      else
        {
          ligma_rgba_set (color, lum + 0.3, lum + 0.3, lum + 0.3, 1.0);
        }

      return TRUE;
    }

  *color = channel_colors[channel];

  return TRUE;
}
