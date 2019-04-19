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

#include <errno.h>

#include <glib/gstdio.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimpcurvesconfig.h"
#include "operations/gimpoperationcurves.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimperror.h"
#include "core/gimphistogram.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimpcurveview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpcurvestool.h"
#include "gimphistogramoptions.h"

#include "gimp-intl.h"


#define GRAPH_SIZE 256
#define BAR_SIZE    12
#define RADIUS       4


/*  local function prototypes  */

static gboolean   gimp_curves_tool_initialize      (GimpTool             *tool,
                                                    GimpDisplay          *display,
                                                    GError              **error);
static void       gimp_curves_tool_button_release  (GimpTool             *tool,
                                                    const GimpCoords     *coords,
                                                    guint32               time,
                                                    GdkModifierType       state,
                                                    GimpButtonReleaseType release_type,
                                                    GimpDisplay          *display);
static gboolean   gimp_curves_tool_key_press       (GimpTool             *tool,
                                                    GdkEventKey          *kevent,
                                                    GimpDisplay          *display);
static void       gimp_curves_tool_oper_update     (GimpTool             *tool,
                                                    const GimpCoords     *coords,
                                                    GdkModifierType       state,
                                                    gboolean              proximity,
                                                    GimpDisplay          *display);

static gchar    * gimp_curves_tool_get_operation   (GimpFilterTool       *filter_tool,
                                                    gchar               **description);
static void       gimp_curves_tool_dialog          (GimpFilterTool       *filter_tool);
static void       gimp_curves_tool_reset           (GimpFilterTool       *filter_tool);
static void       gimp_curves_tool_config_notify   (GimpFilterTool       *filter_tool,
                                                    GimpConfig           *config,
                                                    const GParamSpec     *pspec);
static gboolean   gimp_curves_tool_settings_import (GimpFilterTool       *filter_tool,
                                                    GInputStream         *input,
                                                    GError              **error);
static gboolean   gimp_curves_tool_settings_export (GimpFilterTool       *filter_tool,
                                                    GOutputStream        *output,
                                                    GError              **error);
static void       gimp_curves_tool_color_picked    (GimpFilterTool       *filter_tool,
                                                    gpointer              identifier,
                                                    gdouble               x,
                                                    gdouble               y,
                                                    const Babl           *sample_format,
                                                    const GimpRGB        *color);

static void       gimp_curves_tool_export_setup    (GimpSettingsBox      *settings_box,
                                                    GtkFileChooserDialog *dialog,
                                                    gboolean              export,
                                                    GimpCurvesTool       *tool);
static void       gimp_curves_tool_update_channel  (GimpCurvesTool       *tool);
static void       gimp_curves_tool_update_point    (GimpCurvesTool       *tool);

static void       curves_curve_dirty_callback      (GimpCurve            *curve,
                                                    GimpCurvesTool       *tool);

static void       curves_channel_callback          (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);
static void       curves_channel_reset_callback    (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);

static gboolean   curves_menu_sensitivity          (gint                  value,
                                                    gpointer              data);

static void       curves_graph_selection_callback  (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);

static void       curves_point_coords_callback     (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);
static void       curves_point_type_callback       (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);

static void       curves_curve_type_callback       (GtkWidget            *widget,
                                                    GimpCurvesTool       *tool);

static gboolean   curves_get_channel_color         (GtkWidget            *widget,
                                                    GimpHistogramChannel  channel,
                                                    GimpRGB              *color);


G_DEFINE_TYPE (GimpCurvesTool, gimp_curves_tool, GIMP_TYPE_FILTER_TOOL)

#define parent_class gimp_curves_tool_parent_class


/*  public functions  */

void
gimp_curves_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_CURVES_TOOL,
                GIMP_TYPE_HISTOGRAM_OPTIONS,
                gimp_color_options_gui,
                0,
                "gimp-curves-tool",
                _("Curves"),
                _("Adjust color curves"),
                N_("_Curves..."), NULL,
                NULL, GIMP_HELP_TOOL_CURVES,
                GIMP_ICON_TOOL_CURVES,
                data);
}


/*  private functions  */

static void
gimp_curves_tool_class_init (GimpCurvesToolClass *klass)
{
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  tool_class->initialize             = gimp_curves_tool_initialize;
  tool_class->button_release         = gimp_curves_tool_button_release;
  tool_class->key_press              = gimp_curves_tool_key_press;
  tool_class->oper_update            = gimp_curves_tool_oper_update;

  filter_tool_class->get_operation   = gimp_curves_tool_get_operation;
  filter_tool_class->dialog          = gimp_curves_tool_dialog;
  filter_tool_class->reset           = gimp_curves_tool_reset;
  filter_tool_class->config_notify   = gimp_curves_tool_config_notify;
  filter_tool_class->settings_import = gimp_curves_tool_settings_import;
  filter_tool_class->settings_export = gimp_curves_tool_settings_export;
  filter_tool_class->color_picked    = gimp_curves_tool_color_picked;
}

static void
gimp_curves_tool_init (GimpCurvesTool *tool)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (tool->picked_color); i++)
    tool->picked_color[i] = -1.0;
}

static gboolean
gimp_curves_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpFilterTool       *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesTool       *c_tool      = GIMP_CURVES_TOOL (tool);
  GimpImage            *image       = gimp_display_get_image (display);
  GimpDrawable         *drawable    = gimp_image_get_active_drawable (image);
  GimpCurvesConfig     *config;
  GimpHistogram        *histogram;
  GimpHistogramChannel  channel;

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  config = GIMP_CURVES_CONFIG (filter_tool->config);

  gegl_node_set (filter_tool->operation,
                 "linear", config->linear,
                 NULL);

  histogram = gimp_histogram_new (config->linear);
  g_object_unref (gimp_drawable_calculate_histogram_async (
    drawable, histogram, FALSE));
  gimp_histogram_view_set_background (GIMP_HISTOGRAM_VIEW (c_tool->graph),
                                      histogram);
  g_object_unref (histogram);

  if (gimp_drawable_get_component_type (drawable) == GIMP_COMPONENT_TYPE_U8)
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

  gimp_curve_view_set_range_x (GIMP_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);
  gimp_curve_view_set_range_y (GIMP_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (c_tool->point_output),
                             0, c_tool->scale);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      g_signal_connect (config->curve[channel], "dirty",
                        G_CALLBACK (curves_curve_dirty_callback),
                        tool);
    }

  gimp_curves_tool_update_point (c_tool);

  /*  always pick colors  */
  gimp_filter_tool_enable_color_picking (filter_tool, NULL, FALSE);

  return TRUE;
}

static void
gimp_curves_tool_button_release (GimpTool              *tool,
                                 const GimpCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 GimpButtonReleaseType  release_type,
                                 GimpDisplay           *display)
{
  GimpCurvesTool   *c_tool      = GIMP_CURVES_TOOL (tool);
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);

  if (state & gimp_get_extend_selection_mask ())
    {
      GimpCurve *curve = config->curve[config->channel];
      gdouble    value = c_tool->picked_color[config->channel];
      gint       point;

      point = gimp_curve_get_point_at (curve, value);

      if (point < 0)
        {
          GimpCurvePointType type = GIMP_CURVE_POINT_SMOOTH;

          point = gimp_curve_view_get_selected (
            GIMP_CURVE_VIEW (c_tool->graph));

          if (point >= 0)
            type = gimp_curve_get_point_type (curve, point);

          point = gimp_curve_add_point (
            curve,
            value, gimp_curve_map_value (curve, value));

          gimp_curve_set_point_type (curve, point, type);
        }

      gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph), point);
    }
  else if (state & gimp_get_toggle_behavior_mask ())
    {
      GimpHistogramChannel channel;
      GimpCurvePointType   type = GIMP_CURVE_POINT_SMOOTH;
      gint                 point;

      point = gimp_curve_view_get_selected (GIMP_CURVE_VIEW (c_tool->graph));

      if (point >= 0)
        {
          type = gimp_curve_get_point_type (config->curve[config->channel],
                                            point);
        }

      for (channel = GIMP_HISTOGRAM_VALUE;
           channel <= GIMP_HISTOGRAM_ALPHA;
           channel++)
        {
          GimpCurve *curve = config->curve[channel];
          gdouble    value = c_tool->picked_color[channel];

          if (value != -1)
            {
              point = gimp_curve_get_point_at (curve, value);

              if (point < 0)
                {
                  point = gimp_curve_add_point (
                    curve,
                    value, gimp_curve_map_value (curve, value));

                  gimp_curve_set_point_type (curve, point, type);
                }

              if (channel == config->channel)
                {
                  gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                                point);
                }
            }
        }
    }

  /*  chain up to halt the tool */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static gboolean
gimp_curves_tool_key_press (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpCurvesTool *c_tool = GIMP_CURVES_TOOL (tool);

  if (tool->display && c_tool->graph)
    {
      if (gtk_widget_event (c_tool->graph, (GdkEvent *) kevent))
        return TRUE;
    }

  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
gimp_curves_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  if (gimp_filter_tool_on_guide (GIMP_FILTER_TOOL (tool),
                                 coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                                   display);
    }
  else
    {
      GimpColorPickTarget  target;
      gchar               *status      = NULL;
      GdkModifierType      extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType      toggle_mask = gimp_get_toggle_behavior_mask ();

      gimp_tool_pop_status (tool, display);

      if (state & extend_mask)
        {
          target = GIMP_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add a control point"));
        }
      else if (state & toggle_mask)
        {
          target = GIMP_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add control points to all channels"));
        }
      else
        {
          target = GIMP_COLOR_PICK_TARGET_NONE;
          status = gimp_suggest_modifiers (_("Click to locate on curve"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: add control point"),
                                           _("%s: add control points to all channels"),
                                           NULL);
        }

      GIMP_COLOR_TOOL (tool)->pick_target = target;

      if (proximity)
        gimp_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
}

static gchar *
gimp_curves_tool_get_operation (GimpFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Curves"));

  return g_strdup ("gimp:curves");
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
gimp_curves_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpCurvesTool   *tool         = GIMP_CURVES_TOOL (filter_tool);
  GimpToolOptions  *tool_options = GIMP_TOOL_GET_OPTIONS (filter_tool);
  GimpCurvesConfig *config       = GIMP_CURVES_CONFIG (filter_tool->config);
  GtkListStore     *store;
  GtkWidget        *main_vbox;
  GtkWidget        *frame_vbox;
  GtkWidget        *vbox;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *main_frame;
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *button;
  GtkWidget        *bar;
  GtkWidget        *combo;

  g_signal_connect (filter_tool->settings_box, "file-dialog-setup",
                    G_CALLBACK (gimp_curves_tool_export_setup),
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

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (tool->channel_menu),
                                       "gimp-channel");
  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (tool->channel_menu),
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
                                          _("Adjust curves in linear light"),
                                          _("Adjust curves perceptually"));
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_show (frame_vbox);

  /*  The table for the color bars and the graph  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (frame_vbox), table, TRUE, TRUE, 0);

  /*  The left color bar  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_table_attach (GTK_TABLE (table), vbox, 0, 1, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, RADIUS);
  gtk_widget_show (frame);

  tool->yrange = gimp_color_bar_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request (tool->yrange, BAR_SIZE, -1);
  gtk_container_add (GTK_CONTAINER (frame), tool->yrange);
  gtk_widget_show (tool->yrange);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  tool->graph = gimp_curve_view_new ();

  g_object_add_weak_pointer (G_OBJECT (tool->graph),
                             (gpointer) &tool->graph);

  gimp_curve_view_set_range_x (GIMP_CURVE_VIEW (tool->graph), 0, 255);
  gimp_curve_view_set_range_y (GIMP_CURVE_VIEW (tool->graph), 0, 255);
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
  gtk_table_attach (GTK_TABLE (table), hbox2, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, RADIUS);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  tool->xrange = gimp_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (tool->xrange, -1, BAR_SIZE / 2);
  gtk_box_pack_start (GTK_BOX (vbox), tool->xrange, TRUE, TRUE, 0);
  gtk_widget_show (tool->xrange);

  bar = gimp_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), bar, TRUE, TRUE, 0);
  gtk_widget_show (bar);

  gtk_widget_show (table);

  /*  The point properties box  */
  tool->point_box = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_box);

  label = gtk_label_new_with_mnemonic (_("_Input:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  tool->point_input = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_input, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_input);

  g_signal_connect (tool->point_input, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_input);

  label = gtk_label_new_with_mnemonic (_("O_utput:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  tool->point_output = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_output, FALSE, FALSE, 0);
  gtk_widget_show (tool->point_output);

  g_signal_connect (tool->point_output, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_output);

  label = gtk_label_new_with_mnemonic (_("T_ype:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  hbox2 = gimp_enum_icon_box_new (GIMP_TYPE_CURVE_POINT_TYPE,
                                  "gimp-curve-point",
                                  GTK_ICON_SIZE_MENU,
                                  G_CALLBACK (curves_point_type_callback),
                                  tool,
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
  tool->curve_type = combo = gimp_enum_combo_box_new (GIMP_TYPE_CURVE_TYPE);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-curve");
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), 0,
                              G_CALLBACK (curves_curve_type_callback),
                              tool);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  gimp_curves_tool_update_channel (tool);
}

static void
gimp_curves_tool_reset (GimpFilterTool *filter_tool)
{
  GimpCurvesConfig     *config = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpCurvesConfig     *default_config;
  GimpHistogramChannel  channel;

  default_config = GIMP_CURVES_CONFIG (filter_tool->default_config);

  g_object_freeze_notify (G_OBJECT (config));

  if (default_config)
    g_object_set (config, "linear", default_config->linear, NULL);
  else
    gimp_config_reset_property (G_OBJECT (config), "linear");

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      if (default_config)
        {
          GimpCurveType curve_type = config->curve[channel]->curve_type;

          g_object_freeze_notify (G_OBJECT (config->curve[channel]));

          gimp_config_copy (GIMP_CONFIG (default_config->curve[channel]),
                            GIMP_CONFIG (config->curve[channel]),
                            0);

          g_object_set (config->curve[channel],
                        "curve-type", curve_type,
                        NULL);

          g_object_thaw_notify (G_OBJECT (config->curve[channel]));
        }
      else
        {
          gimp_curve_reset (config->curve[channel], FALSE);
        }
    }

  g_object_thaw_notify (G_OBJECT (config));
}

static void
gimp_curves_tool_config_notify (GimpFilterTool   *filter_tool,
                                GimpConfig       *config,
                                const GParamSpec *pspec)
{
  GimpCurvesTool   *curves_tool   = GIMP_CURVES_TOOL (filter_tool);
  GimpCurvesConfig *curves_config = GIMP_CURVES_CONFIG (config);
  GimpCurve        *curve         = curves_config->curve[curves_config->channel];

  GIMP_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! curves_tool->channel_menu ||
      ! curves_tool->graph)
    return;

  if (! strcmp (pspec->name, "linear"))
    {
      GimpHistogram *histogram;

      gegl_node_set (filter_tool->operation,
                     "linear", curves_config->linear,
                     NULL);

      histogram = gimp_histogram_new (curves_config->linear);
      g_object_unref (gimp_drawable_calculate_histogram_async (
        GIMP_TOOL (filter_tool)->drawable, histogram, FALSE));
      gimp_histogram_view_set_background (GIMP_HISTOGRAM_VIEW (curves_tool->graph),
                                          histogram);
      g_object_unref (histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      gimp_curves_tool_update_channel (GIMP_CURVES_TOOL (filter_tool));
    }
  else if (! strcmp (pspec->name, "curve"))
    {
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (curves_tool->curve_type),
                                     curve->curve_type);
    }
}

static gboolean
gimp_curves_tool_settings_import (GimpFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  GimpCurvesConfig *config = GIMP_CURVES_CONFIG (filter_tool->config);
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

  if (g_str_has_prefix (header, "# GIMP Curves File\n"))
    return gimp_curves_config_load_cruft (config, input, error);

  return GIMP_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
gimp_curves_tool_settings_export (GimpFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  GimpCurvesTool   *tool   = GIMP_CURVES_TOOL (filter_tool);
  GimpCurvesConfig *config = GIMP_CURVES_CONFIG (filter_tool->config);

  if (tool->export_old_format)
    return gimp_curves_config_save_cruft (config, output, error);

  return GIMP_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
}

static void
gimp_curves_tool_color_picked (GimpFilterTool *filter_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               const GimpRGB  *color)
{
  GimpCurvesTool   *tool     = GIMP_CURVES_TOOL (filter_tool);
  GimpCurvesConfig *config   = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpDrawable     *drawable = GIMP_TOOL (tool)->drawable;
  GimpRGB           rgb      = *color;

  if (config->linear)
    babl_process (babl_fish (babl_format ("R'G'B'A double"),
                             babl_format ("RGBA double")),
                  &rgb, &rgb, 1);

  tool->picked_color[GIMP_HISTOGRAM_RED]   = rgb.r;
  tool->picked_color[GIMP_HISTOGRAM_GREEN] = rgb.g;
  tool->picked_color[GIMP_HISTOGRAM_BLUE]  = rgb.b;

  if (gimp_drawable_has_alpha (drawable))
    tool->picked_color[GIMP_HISTOGRAM_ALPHA] = rgb.a;
  else
    tool->picked_color[GIMP_HISTOGRAM_ALPHA] = -1;

  tool->picked_color[GIMP_HISTOGRAM_VALUE] = MAX (MAX (rgb.r, rgb.g), rgb.b);

  gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);
}

static void
gimp_curves_tool_export_setup (GimpSettingsBox      *settings_box,
                               GtkFileChooserDialog *dialog,
                               gboolean              export,
                               GimpCurvesTool       *tool)
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
                    G_CALLBACK (gimp_toggle_button_update),
                    &tool->export_old_format);
}

static void
gimp_curves_tool_update_channel (GimpCurvesTool *tool)
{
  GimpFilterTool       *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig     *config      = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpCurve            *curve       = config->curve[config->channel];
  GimpHistogramChannel  channel;

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);

  switch (config->channel)
    {
      guchar r[256];
      guchar g[256];
      guchar b[256];

    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:
    case GIMP_HISTOGRAM_RGB:
    case GIMP_HISTOGRAM_LUMINANCE:
      gimp_curve_get_uchar (curve, sizeof (r), r);

      gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                  r, r, r);
      break;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_RED],
                            sizeof (r), r);
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_GREEN],
                            sizeof (g), g);
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_BLUE],
                            sizeof (b), b);

      gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                  r, g, b);
      break;
    }

  gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (tool->graph),
                                   config->channel);
  gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);

  gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->yrange),
                              config->channel);

  gimp_curve_view_remove_all_backgrounds (GIMP_CURVE_VIEW (tool->graph));

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      GimpRGB  curve_color;
      gboolean has_color;

      has_color = curves_get_channel_color (tool->graph, channel, &curve_color);

      if (channel == config->channel)
        {
          gimp_curve_view_set_curve (GIMP_CURVE_VIEW (tool->graph), curve,
                                     has_color ? &curve_color : NULL);
        }
      else
        {
          gimp_curve_view_add_background (GIMP_CURVE_VIEW (tool->graph),
                                          config->curve[channel],
                                          has_color ? &curve_color : NULL);
        }
    }

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->curve_type),
                                 curve->curve_type);

  gimp_curves_tool_update_point (tool);
}

static void
gimp_curves_tool_update_point (GimpCurvesTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = gimp_curve_view_get_selected (GIMP_CURVE_VIEW (tool->graph));

  gtk_widget_set_sensitive (tool->point_box, point >= 0);

  if (point >= 0)
    {
      gdouble min = 0.0;
      gdouble max = 1.0;
      gdouble x;
      gdouble y;

      if (point > 0)
        gimp_curve_get_point (curve, point - 1, &min, NULL);

      if (point < gimp_curve_get_n_points (curve) - 1)
        gimp_curve_get_point (curve, point + 1, &max, NULL);

      gimp_curve_get_point (curve, point, &x, &y);

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

      gimp_int_radio_group_set_active (
        GTK_RADIO_BUTTON (tool->point_type),
        gimp_curve_get_point_type (curve, point));

      g_signal_handlers_unblock_by_func (tool->point_type,
                                         curves_point_type_callback,
                                         tool);
    }
}

static void
curves_curve_dirty_callback (GimpCurve      *curve,
                             GimpCurvesTool *tool)
{
  if (tool->graph &&
      gimp_curve_view_get_curve (GIMP_CURVE_VIEW (tool->graph)) == curve)
    {
      gimp_curves_tool_update_point (tool);
    }
}

static void
curves_channel_callback (GtkWidget      *widget,
                         GimpCurvesTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);
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
curves_channel_reset_callback (GtkWidget      *widget,
                               GimpCurvesTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);

  gimp_curve_reset (config->curve[config->channel], FALSE);
}

static gboolean
curves_menu_sensitivity (gint      value,
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
curves_graph_selection_callback (GtkWidget      *widget,
                                 GimpCurvesTool *tool)
{
  gimp_curves_tool_update_point (tool);
}

static void
curves_point_coords_callback (GtkWidget      *widget,
                              GimpCurvesTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = gimp_curve_view_get_selected (GIMP_CURVE_VIEW (tool->graph));

  if (point >= 0)
    {
      gdouble x;
      gdouble y;

      x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_input));
      y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_output));

      x /= tool->scale;
      y /= tool->scale;

      gimp_curve_set_point (curve, point, x, y);
    }
}

static void
curves_point_type_callback (GtkWidget      *widget,
                            GimpCurvesTool *tool)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);
  GimpCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = gimp_curve_view_get_selected (GIMP_CURVE_VIEW (tool->graph));

  if (point >= 0 && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpCurvePointType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                 "gimp-item-data"));

      gimp_curve_set_point_type (curve, point, type);
    }
}

static void
curves_curve_type_callback (GtkWidget      *widget,
                            GimpCurvesTool *tool)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (tool);
      GimpCurvesConfig *config      = GIMP_CURVES_CONFIG (filter_tool->config);
      GimpCurveType     curve_type  = value;

      if (config->curve[config->channel]->curve_type != curve_type)
        gimp_curve_set_curve_type (config->curve[config->channel], curve_type);
    }
}

static gboolean
curves_get_channel_color (GtkWidget            *widget,
                          GimpHistogramChannel  channel,
                          GimpRGB              *color)
{
  static const GimpRGB channel_colors[GIMP_HISTOGRAM_RGB] =
  {
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.5, 0.5, 0.5, 1.0 }
  };

  if (channel == GIMP_HISTOGRAM_VALUE)
    return FALSE;

  if (channel == GIMP_HISTOGRAM_ALPHA)
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      gimp_rgba_set (color,
                     style->text_aa[GTK_STATE_NORMAL].red / 65535.0,
                     style->text_aa[GTK_STATE_NORMAL].green / 65535.0,
                     style->text_aa[GTK_STATE_NORMAL].blue / 65535.0,
                     1.0);
      return TRUE;
    }

  *color = channel_colors[channel];
  return TRUE;
}
