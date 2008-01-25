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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/curves.h"
#include "base/gimphistogram.h"
#include "base/gimplut.h"

#include "gegl/gimpcurvesconfig.h"
#include "gegl/gimpoperationcurves.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpcurveview.h"

#include "display/gimpdisplay.h"

#include "gimpcurvestool.h"
#include "gimphistogramoptions.h"

#include "gimp-intl.h"


#define GRAPH_SIZE 256
#define BAR_SIZE    12
#define RADIUS       4


/*  local function prototypes  */

static void       gimp_curves_tool_finalize       (GObject              *object);

static gboolean   gimp_curves_tool_initialize     (GimpTool             *tool,
                                                   GimpDisplay          *display,
                                                   GError              **error);
static void       gimp_curves_tool_button_release (GimpTool             *tool,
                                                   GimpCoords           *coords,
                                                   guint32               time,
                                                   GdkModifierType       state,
                                                   GimpButtonReleaseType release_type,
                                                   GimpDisplay          *display);
static gboolean   gimp_curves_tool_key_press      (GimpTool             *tool,
                                                   GdkEventKey          *kevent,
                                                   GimpDisplay          *display);
static void       gimp_curves_tool_oper_update    (GimpTool             *tool,
                                                   GimpCoords           *coords,
                                                   GdkModifierType       state,
                                                   gboolean              proximity,
                                                   GimpDisplay          *display);

static void       gimp_curves_tool_color_picked   (GimpColorTool        *color_tool,
                                                   GimpColorPickState    pick_state,
                                                   GimpImageType         sample_type,
                                                   GimpRGB              *color,
                                                   gint                  color_index);
static GeglNode * gimp_curves_tool_get_operation  (GimpImageMapTool     *image_map_tool);
static void       gimp_curves_tool_map            (GimpImageMapTool     *image_map_tool);
static void       gimp_curves_tool_dialog         (GimpImageMapTool     *image_map_tool);
static void       gimp_curves_tool_reset          (GimpImageMapTool     *image_map_tool);
static gboolean   gimp_curves_tool_settings_load  (GimpImageMapTool     *image_map_tool,
                                                   gpointer              fp,
                                                   GError              **error);
static gboolean   gimp_curves_tool_settings_save  (GimpImageMapTool     *image_map_tool,
                                                   gpointer              fp);

static void       curves_curve_callback           (GimpCurve            *curve,
                                                   GimpCurvesTool       *tool);
static void       curves_channel_callback         (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);
static void       curves_channel_reset_callback   (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);

static gboolean   curves_menu_sensitivity         (gint                  value,
                                                   gpointer              data);

static void       curves_curve_type_callback      (GtkWidget            *widget,
                                                   GimpCurvesTool       *tool);


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

  object_class->finalize           = gimp_curves_tool_finalize;

  tool_class->initialize           = gimp_curves_tool_initialize;
  tool_class->button_release       = gimp_curves_tool_button_release;
  tool_class->key_press            = gimp_curves_tool_key_press;
  tool_class->oper_update          = gimp_curves_tool_oper_update;

  color_tool_class->picked         = gimp_curves_tool_color_picked;

  im_tool_class->shell_desc        = _("Adjust Color Curves");
  im_tool_class->settings_name     = "curves";
  im_tool_class->load_dialog_title = _("Load Curves");
  im_tool_class->load_button_tip   = _("Load curves settings from file");
  im_tool_class->save_dialog_title = _("Save Curves");
  im_tool_class->save_button_tip   = _("Save curves settings to file");

  im_tool_class->get_operation     = gimp_curves_tool_get_operation;
  im_tool_class->map               = gimp_curves_tool_map;
  im_tool_class->dialog            = gimp_curves_tool_dialog;
  im_tool_class->reset             = gimp_curves_tool_reset;
  im_tool_class->settings_load     = gimp_curves_tool_settings_load;
  im_tool_class->settings_save     = gimp_curves_tool_settings_save;
}

static void
gimp_curves_tool_init (GimpCurvesTool *tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (tool);
  gint              i;

  tool->lut = gimp_lut_new ();

  for (i = 0; i < G_N_ELEMENTS (tool->col_value); i++)
    tool->col_value[i] = -1;

  im_tool->apply_func = (GimpImageMapApplyFunc) gimp_lut_process;
  im_tool->apply_data = tool->lut;
}

static void
gimp_curves_tool_finalize (GObject *object)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (object);

  if (tool->config)
    {
      g_object_unref (tool->config);
      tool->config = NULL;
    }

  gimp_lut_free (tool->lut);

  if (tool->hist)
    {
      gimp_histogram_free (tool->hist);
      tool->hist = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_curves_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpCurvesTool *c_tool   = GIMP_CURVES_TOOL (tool);
  GimpDrawable   *drawable = gimp_image_get_active_drawable (display->image);
  gint            i;

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Curves does not operate on indexed layers."));
      return FALSE;
    }

  for (i = 0; i < G_N_ELEMENTS (c_tool->config->curve); i++)
    gimp_curve_reset (c_tool->config->curve[i], TRUE);

  if (! c_tool->hist)
    c_tool->hist = gimp_histogram_new ();

  c_tool->color = gimp_drawable_is_rgb (drawable);
  c_tool->alpha = gimp_drawable_has_alpha (drawable);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  /*  always pick colors  */
  gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                          GIMP_COLOR_TOOL_GET_OPTIONS (tool));

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (c_tool->channel_menu),
                                      curves_menu_sensitivity, c_tool, NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (c_tool->channel_menu),
                                 c_tool->config->channel);

  gimp_drawable_calculate_histogram (drawable, c_tool->hist);
  gimp_histogram_view_set_background (GIMP_HISTOGRAM_VIEW (c_tool->graph),
                                      c_tool->hist);
  gimp_curve_view_set_curve (GIMP_CURVE_VIEW (c_tool->graph),
                             c_tool->config->curve[c_tool->config->channel]);

  return TRUE;
}

static void
gimp_curves_tool_button_release (GimpTool              *tool,
                                 GimpCoords            *coords,
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
      gint       closest;

      closest =
        gimp_curve_get_closest_point (curve,
                                      c_tool->col_value[config->channel]);

      gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                    closest);

      gimp_curve_set_point (curve,
                            closest,
                            c_tool->col_value[config->channel],
                            curve->curve[c_tool->col_value[config->channel]]);
    }
  else if (state & GDK_CONTROL_MASK)
    {
      gint i;

      for (i = 0; i < 5; i++)
        {
          GimpCurve *curve = config->curve[i];
          gint       closest;

          closest =
            gimp_curve_get_closest_point (curve,
                                          c_tool->col_value[i]);

          gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                        closest);

          gimp_curve_set_point (curve,
                                closest,
                                c_tool->col_value[i],
                                curve->curve[c_tool->col_value[i]]);
        }
    }

  /*  chain up to halt the tool */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

gboolean
gimp_curves_tool_key_press (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpCurvesTool *c_tool = GIMP_CURVES_TOOL (tool);

  return gtk_widget_event (c_tool->graph, (GdkEvent *) kevent);
}

static void
gimp_curves_tool_oper_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              gboolean         proximity,
                              GimpDisplay     *display)
{
  GimpColorPickMode  mode   = GIMP_COLOR_PICK_MODE_NONE;
  const gchar       *status = NULL;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  gimp_tool_pop_status (tool, display);

  if (state & GDK_SHIFT_MASK)
    {
      mode   = GIMP_COLOR_PICK_MODE_PALETTE;
      status = _("Click to add a control point");
    }
  else if (state & GDK_CONTROL_MASK)
    {
      mode   = GIMP_COLOR_PICK_MODE_PALETTE;
      status = _("Click to add control points to all channels");
    }

  GIMP_COLOR_TOOL (tool)->pick_mode = mode;

  if (status && proximity)
    gimp_tool_push_status (tool, display, status);
}

static void
gimp_curves_tool_color_picked (GimpColorTool      *color_tool,
                               GimpColorPickState  pick_state,
                               GimpImageType       sample_type,
                               GimpRGB            *color,
                               gint                color_index)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (color_tool);
  GimpDrawable   *drawable;
  guchar          r, g, b, a;

  drawable = GIMP_IMAGE_MAP_TOOL (tool)->drawable;

  gimp_rgba_get_uchar (color, &r, &g, &b, &a);

  tool->col_value[GIMP_HISTOGRAM_RED]   = r;
  tool->col_value[GIMP_HISTOGRAM_GREEN] = g;
  tool->col_value[GIMP_HISTOGRAM_BLUE]  = b;

  if (gimp_drawable_has_alpha (drawable))
    tool->col_value[GIMP_HISTOGRAM_ALPHA] = a;

  if (gimp_drawable_is_indexed (drawable))
    tool->col_value[GIMP_HISTOGRAM_ALPHA] = color_index;

  tool->col_value[GIMP_HISTOGRAM_VALUE] = MAX (MAX (r, g), b);

  gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                            tool->col_value[tool->config->channel]);
}

static GeglNode *
gimp_curves_tool_get_operation (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  GeglNode       *node;
  gint            i;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp-curves",
                       NULL);

  tool->config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  for (i = 0; i < G_N_ELEMENTS (tool->config->curve); i++)
    {
      g_signal_connect_object (tool->config->curve[i], "dirty",
                               G_CALLBACK (curves_curve_callback),
                               tool, 0);
    }

  gegl_node_set (node,
                 "config", tool->config,
                 NULL);

  return node;
}

static void
gimp_curves_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  Curves          curves;

  gimp_curves_config_to_cruft (tool->config, &curves, tool->color);

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) curves_lut_func,
                  &curves,
                  gimp_drawable_bytes (image_map_tool->drawable));
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
gimp_curves_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool  *tool         = GIMP_CURVES_TOOL (image_map_tool);
  GimpToolOptions *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GtkListStore    *store;
  GtkWidget       *vbox;
  GtkWidget       *vbox2;
  GtkWidget       *hbox;
  GtkWidget       *hbox2;
  GtkWidget       *label;
  GtkWidget       *bbox;
  GtkWidget       *frame;
  GtkWidget       *menu;
  GtkWidget       *table;
  GtkWidget       *button;
  GtkWidget       *bar;
  gint             padding;

  vbox = image_map_tool->main_vbox;

  /*  The option menu for selecting channels  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
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
                    G_CALLBACK (curves_channel_callback),
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
                    G_CALLBACK (curves_channel_reset_callback),
                    tool);

  menu = gimp_prop_enum_stock_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  /*  The table for the color bars and the graph  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  The left color bar  */
  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), vbox2, 0, 1, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, RADIUS);
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
  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox2, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, RADIUS);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  tool->xrange = gimp_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (tool->xrange, -1, BAR_SIZE / 2);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->xrange, TRUE, TRUE, 0);
  gtk_widget_show (tool->xrange);

  bar = gimp_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox2), bar, TRUE, TRUE, 0);
  gtk_widget_show (bar);

  gtk_widget_show (table);


  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Horizontal button box for load / save */
  frame = gimp_frame_new (_("All Channels"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  bbox = gtk_hbutton_box_new ();
  gtk_box_set_spacing (GTK_BOX (bbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), bbox);
  gtk_widget_show (bbox);

  gtk_box_pack_start (GTK_BOX (bbox), image_map_tool->load_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->load_button);

  gtk_box_pack_start (GTK_BOX (bbox), image_map_tool->save_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (image_map_tool->save_button);

  /*  The radio box for selecting the curve type  */
  frame = gimp_frame_new (_("Curve Type"));
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gimp_enum_stock_box_new (GIMP_TYPE_CURVE_TYPE,
                                  "gimp-curve", GTK_ICON_SIZE_MENU,
                                  G_CALLBACK (curves_curve_type_callback),
                                  tool,
                                  &tool->curve_type);

  gtk_widget_style_get (bbox, "child-internal-pad-x", &padding, NULL);

  gimp_enum_stock_box_set_child_padding (hbox, padding, -1);

  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);
}

static void
gimp_curves_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool       *tool    = GIMP_CURVES_TOOL (image_map_tool);
  GimpHistogramChannel  channel = tool->config->channel;

  gimp_curves_config_reset (tool->config);
  g_object_set (tool->config,
                "channel", channel,
                NULL);
}

static gboolean
gimp_curves_tool_settings_load (GimpImageMapTool  *image_map_tool,
                                gpointer           fp,
                                GError           **error)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);

  if (gimp_curves_config_load_cruft (tool->config, fp, error))
    {
      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                       GIMP_CURVE_SMOOTH);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_curves_tool_settings_save (GimpImageMapTool *image_map_tool,
                                gpointer          fp)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);

  return gimp_curves_config_save_cruft (tool->config, fp);
}

static void
curves_curve_callback (GimpCurve      *curve,
                       GimpCurvesTool *tool)
{
  GimpCurvesConfig *config = tool->config;

  if (curve != config->curve[config->channel])
    return;

  if (tool->xrange)
    {
      switch (config->channel)
        {
        case GIMP_HISTOGRAM_VALUE:
        case GIMP_HISTOGRAM_ALPHA:
        case GIMP_HISTOGRAM_RGB:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      config->curve[config->channel]->curve,
                                      config->curve[config->channel]->curve,
                                      config->curve[config->channel]->curve);
          break;

        case GIMP_HISTOGRAM_RED:
        case GIMP_HISTOGRAM_GREEN:
        case GIMP_HISTOGRAM_BLUE:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      config->curve[GIMP_HISTOGRAM_RED]->curve,
                                      config->curve[GIMP_HISTOGRAM_GREEN]->curve,
                                      config->curve[GIMP_HISTOGRAM_BLUE]->curve);
          break;
        }
    }

  if (GIMP_IMAGE_MAP_TOOL (tool)->drawable)
    gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static void
curves_channel_callback (GtkWidget      *widget,
                         GimpCurvesTool *tool)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      GimpCurvesConfig *config = tool->config;

      g_object_set (config,
                    "channel", value,
                    NULL);

      gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (tool->graph),
                                       config->channel);
      gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                                tool->col_value[config->channel]);

      gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->yrange),
                                  config->channel);

      gimp_curve_view_set_curve (GIMP_CURVE_VIEW (tool->graph),
                                 config->curve[config->channel]);

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                       config->curve[config->channel]->curve_type);

      curves_curve_callback (config->curve[config->channel], tool);
    }
}

static void
curves_channel_reset_callback (GtkWidget      *widget,
                               GimpCurvesTool *tool)
{
  gimp_curves_config_reset_channel (tool->config, tool->config->channel);
}

static gboolean
curves_menu_sensitivity (gint      value,
                         gpointer  data)
{
  GimpCurvesTool       *tool    = GIMP_CURVES_TOOL (data);
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
curves_curve_type_callback (GtkWidget      *widget,
                            GimpCurvesTool *tool)
{
  GimpCurvesConfig *config = tool->config;
  GimpCurveType     curve_type;

  gimp_radio_button_update (widget, &curve_type);

  if (config->curve[config->channel]->curve_type != curve_type)
    {
      gimp_curve_set_curve_type (config->curve[config->channel], curve_type);
    }
}
