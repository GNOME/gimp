/* The GIMP -- an image manipulation program
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

#include "gimptransformtool.h"
#include "gimptransformtool-undo.h"
#include "tool_manager.h"

#include "path_transform.h"

#include "libgimp/gimpintl.h"


/********************/
/*  Transform Undo  */
/********************/

typedef struct _TransformUndo TransformUndo;

struct _TransformUndo
{
  gint         tool_ID;
  GType        tool_type;

  TransInfo    trans_info;
  TileManager *original;
  gpointer     path_undo;
};

static gboolean undo_pop_transform  (GimpUndo            *undo,
                                     GimpUndoMode         undo_mode,
                                     GimpUndoAccumulator *accum);
static void     undo_free_transform (GimpUndo            *undo,
                                     GimpUndoMode         undo_mode);

gboolean
gimp_transform_tool_push_undo (GimpImage   *gimage,
                               const gchar *undo_desc,
                               gint         tool_ID,
                               GType        tool_type,
                               gdouble     *trans_info,
                               TileManager *original,
                               GSList      *path_undo)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (TransformUndo),
                                   sizeof (TransformUndo),
                                   GIMP_UNDO_TRANSFORM, undo_desc,
                                   FALSE,
                                   undo_pop_transform,
                                   undo_free_transform)))
    {
      TransformUndo *tu;
      gint           i;

      tu = new->data;

      tu->tool_ID   = tool_ID;
      tu->tool_type = tool_type;

      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tu->trans_info[i] = trans_info[i];

      tu->original  = original;
      tu->path_undo = path_undo;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_transform (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (undo->gimage->gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    {
      GimpTransformTool *tt;
      TransformUndo     *tu;

      tt = GIMP_TRANSFORM_TOOL (active_tool);
      tu = (TransformUndo *) undo->data;

      path_transform_do_undo (undo->gimage, tu->path_undo);

      /*  only pop if the active tool is the tool that pushed this undo  */
      if (tu->tool_ID == active_tool->ID)
        {
          TileManager *temp;
          gdouble      d;
          gint         i;

          /*  swap the transformation information arrays  */
          for (i = 0; i < TRAN_INFO_SIZE; i++)
            {
              d                 = tu->trans_info[i];
              tu->trans_info[i] = tt->trans_info[i];
              tt->trans_info[i] = d;
            }

          /*  swap the original buffer--the source buffer for repeated transforms
           */
          temp         = tu->original;
          tu->original = tt->original;
          tt->original = temp;

          /*  If we're re-implementing the first transform, reactivate tool  */
          if (undo_mode == GIMP_UNDO_MODE_REDO && tt->original)
            {
              gimp_tool_control_activate (active_tool->control);

              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tt));
            }
        }
    }

  return TRUE;
}

static void
undo_free_transform (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  TransformUndo * tu;

  tu = (TransformUndo *) undo->data;

  if (tu->original)
    tile_manager_destroy (tu->original);

  path_transform_free_undo (tu->path_undo);

  g_free (tu);
}
