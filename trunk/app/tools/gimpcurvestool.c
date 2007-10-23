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
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "base/curves.h"
#include "base/gimphistogram.h"
#include "base/gimplut.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcolorbar.h"
#include "widgets/gimpcursor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimpcurvestool.h"
#include "gimphistogramoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define XRANGE   (1 << 0)
#define YRANGE   (1 << 1)
#define GRAPH    (1 << 2)
#define ALL      (XRANGE | YRANGE | GRAPH)

#define GRAPH_SIZE    256
#define BAR_SIZE       12
#define RADIUS          4
#define MIN_DISTANCE    8


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

static void     curves_add_point                (GimpCurvesTool       *tool,
                                                 gint                  channel);
static gboolean curves_key_press                (GimpCurvesTool       *tool,
                                                 GdkEventKey          *kevent);
static void     curves_update                   (GimpCurvesTool       *tool,
                                                 gint                  update);

static void     curves_channel_callback         (GtkWidget            *widget,
                                                 GimpCurvesTool       *tool);
static void     curves_channel_reset_callback   (GtkWidget            *widget,
                                                 GimpCurvesTool       *tool);

static gboolean curves_menu_sensitivity         (gint                  value,
                                                 gpointer              data);

static void     curves_curve_type_callback      (GtkWidget            *widget,
                                                 GimpCurvesTool       *tool);
static gboolean curves_graph_events             (GtkWidget            *widget,
                                                 GdkEvent             *event,
                                                 GimpCurvesTool       *tool);
static gboolean curves_graph_expose             (GtkWidget            *widget,
                                                 GdkEventExpose       *eevent,
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

  tool->curves  = g_slice_new0 (Curves);
  tool->lut     = gimp_lut_new ();
  tool->channel = GIMP_HISTOGRAM_VALUE;

  curves_init (tool->curves);

  for (i = 0; i < G_N_ELEMENTS (tool->col_value); i++)
    tool->col_value[i] = -1;

  tool->cursor_x = -1;
  tool->cursor_y = -1;
}

static void
gimp_curves_tool_finalize (GObject *object)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (object);

  g_slice_free (Curves, tool->curves);
  gimp_lut_free (tool->lut);

  if (tool->hist)
    {
      gimp_histogram_free (tool->hist);
      tool->hist = NULL;
    }

  if (tool->cursor_layout)
    {
      g_object_unref (tool->cursor_layout);
      tool->cursor_layout = NULL;
    }

  if (tool->xpos_layout)
    {
      g_object_unref (tool->xpos_layout);
      tool->xpos_layout = NULL;
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

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Curves does not operate on indexed layers."));
      return FALSE;
    }

  if (! c_tool->hist)
    c_tool->hist = gimp_histogram_new ();

  curves_init (c_tool->curves);

  c_tool->channel = GIMP_HISTOGRAM_VALUE;
  c_tool->color   = gimp_drawable_is_rgb (drawable);
  c_tool->alpha   = gimp_drawable_has_alpha (drawable);

  c_tool->selected = 0;
  c_tool->grabbed  = FALSE;
  c_tool->last     = 0;

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

  curves_update (c_tool, ALL);

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
      curves_add_point (c_tool, c_tool->channel);
      curves_calculate_curve (c_tool->curves, c_tool->channel);
      curves_update (c_tool, GRAPH | XRANGE);
    }
  else if (state & GDK_CONTROL_MASK)
    {
      gint i;

      for (i = 0; i < 5; i++)
        {
          curves_add_point (c_tool, i);
          curves_calculate_curve (c_tool->curves, c_tool->channel);
        }

      curves_update (c_tool, GRAPH | XRANGE);
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
  return curves_key_press (GIMP_CURVES_TOOL (tool), kevent);
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

  curves_update (tool, GRAPH);
}

static void
curves_add_point (GimpCurvesTool *tool,
                  gint            channel)
{
  Curves *curves        = tool->curves;
  gint    closest_point = 0;
  gint    distance;
  gint    curvex;
  gint    i;

  switch (curves->curve_type[channel])
    {
    case GIMP_CURVE_SMOOTH:
      curvex   = tool->col_value[channel];
      distance = G_MAXINT;

      for (i = 0; i < CURVES_NUM_POINTS; i++)
        {
          if (curves->points[channel][i][0] != -1)
            if (abs (curvex -curves->points[channel][i][0]) < distance)
              {
                distance = abs (curvex - curves->points[channel][i][0]);
                closest_point = i;
              }
        }

      if (distance > MIN_DISTANCE)
        closest_point = (curvex + 8) / 16;

      curves->points[channel][closest_point][0] = curvex;
      curves->points[channel][closest_point][1] = curves->curve[channel][curvex];

      tool->selected = closest_point;
      break;

    case GIMP_CURVE_FREE:
      /* do nothing for free form curves */
      break;
    }
}

static gboolean
curves_key_press (GimpCurvesTool *tool,
                  GdkEventKey    *kevent)
{
  Curves  *curves = tool->curves;
  gint     i      = tool->selected;
  gint     y      = curves->points[tool->channel][i][1];
  gboolean update = FALSE;

  if (tool->grabbed || curves->curve_type[tool->channel] == GIMP_CURVE_FREE)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_Left:
      for (i = i - 1; i >= 0 && !update; i--)
        {
          if (curves->points[tool->channel][i][0] != -1)
            {
              tool->selected = i;
              update = TRUE;
            }
        }
      break;

    case GDK_Right:
      for (i = i + 1; i < CURVES_NUM_POINTS && !update; i++)
        {
          if (curves->points[tool->channel][i][0] != -1)
            {
              tool->selected = i;
              update = TRUE;
            }
        }
      break;

    case GDK_Up:
      if (y < 255)
        {
          y = y + (kevent->state & GDK_SHIFT_MASK ? 16 : 1);
          curves->points[tool->channel][i][1] = MIN (y, 255);
          curves_calculate_curve (curves, tool->channel);
          update = TRUE;
        }
      break;

    case GDK_Down:
      if (y > 0)
        {
          y = y - (kevent->state & GDK_SHIFT_MASK ? 16 : 1);
          curves->points[tool->channel][i][1] = MAX (y, 0);
          curves_calculate_curve (curves, tool->channel);
          update = TRUE;
        }
      break;

    default:
      break;
    }

  if (update)
    {
      curves_update (tool, GRAPH);
      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }

  return update;
}

static void
gimp_curves_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *tool = GIMP_CURVES_TOOL (image_map_tool);

  gimp_lut_setup (tool->lut,
                  (GimpLutFunc) curves_lut_func,
                  tool->curves,
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

  tool->graph = gimp_histogram_view_new (FALSE);
  gtk_widget_set_size_request (tool->graph,
                               GRAPH_SIZE + RADIUS * 2,
                               GRAPH_SIZE + RADIUS * 2);
  GTK_WIDGET_SET_FLAGS (tool->graph, GTK_CAN_FOCUS);
  gtk_widget_add_events (tool->graph, (GDK_BUTTON_PRESS_MASK   |
                                       GDK_BUTTON_RELEASE_MASK |
                                       GDK_POINTER_MOTION_MASK |
                                       GDK_KEY_PRESS_MASK      |
                                       GDK_LEAVE_NOTIFY_MASK));
  g_object_set (tool->graph,
                "border-width", RADIUS,
                "subdivisions", 1,
                NULL);
  GIMP_HISTOGRAM_VIEW (tool->graph)->light_histogram = TRUE;
  gtk_container_add (GTK_CONTAINER (frame), tool->graph);
  gtk_widget_show (tool->graph);

  g_signal_connect (tool->graph, "event",
                    G_CALLBACK (curves_graph_events),
                    tool);
  g_signal_connect_after (tool->graph, "expose-event",
                          G_CALLBACK (curves_graph_expose),
                          tool);


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

  tool->grabbed = FALSE;

  for (channel =  GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    curves_channel_reset (tool->curves, channel);

  curves_update (tool, XRANGE | GRAPH);
}

static gboolean
gimp_curves_tool_settings_load (GimpImageMapTool  *image_map_tool,
                                gpointer           fp,
                                GError           **error)
{
  GimpCurvesTool *tool   = GIMP_CURVES_TOOL (image_map_tool);
  Curves         *curves = tool->curves;
  FILE           *file   = fp;
  gint            i, j;
  gint            fields;
  gchar           buf[50];
  gint            index[5][CURVES_NUM_POINTS];
  gint            value[5][CURVES_NUM_POINTS];

  if (! fgets (buf, sizeof (buf), file) ||
      strcmp (buf, "# GIMP Curves File\n") != 0)
    {
      g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                   _("not a GIMP Levels file"));
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < CURVES_NUM_POINTS; j++)
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
      curves->curve_type[i] = GIMP_CURVE_SMOOTH;

      for (j = 0; j < CURVES_NUM_POINTS; j++)
        {
          curves->points[i][j][0] = index[i][j];
          curves->points[i][j][1] = value[i][j];
        }
    }

  for (i = 0; i < 5; i++)
    curves_calculate_curve (curves, i);

  curves_update (tool, ALL);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                   GIMP_CURVE_SMOOTH);

  return TRUE;
}

static gboolean
gimp_curves_tool_settings_save (GimpImageMapTool *image_map_tool,
                                gpointer          fp)
{
  GimpCurvesTool *tool   = GIMP_CURVES_TOOL (image_map_tool);
  Curves         *curves = tool->curves;
  FILE           *file   = fp;
  gint            i, j;
  gint32          index;

  for (i = 0; i < 5; i++)
    if (curves->curve_type[i] == GIMP_CURVE_FREE)
      {
        /*  pick representative points from the curve
            and make them control points  */
        for (j = 0; j <= 8; j++)
          {
            index = CLAMP0255 (j * 32);
            curves->points[i][j * 2][0] = index;
            curves->points[i][j * 2][1] = curves->curve[i][index];
          }
      }

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < CURVES_NUM_POINTS; j++)
        fprintf (file, "%d %d ",
                 curves->points[i][j][0],
                 curves->points[i][j][1]);

      fprintf (file, "\n");
    }

  return TRUE;
}

/* TODO: preview alpha channel stuff correctly.  -- austin, 20/May/99 */
static void
curves_update (GimpCurvesTool *tool,
               gint            update)
{
  GimpHistogramChannel  channel;
  Curves               *curves = tool->curves;

  if (tool->color)
    {
      channel = tool->channel;
    }
  else
    {
      /* FIXME: hack */
      if (tool->channel == 1)
        channel = GIMP_HISTOGRAM_ALPHA;
      else
        channel = GIMP_HISTOGRAM_VALUE;
    }

  if (update & GRAPH)
    gtk_widget_queue_draw (tool->graph);

  if (update & XRANGE)
    {
      switch (channel)
        {
        case GIMP_HISTOGRAM_VALUE:
        case GIMP_HISTOGRAM_ALPHA:
        case GIMP_HISTOGRAM_RGB:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      curves->curve[tool->channel],
                                      curves->curve[tool->channel],
                                      curves->curve[tool->channel]);
          break;

        case GIMP_HISTOGRAM_RED:
        case GIMP_HISTOGRAM_GREEN:
        case GIMP_HISTOGRAM_BLUE:
          gimp_color_bar_set_buffers (GIMP_COLOR_BAR (tool->xrange),
                                      curves->curve[GIMP_HISTOGRAM_RED],
                                      curves->curve[GIMP_HISTOGRAM_GREEN],
                                      curves->curve[GIMP_HISTOGRAM_BLUE]);
          break;
        }
    }

  if (update & YRANGE)
    {
      gimp_color_bar_set_channel (GIMP_COLOR_BAR (tool->yrange), channel);
    }
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

      /* FIXME: hack */
      if (! tool->color)
        tool->channel = (tool->channel == GIMP_HISTOGRAM_ALPHA) ? 1 : 0;

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (tool->curve_type),
                                       tool->curves->curve_type[tool->channel]);

      curves_update (tool, ALL);
    }
}

static void
curves_channel_reset_callback (GtkWidget      *widget,
                               GimpCurvesTool *tool)
{
  tool->grabbed = FALSE;

  curves_channel_reset (tool->curves, tool->channel);

  curves_update (tool, XRANGE | GRAPH);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
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
  GimpCurveType  curve_type;
  Curves        *curves = tool->curves;

  gimp_radio_button_update (widget, &curve_type);

  if (curves->curve_type[tool->channel] != curve_type)
    {
      curves->curve_type[tool->channel] = curve_type;

      if (curve_type == GIMP_CURVE_SMOOTH)
        {
          gint   i;
          gint32 index;

          /*  pick representative points from the curve
           *  and make them control points
           */
          for (i = 0; i <= 8; i++)
            {
              index = CLAMP0255 (i * 32);
              curves->points[tool->channel][i * 2][0] = index;
              curves->points[tool->channel][i * 2][1] = curves->curve[tool->channel][index];
            }
        }

      curves_calculate_curve (curves, tool->channel);

      curves_update (tool, XRANGE | GRAPH);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
    }
}

static void
curves_set_cursor (GimpCurvesTool *tool,
                   GimpCursorType  new_cursor)
{
  static GimpCursorType cursor_type = GDK_TOP_LEFT_ARROW;

  if (new_cursor != cursor_type)
    {
      Gimp *gimp = GIMP_TOOL (tool)->tool_info->gimp;

      cursor_type = new_cursor;

      gimp_cursor_set (tool->graph,
                       GIMP_GUI_CONFIG (gimp->config)->cursor_format,
                       cursor_type,
                       GIMP_TOOL_CURSOR_NONE,
                       GIMP_CURSOR_MODIFIER_NONE);
    }
}

static gboolean
curves_graph_events (GtkWidget      *widget,
                     GdkEvent       *event,
                     GimpCurvesTool *tool)
{
  Curves         *curves     = tool->curves;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GimpCursorType  new_cursor = GDK_X_CURSOR;
  gint            i;
  gint            tx, ty;
  gint            x, y;
  gint            width, height;
  gint            closest_point;
  gint            distance;
  gint            x1, x2, y1, y2;

  width  = widget->allocation.width  - 2 * RADIUS;
  height = widget->allocation.height - 2 * RADIUS;

  /*  get the pointer position  */
  gdk_window_get_pointer (tool->graph->window, &tx, &ty, NULL);

  x = ROUND (((gdouble) (tx - RADIUS) / (gdouble) width)  * 255.0);
  y = ROUND (((gdouble) (ty - RADIUS) / (gdouble) height) * 255.0);

  x = CLAMP0255 (x);
  y = CLAMP0255 (y);

  distance = G_MAXINT;
  for (i = 0, closest_point = 0; i < CURVES_NUM_POINTS; i++)
    {
      if (curves->points[tool->channel][i][0] != -1)
        if (abs (x - curves->points[tool->channel][i][0]) < distance)
          {
            distance = abs (x - curves->points[tool->channel][i][0]);
            closest_point = i;
          }
    }

  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button != 1)
        return TRUE;

      tool->grabbed = TRUE;

      curves_set_cursor (tool, GDK_TCROSS);

      switch (curves->curve_type[tool->channel])
        {
        case GIMP_CURVE_SMOOTH:
          /*  determine the leftmost and rightmost points  */
          tool->leftmost = -1;
          for (i = closest_point - 1; i >= 0; i--)
            if (curves->points[tool->channel][i][0] != -1)
              {
                tool->leftmost = curves->points[tool->channel][i][0];
                break;
              }

          tool->rightmost = 256;
          for (i = closest_point + 1; i < CURVES_NUM_POINTS; i++)
            if (curves->points[tool->channel][i][0] != -1)
              {
                tool->rightmost = curves->points[tool->channel][i][0];
                break;
              }

          tool->selected = closest_point;
          curves->points[tool->channel][tool->selected][0] = x;
          curves->points[tool->channel][tool->selected][1] = 255 - y;

          gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
          break;

        case GIMP_CURVE_FREE:
          curves->curve[tool->channel][x] = 255 - y;
          tool->selected = x;
          tool->last     = y;
          break;
        }

      curves_calculate_curve (curves, tool->channel);
      curves_update (tool, XRANGE | GRAPH);

      if (! GTK_WIDGET_HAS_FOCUS (widget))
        gtk_widget_grab_focus (widget);

      return TRUE;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button != 1)
        return TRUE;

      tool->grabbed = FALSE;

      curves_set_cursor (tool, GDK_FLEUR);

      if (curves->curve_type[tool->channel] == GIMP_CURVE_FREE)
        gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));

      return TRUE;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      switch (curves->curve_type[tool->channel])
        {
        case GIMP_CURVE_SMOOTH:
          /*  If no point is grabbed...  */
          if (! tool->grabbed)
            {
              if (curves->points[tool->channel][closest_point][0] != -1)
                new_cursor = GDK_FLEUR;
              else
                new_cursor = GDK_TCROSS;
            }
          /*  Else, drag the grabbed point  */
          else
            {
              new_cursor = GDK_TCROSS;

              curves->points[tool->channel][tool->selected][0] = -1;

              if (x > tool->leftmost && x < tool->rightmost)
                {
                  closest_point = (x + 8) / 16;
                  if (curves->points[tool->channel][closest_point][0] == -1)
                    tool->selected = closest_point;

                  curves->points[tool->channel][tool->selected][0] = x;
                  curves->points[tool->channel][tool->selected][1] = 255 - y;
                }

              curves_calculate_curve (curves, tool->channel);

              gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
            }
          break;

        case GIMP_CURVE_FREE:
          if (tool->grabbed)
            {
              if (tool->selected > x)
                {
                  x1 = x;
                  x2 = tool->selected;
                  y1 = y;
                  y2 = tool->last;
                }
              else
                {
                  x1 = tool->selected;
                  x2 = x;
                  y1 = tool->last;
                  y2 = y;
                }

              if (x2 != x1)
                {
                  for (i = x1; i <= x2; i++)
                    curves->curve[tool->channel][i] =
                      255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
                }
              else
                {
                  curves->curve[tool->channel][x] = 255 - y;
                }

              tool->selected = x;
              tool->last     = y;
            }

          if (mevent->state & GDK_BUTTON1_MASK)
            new_cursor = GDK_TCROSS;
          else
            new_cursor = GDK_PENCIL;

          break;
        }

      curves_set_cursor (tool, new_cursor);

      tool->cursor_x = x;
      tool->cursor_y = y;

      curves_update (tool, XRANGE | GRAPH);
      return TRUE;

    case GDK_KEY_PRESS:
      if (curves_key_press (tool, (GdkEventKey *) event))
        {
          curves_set_cursor (tool, GDK_TCROSS);
          return TRUE;
        }
      break;

    case GDK_LEAVE_NOTIFY:
      tool->cursor_x = -1;
      tool->cursor_y = -1;

      curves_update (tool, GRAPH);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
curves_print_loc (GimpCurvesTool *tool)
{
  gchar buf[32];
  gint  x, y;
  gint  w, h;

  if (tool->cursor_x < 0 || tool->cursor_x > 255 ||
      tool->cursor_y < 0 || tool->cursor_y > 255)
    return;

  if (! tool->cursor_layout)
    {
      tool->cursor_layout = gtk_widget_create_pango_layout (tool->graph,
                                                            "x:888 y:888");
      pango_layout_get_pixel_extents (tool->cursor_layout,
                                      NULL, &tool->cursor_rect);
    }

  x = RADIUS * 2 + 2;
  y = RADIUS * 2 + 2;
  w = tool->cursor_rect.width  + 4;
  h = tool->cursor_rect.height + 4;

  gdk_draw_rectangle (tool->graph->window,
                      tool->graph->style->base_gc[GTK_STATE_ACTIVE],
                      TRUE,
                      x, y, w + 1, h + 1);
  gdk_draw_rectangle (tool->graph->window,
                      tool->graph->style->text_gc[GTK_STATE_NORMAL],
                      FALSE,
                      x, y, w, h);

  g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
              tool->cursor_x, 255 - tool->cursor_y);
  pango_layout_set_text (tool->cursor_layout, buf, 11);

  gdk_draw_layout (tool->graph->window,
                   tool->graph->style->text_gc[GTK_STATE_ACTIVE],
                   x + 2, y + 2,
                   tool->cursor_layout);
}

static gboolean
curves_graph_expose (GtkWidget      *widget,
                     GdkEventExpose *eevent,
                     GimpCurvesTool *tool)
{
  Curves               *curves = tool->curves;
  GimpHistogramChannel  channel;
  gint                  width;
  gint                  height;
  gint                  x, y, i;
  GdkPoint              points[256];
  GdkGC                *graph_gc;

  width  = widget->allocation.width  - 2 * RADIUS;
  height = widget->allocation.height - 2 * RADIUS;

  if (width < 1 || height < 1)
    return FALSE;

  if (tool->color)
    {
      channel = tool->channel;
    }
  else
    {
      /* FIXME: hack */
      if (tool->channel == 1)
        channel = GIMP_HISTOGRAM_ALPHA;
      else
        channel = GIMP_HISTOGRAM_VALUE;
    }

  /*  Draw the grid lines  */
  for (i = 1; i < 4; i++)
    {
      gdk_draw_line (widget->window,
                     tool->graph->style->dark_gc[GTK_STATE_NORMAL],
                     RADIUS,
                     RADIUS + i * (height / 4),
                     RADIUS + width - 1,
                     RADIUS + i * (height / 4));
      gdk_draw_line (widget->window,
                     tool->graph->style->dark_gc[GTK_STATE_NORMAL],
                     RADIUS + i * (width / 4),
                     RADIUS,
                     RADIUS + i * (width / 4),
                     RADIUS + height - 1);
    }

  /*  Draw the curve  */
  graph_gc = tool->graph->style->text_gc[GTK_STATE_NORMAL];

  for (i = 0; i < 256; i++)
    {
      x = i;
      y = 255 - curves->curve[tool->channel][x];

      points[i].x = RADIUS + ROUND ((gdouble) width  * x / 256.0);
      points[i].y = RADIUS + ROUND ((gdouble) height * y / 256.0);
    }

  gdk_draw_lines (widget->window, graph_gc, points, 256);

  if (curves->curve_type[tool->channel] == GIMP_CURVE_SMOOTH)
    {
      /*  Draw the points  */
      for (i = 0; i < CURVES_NUM_POINTS; i++)
        {
          x = curves->points[tool->channel][i][0];
          if (x < 0)
            continue;

          y = 255 - curves->points[tool->channel][i][1];

          gdk_draw_arc (widget->window, graph_gc,
                        TRUE,
                        ROUND ((gdouble) width  * x / 256.0),
                        ROUND ((gdouble) height * y / 256.0),
                        RADIUS * 2, RADIUS * 2, 0, 23040);

          if (i == tool->selected)
            gdk_draw_arc (widget->window,
                          tool->graph->style->base_gc[GTK_STATE_NORMAL],
                          TRUE,
                          ROUND ((gdouble) width  * x / 256.0) + 2,
                          ROUND ((gdouble) height * y / 256.0) + 2,
                          (RADIUS - 2) * 2, (RADIUS - 2) * 2, 0, 23040);
        }
    }

  if (tool->col_value[channel] >= 0)
    {
      gchar  buf[32];

      /* draw the color line */
      gdk_draw_line (widget->window,
                     graph_gc,
                     RADIUS +
                     ROUND ((gdouble) width *
                            (tool->col_value[channel]) / 256.0),
                     RADIUS,
                     RADIUS +
                     ROUND ((gdouble) width *
                            (tool->col_value[channel]) / 256.0),
                     height + RADIUS - 1);

      /* and xpos indicator */
      g_snprintf (buf, sizeof (buf), "x:%d", tool->col_value[channel]);

      if (! tool->xpos_layout)
        tool->xpos_layout = gtk_widget_create_pango_layout (tool->graph, buf);
      else
        pango_layout_set_text (tool->xpos_layout, buf, -1);

      pango_layout_get_pixel_size (tool->xpos_layout, &x, &y);

      if ((tool->col_value[channel] + RADIUS) < 127)
        x = RADIUS + 4;
      else
        x = -(x + 2);

      gdk_draw_layout (widget->window,
                       graph_gc,
                       RADIUS +
                       ROUND (((gdouble) width *
                               (tool->col_value[channel])) / 256.0 + x),
                       height - y - 2,
                       tool->xpos_layout);
    }

  curves_print_loc (tool);

  return FALSE;
}
