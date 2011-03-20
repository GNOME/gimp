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

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/desaturate.h"

#include "gegl/gimpdesaturateconfig.h"

#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpimagemapoptions.h"
#include "gimpdesaturatetool.h"

#include "gimp-intl.h"


static gboolean   gimp_desaturate_tool_initialize    (GimpTool           *tool,
                                                      GimpDisplay        *display,
                                                      GError            **error);

static GeglNode * gimp_desaturate_tool_get_operation (GimpImageMapTool   *im_tool,
                                                      GObject           **config);
static void       gimp_desaturate_tool_map           (GimpImageMapTool   *im_tool);
static void       gimp_desaturate_tool_dialog        (GimpImageMapTool   *im_tool);

static void       gimp_desaturate_tool_config_notify (GObject            *object,
                                                      GParamSpec         *pspec,
                                                      GimpDesaturateTool *desaturate_tool);
static void       gimp_desaturate_tool_mode_changed  (GtkWidget          *button,
                                                      GimpDesaturateTool *desaturate_tool);


G_DEFINE_TYPE (GimpDesaturateTool, gimp_desaturate_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_desaturate_tool_parent_class


void
gimp_desaturate_tool_register (GimpToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (GIMP_TYPE_DESATURATE_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-desaturate-tool",
                _("Desaturate"),
                _("Desaturate Tool: Turn colors into shades of gray"),
                N_("_Desaturate..."), NULL,
                NULL, GIMP_HELP_TOOL_DESATURATE,
                GIMP_STOCK_TOOL_DESATURATE,
                data);
}

static void
gimp_desaturate_tool_class_init (GimpDesaturateToolClass *klass)
{
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  tool_class->initialize       = gimp_desaturate_tool_initialize;

  im_tool_class->dialog_desc   = _("Desaturate (Remove Colors)");

  im_tool_class->get_operation = gimp_desaturate_tool_get_operation;
  im_tool_class->map           = gimp_desaturate_tool_map;
  im_tool_class->dialog        = gimp_desaturate_tool_dialog;
}

static void
gimp_desaturate_tool_init (GimpDesaturateTool *desaturate_tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (desaturate_tool);

  im_tool->apply_func = (GimpImageMapApplyFunc) desaturate_region;
  im_tool->apply_data = &desaturate_tool->mode;
}

static gboolean
gimp_desaturate_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpDesaturateTool *desaturate_tool = GIMP_DESATURATE_TOOL (tool);
  GimpImage          *image           = gimp_display_get_image (display);
  GimpDrawable       *drawable        = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Desaturate only operates on RGB layers."));
      return FALSE;
    }

  gimp_config_reset (GIMP_CONFIG (desaturate_tool->config));

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (desaturate_tool->button),
                                   desaturate_tool->config->mode);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (desaturate_tool));

  return TRUE;
}

static GeglNode *
gimp_desaturate_tool_get_operation (GimpImageMapTool  *image_map_tool,
                                    GObject          **config)
{
  GimpDesaturateTool *desaturate_tool = GIMP_DESATURATE_TOOL (image_map_tool);
  GeglNode           *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:desaturate",
                       NULL);

  desaturate_tool->config = g_object_new (GIMP_TYPE_DESATURATE_CONFIG, NULL);

  *config = G_OBJECT (desaturate_tool->config);

  g_signal_connect_object (desaturate_tool->config, "notify",
                           G_CALLBACK (gimp_desaturate_tool_config_notify),
                           G_OBJECT (desaturate_tool), 0);

  gegl_node_set (node,
                 "config", desaturate_tool->config,
                 NULL);

  return node;
}

static void
gimp_desaturate_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpDesaturateTool *desaturate_tool = GIMP_DESATURATE_TOOL (image_map_tool);

  desaturate_tool->mode = desaturate_tool->config->mode;
}


/**********************/
/*  Desaturate dialog  */
/**********************/

static void
gimp_desaturate_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpDesaturateTool *desaturate_tool = GIMP_DESATURATE_TOOL (image_map_tool);
  GtkWidget          *main_vbox;
  GtkWidget          *frame;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  /*  The table containing sliders  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_DESATURATE_MODE,
                                     gtk_label_new (_("Choose shade of gray based on:")),
                                     G_CALLBACK (gimp_desaturate_tool_mode_changed),
                                     desaturate_tool,
                                     &desaturate_tool->button);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
}

static void
gimp_desaturate_tool_config_notify (GObject            *object,
                                    GParamSpec         *pspec,
                                    GimpDesaturateTool *desaturate_tool)
{
  GimpDesaturateConfig *config = GIMP_DESATURATE_CONFIG (object);

  if (! desaturate_tool->button)
    return;

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (desaturate_tool->button),
                                   config->mode);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (desaturate_tool));
}

static void
gimp_desaturate_tool_mode_changed (GtkWidget          *button,
                                   GimpDesaturateTool *desaturate_tool)
{
  GimpDesaturateConfig *config = desaturate_tool->config;
  GimpDesaturateMode    mode;

  mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                             "gimp-item-data"));

  if (config->mode != mode)
    {
      g_object_set (config,
                    "mode", mode,
                    NULL);
    }
}
