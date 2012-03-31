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

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpoperationtool.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "filters-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
filters_filter_cmd_callback (GtkAction   *action,
                             const gchar *operation,
                             gpointer     data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpDisplay  *display;
  GimpTool     *active_tool;
  return_if_no_drawable (image, drawable, data);
  return_if_no_display (display, data);

  active_tool = tool_manager_get_active (image->gimp);

  if (G_TYPE_FROM_INSTANCE (active_tool) != GIMP_TYPE_OPERATION_TOOL)
    {
      GimpToolInfo *tool_info = gimp_get_tool_info (image->gimp,
                                                    "gimp-operation-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        gimp_context_set_tool (action_data_get_context (data), tool_info);
    }
  else
    {
      gimp_context_tool_changed (action_data_get_context (data));
    }

  active_tool = tool_manager_get_active (image->gimp);

  if (GIMP_IS_OPERATION_TOOL (active_tool))
    {
      gchar *label = gimp_strip_uline (gtk_action_get_label (action));

      gimp_operation_tool_set_operation (GIMP_OPERATION_TOOL (active_tool),
                                         operation, label);
      tool_manager_initialize_active (image->gimp, display);

      g_free (label);
    }
}
