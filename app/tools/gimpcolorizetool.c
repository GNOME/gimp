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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/colorize.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpcolorizetool.h"
#include "gimpimagemapoptions.h"

#include "gimp-intl.h"


#define SLIDER_WIDTH 200


/*  local function prototypes  */

static void       gimp_colorize_tool_finalize      (GObject          *object);

static gboolean   gimp_colorize_tool_initialize    (GimpTool         *tool,
                                                    GimpDisplay      *display,
                                                    GError          **error);

static GeglNode * gimp_colorize_tool_get_operation (GimpImageMapTool  *im_tool);
static void       gimp_colorize_tool_map           (GimpImageMapTool *im_tool);
static void       gimp_colorize_tool_dialog        (GimpImageMapTool *im_tool);
static void       gimp_colorize_tool_reset         (GimpImageMapTool *im_tool);

static void       colorize_update_sliders          (GimpColorizeTool *col_tool);
static void       colorize_hue_changed             (GtkAdjustment    *adj,
                                                    GimpColorizeTool *col_tool);
static void       colorize_saturation_changed      (GtkAdjustment    *adj,
                                                    GimpColorizeTool *col_tool);
static void       colorize_lightness_changed       (GtkAdjustment    *adj,
                                                    GimpColorizeTool *col_tool);


G_DEFINE_TYPE (GimpColorizeTool, gimp_colorize_tool, GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_colorize_tool_parent_class


void
gimp_colorize_tool_register (GimpToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (GIMP_TYPE_COLORIZE_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-colorize-tool",
                _("Colorize"),
                _("Colorize Tool: Colorize the image"),
                N_("Colori_ze..."), NULL,
                NULL, GIMP_HELP_TOOL_COLORIZE,
                GIMP_STOCK_TOOL_COLORIZE,
                data);
}

static void
gimp_colorize_tool_class_init (GimpColorizeToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize       = gimp_colorize_tool_finalize;

  tool_class->initialize       = gimp_colorize_tool_initialize;

  im_tool_class->shell_desc    = _("Colorize the Image");

  im_tool_class->get_operation = gimp_colorize_tool_get_operation;
  im_tool_class->map           = gimp_colorize_tool_map;
  im_tool_class->dialog        = gimp_colorize_tool_dialog;
  im_tool_class->reset         = gimp_colorize_tool_reset;
}

static void
gimp_colorize_tool_init (GimpColorizeTool *col_tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (col_tool);

  col_tool->colorize = g_slice_new0 (Colorize);

  colorize_init (col_tool->colorize);

  im_tool->apply_func = (GimpImageMapApplyFunc) colorize;
  im_tool->apply_data = col_tool->colorize;
}

static void
gimp_colorize_tool_finalize (GObject *object)
{
  GimpColorizeTool *col_tool = GIMP_COLORIZE_TOOL (object);

  g_slice_free (Colorize, col_tool->colorize);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_colorize_tool_initialize (GimpTool     *tool,
                               GimpDisplay  *display,
                               GError      **error)
{
  GimpColorizeTool *col_tool = GIMP_COLORIZE_TOOL (tool);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Colorize operates only on RGB color layers."));
      return FALSE;
    }

  colorize_init (col_tool->colorize);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  colorize_update_sliders (col_tool);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));

  return TRUE;
}

static GeglNode *
gimp_colorize_tool_get_operation (GimpImageMapTool *im_tool)
{
  return g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp-colorize",
                       NULL);
}

static void
gimp_colorize_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpColorizeTool *col_tool = GIMP_COLORIZE_TOOL (image_map_tool);

  gegl_node_set (image_map_tool->operation,
                 "hue",        col_tool->colorize->hue,
                 "saturation", col_tool->colorize->saturation,
                 "lightness",  col_tool->colorize->lightness,
                 NULL);

  colorize_calculate (col_tool->colorize);
}


/***************************/
/*  Hue-Saturation dialog  */
/***************************/

static void
gimp_colorize_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpColorizeTool *col_tool = GIMP_COLORIZE_TOOL (image_map_tool);
  GtkWidget        *table;
  GtkWidget        *slider;
  GtkWidget        *frame;
  GtkWidget        *vbox;
  GtkObject        *data;

  frame = gimp_frame_new (_("Select Color"));
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  The table containing sliders  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Create the hue scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                               _("_Hue:"), SLIDER_WIDTH, -1,
                               0.0, 0.0, 360.0, 1.0, 15.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  col_tool->hue_data = GTK_ADJUSTMENT (data);
  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (colorize_hue_changed),
                    col_tool);

  /*  Create the saturation scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                               _("_Saturation:"), SLIDER_WIDTH, -1,
                               0.0, 0.0, 100.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  col_tool->saturation_data = GTK_ADJUSTMENT (data);
  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (col_tool->saturation_data, "value-changed",
                    G_CALLBACK (colorize_saturation_changed),
                    col_tool);

  /*  Create the lightness scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                               _("_Lightness:"), SLIDER_WIDTH, -1,
                               0.0, -100.0, 100.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  col_tool->lightness_data = GTK_ADJUSTMENT (data);
  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (colorize_lightness_changed),
                    col_tool);
}

static void
gimp_colorize_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpColorizeTool *col_tool = GIMP_COLORIZE_TOOL (image_map_tool);

  colorize_init (col_tool->colorize);

  colorize_update_sliders (col_tool);
}

static void
colorize_update_sliders (GimpColorizeTool *col_tool)
{
  gtk_adjustment_set_value (GTK_ADJUSTMENT (col_tool->hue_data),
                            col_tool->colorize->hue);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (col_tool->saturation_data),
                            col_tool->colorize->saturation);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (col_tool->lightness_data),
                            col_tool->colorize->lightness);
}

static void
colorize_hue_changed (GtkAdjustment    *adjustment,
                      GimpColorizeTool *col_tool)
{
  if (col_tool->colorize->hue != adjustment->value)
    {
      col_tool->colorize->hue = adjustment->value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (col_tool));
    }
}

static void
colorize_saturation_changed (GtkAdjustment    *adjustment,
                             GimpColorizeTool *col_tool)
{
  if (col_tool->colorize->saturation != adjustment->value)
    {
      col_tool->colorize->saturation = adjustment->value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (col_tool));
    }
}

static void
colorize_lightness_changed (GtkAdjustment    *adjustment,
                            GimpColorizeTool *col_tool)
{
  if (col_tool->colorize->lightness != adjustment->value)
    {
      col_tool->colorize->lightness = adjustment->value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (col_tool));
    }
}
