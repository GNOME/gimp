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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/threshold.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogrambox.h"
#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimphistogramoptions.h"
#include "gimpthresholdtool.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_threshold_tool_finalize        (GObject           *object);

static gboolean gimp_threshold_tool_initialize      (GimpTool          *tool,
                                                     GimpDisplay       *display,
                                                     GError           **error);

static void     gimp_threshold_tool_map             (GimpImageMapTool  *im_tool);
static void     gimp_threshold_tool_dialog          (GimpImageMapTool  *im_tool);
static void     gimp_threshold_tool_reset           (GimpImageMapTool  *im_tool);

static void     gimp_threshold_tool_histogram_range (GimpHistogramView *view,
                                                     gint               start,
                                                     gint               end,
                                                     GimpThresholdTool *t_tool);
static void     gimp_threshold_tool_auto_clicked    (GtkWidget         *button,
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

  object_class->finalize    = gimp_threshold_tool_finalize;

  tool_class->initialize    = gimp_threshold_tool_initialize;

  im_tool_class->shell_desc = _("Apply Threshold");

  im_tool_class->map        = gimp_threshold_tool_map;
  im_tool_class->dialog     = gimp_threshold_tool_dialog;
  im_tool_class->reset      = gimp_threshold_tool_reset;
}

static void
gimp_threshold_tool_init (GimpThresholdTool *t_tool)
{
  t_tool->threshold = g_slice_new0 (Threshold);
  t_tool->hist      = NULL;

  t_tool->threshold->low_threshold  = 127;
  t_tool->threshold->high_threshold = 255;
}

static void
gimp_threshold_tool_finalize (GObject *object)
{
  GimpThresholdTool *t_tool = GIMP_THRESHOLD_TOOL (object);

  g_slice_free (Threshold, t_tool->threshold);

  if (t_tool->hist)
    {
      gimp_histogram_free (t_tool->hist);
      t_tool->hist = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_threshold_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpThresholdTool *t_tool   = GIMP_THRESHOLD_TOOL (tool);
  GimpDrawable      *drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Threshold does not operate on indexed layers."));
      return FALSE;
    }

  if (!t_tool->hist)
    t_tool->hist = gimp_histogram_new ();

  t_tool->threshold->color          = gimp_drawable_is_rgb (drawable);
  t_tool->threshold->low_threshold  = 127;
  t_tool->threshold->high_threshold = 255;

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  gimp_drawable_calculate_histogram (drawable, t_tool->hist);

  g_signal_handlers_block_by_func (t_tool->histogram_box->view,
                                   gimp_threshold_tool_histogram_range,
                                   t_tool);
  gimp_histogram_view_set_histogram (t_tool->histogram_box->view,
                                     t_tool->hist);
  gimp_histogram_view_set_range (t_tool->histogram_box->view,
                                 t_tool->threshold->low_threshold,
                                 t_tool->threshold->high_threshold);
  g_signal_handlers_unblock_by_func (t_tool->histogram_box->view,
                                     gimp_threshold_tool_histogram_range,
                                     t_tool);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));

  return TRUE;
}

static void
gimp_threshold_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);

  gimp_image_map_apply (image_map_tool->image_map,
                        (GimpImageMapApplyFunc) threshold,
                        t_tool->threshold);
}


/**********************/
/*  Threshold dialog  */
/**********************/

static void
gimp_threshold_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool       = GIMP_THRESHOLD_TOOL (image_map_tool);
  GimpToolOptions   *tool_options = GIMP_TOOL_GET_OPTIONS (image_map_tool);
  GtkWidget         *vbox;
  GtkWidget         *hbox;
  GtkWidget         *menu;
  GtkWidget         *box;
  GtkWidget         *button;

  vbox = image_map_tool->main_vbox;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  menu = gimp_prop_enum_stock_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  box = gimp_histogram_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  t_tool->histogram_box = GIMP_HISTOGRAM_BOX (box);

  g_signal_connect (t_tool->histogram_box->view, "range-changed",
                    G_CALLBACK (gimp_threshold_tool_histogram_range),
                    t_tool);

  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       t_tool->histogram_box->view);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
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
gimp_threshold_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpThresholdTool *t_tool = GIMP_THRESHOLD_TOOL (image_map_tool);

  gimp_histogram_view_set_range (t_tool->histogram_box->view, 127.0, 255.0);
}

static void
gimp_threshold_tool_histogram_range (GimpHistogramView *widget,
                                     gint               start,
                                     gint               end,
                                     GimpThresholdTool *t_tool)
{
  if (start != t_tool->threshold->low_threshold ||
      end   != t_tool->threshold->high_threshold)
    {
      t_tool->threshold->low_threshold  = start;
      t_tool->threshold->high_threshold = end;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (t_tool));
    }
}

static void
gimp_threshold_tool_auto_clicked (GtkWidget         *button,
                                  GimpThresholdTool *t_tool)
{
  gdouble low_threshold;

  low_threshold =
    gimp_histogram_get_threshold (t_tool->hist,
                                  (t_tool->threshold->color ?
                                   GIMP_HISTOGRAM_RGB : GIMP_HISTOGRAM_VALUE),
                                  0, 255);

  gimp_histogram_view_set_range (t_tool->histogram_box->view,
                                 low_threshold, 255.0);
}
