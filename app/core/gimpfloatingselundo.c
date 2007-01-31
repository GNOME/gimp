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

#include <glib-object.h>

#include "core-types.h"

#include "gimpfloatingselundo.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"


static GObject * gimp_floating_sel_undo_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);

static void      gimp_floating_sel_undo_pop         (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode,
                                                     GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpFloatingSelUndo, gimp_floating_sel_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_floating_sel_undo_parent_class


static void
gimp_floating_sel_undo_class_init (GimpFloatingSelUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_floating_sel_undo_constructor;

  undo_class->pop           = gimp_floating_sel_undo_pop;
}

static void
gimp_floating_sel_undo_init (GimpFloatingSelUndo *undo)
{
}

static GObject *
gimp_floating_sel_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  g_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  return object;
}

static void
gimp_floating_sel_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpFloatingSelUndo *floating_sel_undo = GIMP_FLOATING_SEL_UNDO (undo);
  GimpLayer           *floating_layer    = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if (! gimp_layer_is_floating_sel (floating_layer))
    return;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_FS_RIGOR) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_FS_RELAX))
    {
      floating_sel_relax (floating_layer, FALSE);
    }
  else
    {
      floating_sel_rigor (floating_layer, FALSE);
    }
}
