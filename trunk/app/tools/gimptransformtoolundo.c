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

#include "gimptoolcontrol.h"
#include "gimptransformtool.h"
#include "gimptransformtoolundo.h"


enum
{
  PROP_0,
  PROP_TRANSFORM_TOOL
};


static GObject * gimp_transform_tool_undo_constructor  (GType                  type,
                                                        guint                  n_params,
                                                        GObjectConstructParam *params);
static void      gimp_transform_tool_undo_set_property (GObject               *object,
                                                        guint                  property_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void      gimp_transform_tool_undo_get_property (GObject               *object,
                                                        guint                  property_id,
                                                        GValue                *value,
                                                        GParamSpec            *pspec);

static void      gimp_transform_tool_undo_pop          (GimpUndo              *undo,
                                                        GimpUndoMode           undo_mode,
                                                        GimpUndoAccumulator   *accum);
static void      gimp_transform_tool_undo_free         (GimpUndo              *undo,
                                                        GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpTransformToolUndo, gimp_transform_tool_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_transform_tool_undo_parent_class


static void
gimp_transform_tool_undo_class_init (GimpTransformToolUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor  = gimp_transform_tool_undo_constructor;
  object_class->set_property = gimp_transform_tool_undo_set_property;
  object_class->get_property = gimp_transform_tool_undo_get_property;

  undo_class->pop            = gimp_transform_tool_undo_pop;
  undo_class->free           = gimp_transform_tool_undo_free;

  g_object_class_install_property (object_class, PROP_TRANSFORM_TOOL,
                                   g_param_spec_object ("transform-tool",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TRANSFORM_TOOL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_transform_tool_undo_init (GimpTransformToolUndo *undo)
{
}

static GObject *
gimp_transform_tool_undo_constructor (GType                  type,
                                      guint                  n_params,
                                      GObjectConstructParam *params)
{
  GObject               *object;
  GimpTransformToolUndo *transform_tool_undo;
  GimpTransformTool     *transform_tool;
  gint                   i;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  transform_tool_undo = GIMP_TRANSFORM_TOOL_UNDO (object);

  g_assert (GIMP_IS_TRANSFORM_TOOL (transform_tool_undo->transform_tool));

  transform_tool = GIMP_TRANSFORM_TOOL (transform_tool_undo->transform_tool);

  for (i = 0; i < TRANS_INFO_SIZE; i++)
    transform_tool_undo->trans_info[i] = transform_tool->old_trans_info[i];

#if 0
  if (transform_tool->original)
    transform_tool_undo->original = tile_manager_ref (transform_tool->original);
#endif

  g_object_add_weak_pointer (G_OBJECT (transform_tool_undo->transform_tool),
                             (gpointer) &transform_tool_undo->transform_tool);

  return object;
}

static void
gimp_transform_tool_undo_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpTransformToolUndo *transform_tool_undo = GIMP_TRANSFORM_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_TRANSFORM_TOOL:
      transform_tool_undo->transform_tool = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_tool_undo_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpTransformToolUndo *transform_tool_undo = GIMP_TRANSFORM_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_TRANSFORM_TOOL:
      g_value_set_object (value, transform_tool_undo->transform_tool);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_tool_undo_pop (GimpUndo              *undo,
                              GimpUndoMode           undo_mode,
                              GimpUndoAccumulator   *accum)
{
  GimpTransformToolUndo *transform_tool_undo = GIMP_TRANSFORM_TOOL_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (transform_tool_undo->transform_tool)
    {
      GimpTransformTool *transform_tool;
      TileManager       *temp;
      gdouble            d;
      gint               i;

      transform_tool = transform_tool_undo->transform_tool;

      /*  swap the transformation information arrays  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        {
          d = transform_tool_undo->trans_info[i];
          transform_tool_undo->trans_info[i] = transform_tool->trans_info[i];
          transform_tool->trans_info[i] = d;
        }

      /*  swap the original buffer--the source buffer for repeated transforms
       */
      temp                          = transform_tool_undo->original;
      transform_tool_undo->original = transform_tool->original;
      transform_tool->original      = temp;

      /*  If we're re-implementing the first transform, reactivate tool  */
      if (undo_mode == GIMP_UNDO_MODE_REDO && transform_tool->original)
        {
          gimp_tool_control_activate (GIMP_TOOL (transform_tool)->control);

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (transform_tool));
        }
    }
 }

static void
gimp_transform_tool_undo_free (GimpUndo     *undo,
                               GimpUndoMode  undo_mode)
{
  GimpTransformToolUndo *transform_tool_undo = GIMP_TRANSFORM_TOOL_UNDO (undo);

  if (transform_tool_undo->transform_tool)
    {
      g_object_remove_weak_pointer (G_OBJECT (transform_tool_undo->transform_tool),
                                    (gpointer) &transform_tool_undo->transform_tool);
      transform_tool_undo->transform_tool = NULL;
    }

  if (transform_tool_undo->original)
    {
      tile_manager_unref (transform_tool_undo->original);
      transform_tool_undo->original = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
