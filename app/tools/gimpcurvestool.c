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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/curves.h"
#include "base/gimphistogram.h"
#include "base/gimplut.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

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

static void     gimp_curves_tool_finalize       (GObject              *object);

static gboolean gimp_curves_tool_initialize     (GimpTool             *tool,
                                                 GimpDisplay          *display,
                                                 GError              **error);
static void     gimp_curves_tool_button_release (GimpTool             *tool,
                                                 GimpCoords           *coords,
                                                 guint32               time,
                                                 GdkModifierType       state,
                                                 GimpButtonReleaseType release_type,
                                                 GimpDisplay          *display);
static gboolean gimp_curves_tool_key_press      (GimpTool             *tool,
                                                 GdkEventKey          *kevent,
                                                 GimpDisplay          *display);
static void     gimp_curves_tool_oper_update    (GimpTool             *tool,
                                                 GimpCoords           *coords,
                                                 GdkModifierType       state,
                                                 gboolean              proximity,
                                                 GimpDisplay          *display);

static void     gimp_curves_tool_color_picked   (GimpColorTool        *color_tool,
                                                 GimpColorPickState    pick_state,
                                                 GimpImageType         sample_type,
                                                 GimpRGB              *color,
                                                 gint                  color_index);
static void     gimp_curves_tool_map            (GimpImageMapTool     *image_map_tool);
static void     gimp_curves_tool_dialog         (GimpImageMapTool     *image_map_tool);
static void     gimp_curves_tool_reset          (GimpImageMapTool     *image_map_tool);
static gboolean gimp_curves_tool_settings_load  (GimpImageMapTool     *image_map_tool,
                                                 gpointer              fp,
                                                 GError              **error);
static gboolean gimp_curves_tool_settings_save  (GimpImageMapTool     *image_map_tool,
                                                 gpointer              fp);

static void     curves_curve_callback           (GimpCurve            *curve,
                                                 GimpCurvesTool       *tool);
static void     curves_channel_callback         (GtkWidget            *widget,
                                                 GimpCurvesTool       *tool);
static void     curves_channel_reset_callback   (GtkWidget            *widget,
                                                 GimpCurvesTool       *tool);

static gboolean curves_menu_sensitivity         (gint                  value,
                                                 gpointer              data);

static void     curves_curve_type_callback      (GtkWidget            *widget,
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

  im_tool_class->map               = gimp_curves_tool_map;
  im_tool_class->dialog            = gimp_curves_tool_dialog;
  im_tool_class->reset             = gimp_curves_tool_reset;
  im_tool_class->settings_load     = gimp_curves_tool_settings_load;
  im_tool_class->settings_save     = gimp_curves_tool_settings_save;
}

static void
gimp_curves_tool_init (GimpCurvesTool *tool)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (tool->curve); i++)
    {
      tool->curve[i] = GIMP_CURVE (gimp_curve_new ("curves tool"));

      g_signal_connect_object (tool->curve[i], "dirty",
                               G_CALLBACK (curves_curve_callback),
                               tool, 0);
    }

  tool->lut     = gimp_lut_new ();
  tool->channel = GIMP_HISTOGRAM_VALUE;

  for (i = 0; i < G_N_ELEMENTS (tool->col_value); i++)
    tool->col_value[i] = -1;
}

static void
gimp_curves_tool_finalize (GObject *object)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (object);
  gint            i;

  for (i = 0; i < G_N_ELEMENTS (tool->curve); i++)
    g_object_unref (tool->curve[i]);

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

  for (i = 0; i < G_N_ELEMENTS (c_tool->curve); i++)
    gimp_curve_reset (c_tool->curve[i], TRUE);

  if (! c_tool->hist)
    c_tool->hist = gimp_histogram_new ();

  c_tool->channel = GIMP_HISTOGRAM_VALUE;
  c_tool->color   = gimp_drawable_is_rgb (drawable);
  c_tool->alpha   = gimp_drawable_has_alpha (drawable);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  /*  always pick colors  */
  gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                          GIMP_COLOR_TOOL_GET_OPTIONS (tool));

  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (c_tool->channel_menu),
                                      curves_menu_sensitivity, c_tool, NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (c_tool->channel_menu),
                                 c_tool->channel);

  /* FIXME: hack */
  if (! c_tool->color)
    c_tool->channel = (c_tool->channel == GIMP_HISTOGRAM_ALPHA) ? 1 : 0;

  gimp_drawable_calculate_histogram (drawable, c_tool->hist);
  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (c_tool->graph),
                                     c_tool->hist);
  gimp_curve_view_set_curve (GIMP_CURVE_VIEW (c_tool->graph),
                             c_tool->curve[c_tool->channel]);

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
  GimpCurvesTool *c_tool = GIMP_CURVES_TOOL (tool);

  if (state & GDK_SHIFT_MASK)
    {
      GimpCurve *curve = c_tool->curve[c_tool->channel];
      gint       closest;

      closest =
        gimp_curve_get_closest_point (curve,
                                      c_tool->col_value[c_tool->channel]);

      gimp_curve_view_set_selected (GIMP_CURVE_VIEW (c_tool->graph),
                                    closest);

      gimp_curve_set_point (curve,
                            closest,
                            c_tool->col_value[c_tool->channel],
                            curve->curve[c_tool->col_value[c_tool->channel]]);
    }
  else if (state & GDK_CONTROL_MASK)
    {
      gint i;

      for (i = 0; i < 5; i++)
        {
          GimpCurve *curve = c_tool->curve[i];
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
  GimpCurvesTool       *tool = GIMP_CURVES_TOOL (color_tool);
  GimpDrawable         *drawable;
  guchar                r, g, b, a;
  GimpHistogramChannel  channel;

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

  if (tool->color)
    channel = tool->channel;
  else
    channel = (tool->channel == 1) ? GIMP_HISTOGRAM_ALPHA : GIMP_HISTOGRAM_VALUE;

  gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                            tool->col_value[channel]);
}

static void
gimp_curves_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  Curves          curves;
  gint            i;

  for (i = 0; i < G_N_ELEMENTS (tool->curve); i++)
    gimp_curve_get_uchar (tool->curve[i], curves.curve[i]);

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) curves_lut_func,
                  &curves,
                  gimp_drawable_bytes (image_map_tool->drawable));

  gimp_image_map_apply (image_map_tool->image_map,
                        (GimpImageMapApplyFunc) gimp_lut_process,
                        tool->lut);
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
  GimpCurvesTool       *tool = GIMP_CURVES_TOOL (image_map_tool);
  GimpHistogramChannel  channel;

  for (channel =  GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_curve_reset (tool->curve[channel], FALSE);
    }
}

static gboolean
gimp_curves_tool_settings_load (GimpImageMapTool  *image_map_tool,
                                gpointer           fp,
                                GError           **error)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  FILE           *file = fp;
  gint            i, j;
  gint            fields;
  gchar           buf[50];
  gint            index[5][GIMP_CURVE_NUM_POINTS];
  gint            value[5][GIMP_CURVE_NUM_POINTS];

  if (! fgets (buf, sizeof (buf), file) ||
      strcmp (buf, "# GIMP Curves File\n") != 0)
    {
      g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                   _("not a GIMP Curves file"));
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        {
          fields = fscanf (file, "%d %d ", &index[i][j], &value[i][j]);
          if (fields != 2)
            {
              /*  FIXME: should have a helpful error message here  */
              g_printerr ("fields != 2");
              g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                           _("parse error"));
              return FALSE;
            }
        }
    }

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = tool->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        gimp_curve_set_point (curve, j, index[i][j], value[i][j]);

      gimp_data_thaw (GIMP_DATA (curve));
    }

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                   GIMP_CURVE_SMOOTH);

  return TRUE;
}

static gboolean
gimp_curves_tool_settings_save (GimpImageMapTool *image_map_tool,
                                gpointer          fp)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);
  FILE           *file = fp;
  gint            i, j;
  gint32          index;

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = tool->curve[i];

      if (curve->curve_type == GIMP_CURVE_FREE)
        {
          /* pick representative points from the curve and make them
           * control points
           */
          for (j = 0; j <= 8; j++)
            {
              index = CLAMP0255 (j * 32);

              curve->points[j * 2][0] = index;
              curve->points[j * 2][1] = curve->curve[index];
            }
        }

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        fprintf (file, "%d %d ",
                 curve->points[j][0],
                 curve->points[j][1]);

      fprintf (file, "\n");
    }

  return TRUE;
}

static void
curves_curve_callback (GimpCurve      *curve,
                       GimpCurvesTool *tool)
{
  if (curve != tool->curve[tool->channel])
    return;

  if (tool->xrange)
    {
      GimpHistogramChannel channel;

      if (tool->color)
        channel = tool->channel;
      else
        channel = ((tool->channel == 1) ?
                   GIMP_HISTOGRAM_ALPHA : GIMP_HISTOGRAM_VALUE);

      switch (channel)
        {
        case GIMP_HISTOGRAM_VALUE:
        case GIMP_HISTOGRAM_ALPHA:
        case GIMP_HISTOGRAM_RGB:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      tool->curve[tool->channel]->curve,
                                      tool->curve[tool->channel]->curve,
                                      tool->curve[tool->channel]->curve);
          break;

        case GIMP_HISTOGRAM_RED:
        case GIMP_HISTOGRAM_GREEN:
        case GIMP_HISTOGRAM_BLUE:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      tool->curve[GIMP_HISTOGRAM_RED]->curve,
                                      tool->curve[GIMP_HISTOGRAM_GREEN]->curve,
                                      tool->curve[GIMP_HISTOGRAM_BLUE]->curve);
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
      tool->channel = value;
      gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (tool->graph),
                                       tool->channel);
      gimp_curve_view_set_xpos (GIMP_CURVE_VIEW (tool->graph),
                                tool->col_value[tool->channel]);

      gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->yrange), tool->channel);

      /* FIXME: hack */
      if (! tool->color)
        tool->channel = (tool->channel == GIMP_HISTOGRAM_ALPHA) ? 1 : 0;

      gimp_curve_view_set_curve (GIMP_CURVE_VIEW (tool->graph),
                                 tool->curve[tool->channel]);

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                       tool->curve[tool->channel]->curve_type);

      curves_curve_callback (tool->curve[tool->channel], tool);
    }
}

static void
curves_channel_reset_callback (GtkWidget      *widget,
                               GimpCurvesTool *tool)
{
  gimp_curve_reset (tool->curve[tool->channel], FALSE);
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
  GimpCurveType curve_type;

  gimp_radio_button_update (widget, &curve_type);

  if (tool->curve[tool->channel]->curve_type != curve_type)
    {
      gimp_curve_set_curve_type (tool->curve[tool->channel], curve_type);
    }
}
