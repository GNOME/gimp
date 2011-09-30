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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/threshold.h"

#include "gegl/gimpthresholdconfig.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogrambox.h"
#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimphistogramoptions.h"
#include "gimpthresholdtool.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       gimp_threshold_tool_finalize        (GObject           *object);

static gboolean   gimp_threshold_tool_initialize      (GimpTool          *tool,
                                                       GimpDisplay       *display,
                                                       GError           **error);

static GeglNode * gimp_threshold_tool_get_operation   (GimpImageMapTool  *im_tool,
                                                       GObject          **config);
static void       gimp_threshold_tool_map             (GimpImageMapTool  *im_tool);
static void       gimp_threshold_tool_dialog          (GimpImageMapTool  *im_tool);

static void       gimp_threshold_tool_config_notify   (GObject           *object,
                                                       GParamSpec        *pspec,
                                                       GimpThresholdTool *t_tool);

static void       gimp_threshold_tool_histogram_range (GimpHistogramView *view,
                                                       gint               start,
                                                       gint               end,
                                                       GimpThresholdTool *t_tool);
static void       gimp_threshold_tool_auto_clicked    (GtkWidget         *button,
                                                       GimpThresholdTool *t_tool);


G_DEFINE_TYPE (GimpThresholdTool, gimp_threshold_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_threshold_tool_parent_class


void
gimp_threshold_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_THRESHOLD_TOOL,
                GIMP_TYPE_HISTOGRAM_OPTIONS,
                gimp_histogram_options_gui,
                0,
                "gimp-threshold-tool",
                _("Threshold"),
                _("Threshold Tool: Reduce image to two colors using a threshold"),
                N_("_Threshold..."), NULL,
                NULL, GIMP_HELP_TOOL_THRESHOLD,
                GIMP_STOCK_TOOL_THRESHOLD,
                data);
}

static void
gimp_threshold_tool_class_init (GimpThresholdToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize             = gimp_threshold_tool_finalize;

  tool_class->initialize             = gimp_threshold_tool_initialize;

  im_tool_class->dialog_desc         = _("Apply Threshold");
  im_tool_class->settings_name       = "threshold";
  im_tool_class->import_dialog_title = _("Import Threshold Settings");
  im_tool_class->export_dialog_title = _("Export Threshold Settings");

  im_tool_class->get_operation       = gimp_threshold_tool_get_operation;
  im_tool_class->map                 = gimp_threshold_tool_map;
  im_tool_class->dialog              = gimp_threshold_tool_dialog;
}

static void
gimp_threshold_tool_init (GimpThresholdTool *t_tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (t_tool);

  t_tool->threshold = g_slice_new0 (Threshold);
  t_tool->histogram = gimp_histogram_new ();

  im_tool->apply_func = (GimpImageMapApplyFunc) threshold;
  im_tool->apply_data = t_tool->threshold;
}

static void
gimp_threshold_tool_finalize (GObject *object)
{
  GimpThresholdTool *t_tool = GIMP_THRESHOLD_TOOL (object);

  g_slice_free (Threshold, t_tool->threshold);

  if (t_tool->histogram)
    {
      gimp_histogram_unref (t_tool->histogram);
      t_tool->histogram = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_threshold_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpThresholdTool *t_tool   = GIMP_THRESHOLD_TOOL (tool);
  GimpImage         *image    = gimp_display_get_image (display);
  GimpDrawable      *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Threshold does not operate on indexed layers."));
      return FALSE;
    }

  gimp_config_reset (GIMP_CONFIG (t_tool->config));

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  gimp_drawable_calculate_histogram (drawable, t_tool->histogram);
  gimp_histogram_view_set_histogram (t_tool->histogram_box->view,
                                     t_tool->histogram);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));

  return TRUE;
}

static GeglNode *
gimp_threshold_tool_get_operation (GimpImageMapTool  *image_map_tool,
                                   GObject          **config)
{
  GimpThresholdTool *t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);
  GeglNode          *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:threshold",
                       NULL);

  t_tool->config = g_object_new (GIMP_TYPE_THRESHOLD_CONFIG, NULL);

  *config = G_OBJECT (t_tool->config);

  g_signal_connect_object (t_tool->config, "notify",
                           G_CALLBACK (gimp_threshold_tool_config_notify),
                           G_OBJECT (t_tool), 0);

  gegl_node_set (node,
                 "config", t_tool->config,
                 NULL);

  return node;
}

static void
gimp_threshold_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool   = GIMP_THRESHOLD_TOOL (image_map_tool);
  GimpDrawable      *drawable = image_map_tool->drawable;

  gimp_threshold_config_to_cruft (t_tool->config, t_tool->threshold,
                                  gimp_drawable_is_rgb (drawable));
}


/**********************/
/*  Threshold dialog  */
/**********************/

static void
gimp_threshold_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool   *t_tool       = GIMP_THRESHOLD_TOOL (image_map_tool);
  GimpToolOptions     *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GimpThresholdConfig *config       = t_tool->config;
  GtkWidget           *main_vbox;
  GtkWidget           *hbox;
  GtkWidget           *menu;
  GtkWidget           *box;
  GtkWidget           *button;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  menu = gimp_prop_enum_stock_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  box = gimp_histogram_box_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  t_tool->histogram_box = GIMP_HISTOGRAM_BOX (box);

  gimp_histogram_view_set_range (t_tool->histogram_box->view,
                                 config->low  * 255.999,
                                 config->high * 255.999);

  g_signal_connect (t_tool->histogram_box->view, "range-changed",
                    G_CALLBACK (gimp_threshold_tool_histogram_range),
                    t_tool);

  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       t_tool->histogram_box->view);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Auto"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Automatically adjust to optimal "
                                     "binarization threshold"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_threshold_tool_auto_clicked),
                    t_tool);
}

static void
gimp_threshold_tool_config_notify (GObject           *object,
                                   GParamSpec        *pspec,
                                   GimpThresholdTool *t_tool)
{
  GimpThresholdConfig *config = GIMP_THRESHOLD_CONFIG (object);

  if (! t_tool->histogram_box)
    return;

  gimp_histogram_view_set_range (t_tool->histogram_box->view,
                                 config->low  * 255.999,
                                 config->high * 255.999);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
}

static void
gimp_threshold_tool_histogram_range (GimpHistogramView *widget,
                                     gint               start,
                                     gint               end,
                                     GimpThresholdTool *t_tool)
{
  gdouble low  = start / 255.0;
  gdouble high = end   / 255.0;

  if (low  != t_tool->config->low ||
      high != t_tool->config->high)
    {
      g_object_set (t_tool->config,
                    "low",  low,
                    "high", high,
                    NULL);
    }
}

static void
gimp_threshold_tool_auto_clicked (GtkWidget         *button,
                                  GimpThresholdTool *t_tool)
{
  GimpDrawable *drawable = GIMP_IMAGE_MAP_TOOL (t_tool)->drawable;
  gdouble       low;

  low = gimp_histogram_get_threshold (t_tool->histogram,
                                      gimp_drawable_is_rgb (drawable) ?
                                      GIMP_HISTOGRAM_RGB :
                                      GIMP_HISTOGRAM_VALUE,
                                      0, 255);

  gimp_histogram_view_set_range (t_tool->histogram_box->view,
                                 low, 255.0);
}
