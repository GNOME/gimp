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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimpposterizeconfig.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "gimpimagemapoptions.h"
#include "gimpposterizetool.h"

#include "gimp-intl.h"


static GeglNode * gimp_posterize_tool_get_operation (GimpImageMapTool  *im_tool,
                                                     GObject          **config,
                                                     gchar            **undo_desc);
static void       gimp_posterize_tool_dialog        (GimpImageMapTool  *im_tool);


G_DEFINE_TYPE (GimpPosterizeTool, gimp_posterize_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_posterize_tool_parent_class


void
gimp_posterize_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_POSTERIZE_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-posterize-tool",
                _("Posterize"),
                _("Posterize Tool: Reduce to a limited set of colors"),
                N_("_Posterize..."), NULL,
                NULL, GIMP_HELP_TOOL_POSTERIZE,
                GIMP_STOCK_TOOL_POSTERIZE,
                data);
}

static void
gimp_posterize_tool_class_init (GimpPosterizeToolClass *klass)
{
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  im_tool_class->dialog_desc   = _("Posterize (Reduce Number of Colors)");

  im_tool_class->get_operation = gimp_posterize_tool_get_operation;
  im_tool_class->dialog        = gimp_posterize_tool_dialog;
}

static void
gimp_posterize_tool_init (GimpPosterizeTool *posterize_tool)
{
}

static GeglNode *
gimp_posterize_tool_get_operation (GimpImageMapTool  *image_map_tool,
                                   GObject          **config,
                                   gchar            **undo_desc)
{
  *config = g_object_new (GIMP_TYPE_POSTERIZE_CONFIG, NULL);

  return gegl_node_new_child (NULL,
                              "operation", "gimp:posterize",
                              "config",    *config,
                              NULL);
}


/**********************/
/*  Posterize dialog  */
/**********************/

static void
gimp_posterize_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GtkWidget *main_vbox;
  GtkWidget *scale;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  scale = gimp_prop_spin_scale_new (image_map_tool->config, "levels",
                                    _("Posterize _levels"),
                                    1.0, 10.0, 0);
  gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), 1.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);
}
