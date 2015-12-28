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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/curves.h"
#include "base/gimphistogram.h"
#include "base/gimplut.h"

#include "gegl/gimpcurvesconfig.h"
#include "gegl/gimpoperationcurves.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpcurveview.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpcurvestool.h"
#include "gimphistogramoptions.h"

#include "gimp-intl.h"


#define GRAPH_SIZE 256
#define BAR_SIZE    12
#define RADIUS       4


/*  local function prototypes  */

static void       gimp_curves_tool_constructed    (GObject              *object);
static void       gimp_curves_tool_finalize       (GObject              *object);

static gboolean   gimp_curves_tool_initialize     (GimpTool             *tool,
                                                   GimpDisplay          *display,
                                                   GError              **error);
static void       gimp_curves_tool_button_release (GimpTool             *tool,
                                                   const GimpCoords     *coords,
                                                   guint32               time,
                                                   GdkModifierType       state,
                                                   GimpButtonReleaseType release_type,
                                                   GimpDisplay          *display);
static gboolean   gimp_curves_tool_key_press      (GimpTool             *tool,
                                                   GdkEventKey          *kevent,
                                                   GimpDisplay          *display);
static void       gimp_curves_tool_oper_update    (GimpTool             *tool,
                                                   const GimpCoords     *coords,
                                                   GdkModifierType       state,
                                                   gboolean              proximity,
                                                   GimpDisplay          *display);

static void       gimp_curves_tool_color_picked   (GimpColorTool        *color_tool,
                                                   GimpColorPickState    pick_state,
                                                   GimpImageType         sample_type,
                                                   const GimpRGB        *color,
                                                   gint                  color_index);
static GeglNode * gimp_curves_tool_get_operation  (GimpImageMapTool     *image_map_tool,
                                                   GObject             **config);
static void       gimp_curves_tool_map            (GimpImageMapTool     *image_map_tool);
static void       gimp_curves_tool_dialog         (GimpImageMapTool     *image_map_tool);
static void       gimp_curves_tool_reset          (GimpImageMapTool     *image_map_tool);
static gboolean   gimp_curves_tool_settings_import(GimpImageMapTool     *image_map_tool,
                                                   const gchar          *filename,
                                                   GError              **error);
static gboolean   gimp_curves_tool_settings_export(GimpImageMapTool     *image_map_tool,
                                                   const gchar          *filename,
                                                   GError              **error);

static void       gimp_curves_tool_export_setup   (GimpSettingsBox      *settings_box,
                                                   GtkFileChooserDialog *dialog,
                                                   gboolean              export,
                                                   GimpCurvesTool       *tool);
static void       gimp_curves_tool_update_channel (GimpCurvesTool       *tool);
static void       gimp_curves_tool_config_notify  (GObject              *object,
                                                   GParamSpec           *pspec,
                                                   GimpCurvesTool       *tool);

static void       curves_channel_callback         (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);
static void       curves_channel_reset_callback   (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);

static gboolean   curves_menu_sensitivity         (gint                  value,
                                                   gpointer              data);

static void       curves_curve_type_callback      (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);

static const GimpRGB * curves_get_channel_color   (GimpHistogramChannel  channel);


G_DEFINE_TYPE (GimpCurvesTool, gimp_curves_tool, GIMP_TYPE_IMAGE_MAP_TOOL)

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
                _("Curves Tool: Adjust color curves"),
                N_("_Curves..."), NULL,
                NULL, GIMP_HELP_TOOL_CURVES,
                GIMP_STOCK_TOOL_CURVES,
                data);
}


/*  private functions  */

static void
gimp_curves_tool_class_init (GimpCurvesToolClass *klass)
{
  GObjectClass          *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass    *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class    = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->constructed          = gimp_curves_tool_constructed;
  object_class->finalize             = gimp_curves_tool_finalize;

  tool_class->initialize             = gimp_curves_tool_initialize;
  tool_class->button_release         = gimp_curves_tool_button_release;
  tool_class->key_press              = gimp_curves_tool_key_press;
  tool_class->oper_update            = gimp_curves_tool_oper_update;

  color_tool_class->picked           = gimp_curves_tool_color_picked;

  im_tool_class->dialog_desc         = _("Adjust Color Curves");
  im_tool_class->settings_name       = "curves";
  im_tool_class->import_dialog_title = _("Import Curves");
  im_tool_class->export_dialog_title = _("Export Curves");

  im_tool_class->get_operation       = gimp_curves_tool_get_operation;
  im_tool_class->map                 = gimp_curves_tool_map;
  im_tool_class->dialog              = gimp_curves_tool_dialog;
  im_tool_class->reset               = gimp_curves_tool_reset;
  im_tool_class->settings_import     = gimp_curves_tool_settings_import;
  im_tool_class->settings_export     = gimp_curves_tool_settings_export;
}

static void
gimp_curves_tool_init (GimpCurvesTool *tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);
  gint              i;

  tool->lut = gimp_lut_new ();

  for (i = 0; i < G_N_ELEMENTS (tool->picked_color); i++)
    tool->picked_color[i] = -1.0;

  im_tool->apply_func = (GimpImageMapApplyFunc) gimp_lut_process;
  im_tool->apply_data = tool->lut;
}

static void
gimp_curves_tool_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  /*  always pick colors  */
  gimp_color_tool_enable (GIMP_COLOR_TOOL (object),
                          GIMP_COLOR_TOOL_GET_OPTIONS (object));

}

static void
gimp_curves_tool_finalize (GObject *object)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (object);

  gimp_lut_free (tool->lut);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_curves_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpCurvesTool *c_tool   = GIMP_CURVES_TOOL (tool);
  GimpImage      *image    = gimp_display_get_image (display);
  GimpDrawable   *drawable = gimp_image_get_active_drawable (image);
  GimpHistogram  *histogram;

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Curves does not operate on indexed layers."));
      return FALSE;
    }

  gimp_config_reset (GIMP_CONFIG (c_tool->config));

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (c_tool->channel_menu),
                                      curves_menu_sensitivity, drawable, NULL);

  histogram = gimp_histogram_new ();
  gimp_drawable_calculate_histogram (drawable, histogram);
  gimp_histogram_view_set_background (GIMP_HISTOGRAM_VIEW (c_tool->graph),
                                      histogram);
  gimp_histogram_unref (histogram);

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
  GimpCurvesTool   *c_tool = GIMP_CURVES_TOOL (tool);
  GimpCurvesConfig *config = c_tool->config;

  if (state & GDK_SHIFT_MASK)
    {
      GimpCurve *curve = config->curve[config->channel];
      gdouble    value = c_tool->picked_color[config->channel];
      gint       closest;

      closest = gimp_curve_get_closest_point (curve, value);

      gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                    closest);

      gimp_curve_set_point (curve, closest,
                            value, gimp_curve_map_value (curve, value));
    }
  else if (state & gimp_get_toggle_behavior_mask ())
    {
      GimpHistogramChannel channel;

      for (channel = GIMP_HISTOGRAM_VALUE;
           channel <= GIMP_HISTOGRAM_ALPHA;
           channel++)
        {
          GimpCurve *curve = config->curve[channel];
          gdouble    value = c_tool->picked_color[channel];
          gint       closest;

          if (value != -1)
            {
              closest = gimp_curve_get_closest_point (curve, value);

              gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                            closest);

              gimp_curve_set_point (curve, closest,
                                    value, gimp_curve_map_value (curve, value));
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

  if (c_tool->graph && gtk_widget_event (c_tool->graph, (GdkEvent *) kevent))
    return TRUE;

  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
gimp_curves_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  GimpColorPickMode  mode;
  const gchar       *status;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  gimp_tool_pop_status (tool, display);

  if (state & GDK_SHIFT_MASK)
    {
      mode   = GIMP_COLOR_PICK_MODE_PALETTE;
      status = _("Click to add a control point");
    }
  else if (state & gimp_get_toggle_behavior_mask ())
    {
      mode   = GIMP_COLOR_PICK_MODE_PALETTE;
      status = _("Click to add control points to all channels");
    }
  else
    {
      mode   = GIMP_COLOR_PICK_MODE_NONE;
      status = _("Click to locate on curve (try Shift, Ctrl)");
    }

  GIMP_COLOR_TOOL (tool)->pick_mode = mode;

  if (proximity)
    gimp_tool_push_status (tool, display, "%s", status);
}

static void
gimp_curves_tool_color_picked (GimpColorTool      *color_tool,
                               GimpColorPickState  pick_state,
                               GimpImageType       sample_type,
                               const GimpRGB      *color,
                               gint                color_index)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (color_tool);
  GimpDrawable   *drawable;

  drawable = GIMP_IMAGE_MAP_TOOL (tool)->drawable;

  tool->picked_color[GIMP_HISTOGRAM_RED]   = color->r;
  tool->picked_color[GIMP_HISTOGRAM_GREEN] = color->g;
  tool->picked_color[GIMP_HISTOGRAM_BLUE]  = color->b;

  if (gimp_drawable_has_alpha (drawable))
    tool->picked_color[GIMP_HISTOGRAM_ALPHA] = color->a;
  else
    tool->picked_color[GIMP_HISTOGRAM_ALPHA] = -1;

  tool->picked_color[GIMP_HISTOGRAM_VALUE] = MAX (MAX (color->r, color->g),
                                                  color->b);

  gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                            tool->picked_color[tool->config->channel]);
}

static GeglNode *
gimp_curves_tool_get_operation (GimpImageMapTool  *image_map_tool,
                                GObject          **config)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  GeglNode       *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:curves",
                       NULL);

  tool->config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  *config = G_OBJECT (tool->config);

  g_signal_connect_object (tool->config, "notify",
                           G_CALLBACK (gimp_curves_tool_config_notify),
                           tool, 0);

  gegl_node_set (node,
                 "config", tool->config,
                 NULL);

  return node;
}

static void
gimp_curves_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *tool     = GIMP_CURVES_TOOL (image_map_tool);
  GimpDrawable   *drawable = image_map_tool->drawable;
  Curves          curves;

  gimp_curves_config_to_cruft (tool->config, &curves,
                               gimp_drawable_is_rgb (drawable));

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) curves_lut_func,
                  &curves,
                  gimp_drawable_bytes (drawable));
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
gimp_curves_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool   *tool         = GIMP_CURVES_TOOL (image_map_tool);
  GimpToolOptions  *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GimpCurvesConfig *config       = tool->config;
  GtkListStore     *store;
  GtkSizeGroup     *label_group;
  GtkWidget        *main_vbox;
  GtkWidget        *vbox;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *button;
  GtkWidget        *bar;
  GtkWidget        *combo;

  g_signal_connect (image_map_tool->settings_box, "file-dialog-setup",
                    G_CALLBACK (gimp_curves_tool_export_setup),
                    image_map_tool);

  main_vbox   = gimp_image_map_tool_dialog_get_vbox (image_map_tool);
  label_group = gimp_image_map_tool_dialog_get_label_group (image_map_tool);

  /*  The combo box for selecting channels  */
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
  tool->channel_menu =
    gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));
  g_object_unref (store);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
  gimp_enum_combo_box_set_stock_prefix (GIMP_ENUM_COMBO_BOX (tool->channel_menu),
                                        "gimp-channel");
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
  hbox2 = gimp_prop_enum_stock_box_new (G_OBJECT (tool_options),
                                        "histogram-scale", "gimp-histogram",
                                        0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  /*  The table for the color bars and the graph  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, TRUE, TRUE, 0);

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

  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       GIMP_HISTOGRAM_VIEW (tool->graph));

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

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Curve _type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  tool->curve_type = combo = gimp_enum_combo_box_new (GIMP_TYPE_CURVE_TYPE);
  gimp_enum_combo_box_set_stock_prefix (GIMP_ENUM_COMBO_BOX (combo),
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
gimp_curves_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool       *tool = GIMP_CURVES_TOOL (image_map_tool);
  GimpCurvesConfig     *default_config;
  GimpHistogramChannel  channel;

  default_config = GIMP_CURVES_CONFIG (image_map_tool->default_config);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      if (default_config)
        {
          GimpCurveType curve_type = tool->config->curve[channel]->curve_type;

          g_object_freeze_notify (G_OBJECT (tool->config->curve[channel]));

          gimp_config_copy (GIMP_CONFIG (default_config->curve[channel]),
                            GIMP_CONFIG (tool->config->curve[channel]),
                            0);

          g_object_set (tool->config->curve[channel],
                        "curve-type", curve_type,
                        NULL);

          g_object_thaw_notify (G_OBJECT (tool->config->curve[channel]));
        }
      else
        {
          gimp_curve_reset (tool->config->curve[channel], FALSE);
        }
    }
}

static gboolean
gimp_curves_tool_settings_import (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
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

  if (g_str_has_prefix (header, "# GIMP Curves File\n"))
    {
      gboolean success;

      rewind (file);

      success = gimp_curves_config_load_cruft (tool->config, file, error);

      fclose (file);

      return success;
    }

  fclose (file);

  return GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->settings_import (image_map_tool,
                                                                    filename,
                                                                    error);
}

static gboolean
gimp_curves_tool_settings_export (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);

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

      success = gimp_curves_config_save_cruft (tool->config, file, error);

      fclose (file);

      return success;
    }

  return GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->settings_export (image_map_tool,
                                                                    filename,
                                                                    error);
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
  GimpCurvesConfig     *config = GIMP_CURVES_TOOL (tool)->config;
  GimpCurve            *curve  = config->curve[config->channel];
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
      if (channel == config->channel)
        {
          gimp_curve_view_set_curve (GIMP_CURVE_VIEW (tool->graph), curve,
                                     curves_get_channel_color (channel));
        }
      else
        {
          gimp_curve_view_add_background (GIMP_CURVE_VIEW (tool->graph),
                                          config->curve[channel],
                                          curves_get_channel_color (channel));
        }
    }

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->curve_type),
                                 curve->curve_type);
}

static void
gimp_curves_tool_config_notify (GObject        *object,
                                GParamSpec     *pspec,
                                GimpCurvesTool *tool)
{
  GimpCurvesConfig *config = GIMP_CURVES_CONFIG (object);
  GimpCurve        *curve  = config->curve[config->channel];

  if (! tool->xrange)
    return;

  if (! strcmp (pspec->name, "channel"))
    {
      gimp_curves_tool_update_channel (GIMP_CURVES_TOOL (tool));
    }
  else if (! strcmp (pspec->name, "curve"))
    {
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (tool->curve_type),
                                     curve->curve_type);
    }

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static void
curves_channel_callback (GtkWidget      *widget,
                         GimpCurvesTool *tool)
{
  GimpCurvesConfig *config = tool->config;
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
  gimp_curve_reset (tool->config->curve[tool->config->channel], FALSE);
}

static gboolean
curves_menu_sensitivity (gint      value,
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
curves_curve_type_callback (GtkWidget      *widget,
                            GimpCurvesTool *tool)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      GimpCurvesConfig *config     = tool->config;
      GimpCurveType     curve_type = value;

      if (config->curve[config->channel]->curve_type != curve_type)
        gimp_curve_set_curve_type (config->curve[config->channel], curve_type);
    }
}

static const GimpRGB *
curves_get_channel_color (GimpHistogramChannel channel)
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
    return NULL;

  return &channel_colors[channel];
}
