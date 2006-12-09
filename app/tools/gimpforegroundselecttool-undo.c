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

#include "tools-types.h"

#include "base/tile-manager.h"

#include "core/gimpimage-undo.h"
#include "core/gimpimage.h"
#include "core/gimpundo.h"

#include "gimptoolcontrol.h"
#include "gimpforegroundselecttool.h"
#include "gimpforegroundselecttool-undo.h"
#include "tool_manager.h"


typedef struct
{
  gint  tool_ID;
} FgSelectUndo;


static gboolean undo_pop_foreground_select  (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode,
                                             GimpUndoAccumulator *accum);
static void     undo_free_foreground_select (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode);


gboolean
gimp_foreground_select_tool_push_undo (GimpImage   *image,
                                       const gchar *undo_desc,
                                       gint         tool_ID)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (FgSelectUndo),
                                   sizeof (FgSelectUndo),
                                   GIMP_UNDO_FOREGROUND_SELECT, undo_desc,
                                   FALSE,
                                   undo_pop_foreground_select,
                                   undo_free_foreground_select,
                                   NULL)))
    {
      FgSelectUndo *undo = new->data;

      undo->tool_ID = tool_ID;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_foreground_select (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (undo->image->gimp);

  if (GIMP_IS_FOREGROUND_SELECT_TOOL (active_tool))
    {
      FgSelectUndo *fg_undo = undo->data;

      /*  only pop if the active tool is the tool that pushed this undo  */
      if (fg_undo->tool_ID == active_tool->ID)
        {

        }
    }

  return TRUE;
}

static void
undo_free_foreground_select (GimpUndo     *undo,
                             GimpUndoMode  undo_mode)
{
  FgSelectUndo *fg_undo = undo->data;


  g_free (fg_undo);
}
