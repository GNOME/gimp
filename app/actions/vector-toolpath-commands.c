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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolpath.h"

#include "dialogs/dialogs.h"

#include "vector-toolpath-commands.h"


/*  public functions  */

void
vector_toolpath_delete_anchor_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);

  gimp_tool_path_delete_anchor (tool_path);
}

void
vector_toolpath_shift_start_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);

  gimp_tool_path_shift_start (tool_path);
}

void
vector_toolpath_insert_anchor_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);

  gimp_tool_path_insert_anchor (tool_path);
}

void
vector_toolpath_delete_segment_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);

  gimp_tool_path_delete_segment (tool_path);
}

void
vector_toolpath_reverse_stroke_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);

  gimp_tool_path_reverse_stroke (tool_path);
}

