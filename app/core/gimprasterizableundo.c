/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprasterizableundo.c
 * Copyright (C) 2025 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gimprasterizable.h"
#include "gimprasterizableundo.h"


struct _GimpRasterizableUndo
{
  GimpItemUndo      parent_instance;

  gboolean          rasterized;
};


static void     gimp_rasterizable_undo_constructed  (GObject             *object);

static void     gimp_rasterizable_undo_pop          (GimpUndo            *undo,
                                                     GimpUndoMode         undo_mode,
                                                     GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpRasterizableUndo, gimp_rasterizable_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_rasterizable_undo_parent_class


static void
gimp_rasterizable_undo_class_init (GimpRasterizableUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_rasterizable_undo_constructed;
  undo_class->pop           = gimp_rasterizable_undo_pop;
}

static void
gimp_rasterizable_undo_init (GimpRasterizableUndo *undo)
{
}

static void
gimp_rasterizable_undo_constructed (GObject *object)
{
  GimpRasterizableUndo  *undo = GIMP_RASTERIZABLE_UNDO (object);
  GimpRasterizable      *rasterizable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_RASTERIZABLE (GIMP_ITEM_UNDO (undo)->item));

  rasterizable = GIMP_RASTERIZABLE (GIMP_ITEM_UNDO (undo)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_RASTERIZABLE:
      undo->rasterized = gimp_rasterizable_is_rasterized (rasterizable);
      break;

    default:
      gimp_assert_not_reached ();
    }
}

static void
gimp_rasterizable_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpRasterizableUndo *rundo        = GIMP_RASTERIZABLE_UNDO (undo);
  GimpRasterizable     *rasterizable = GIMP_RASTERIZABLE (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_RASTERIZABLE:
      {
        gboolean rasterized = gimp_rasterizable_is_rasterized (rasterizable);

        gimp_rasterizable_set_undo_rasterized (rasterizable, rundo->rasterized);
        rundo->rasterized = rasterized;

        GIMP_RASTERIZABLE_GET_IFACE (rasterizable)->set_rasterized (rasterizable, ! rasterized);

        gimp_viewable_invalidate_preview (GIMP_VIEWABLE (rasterizable));
      }
      break;

    default:
      gimp_assert_not_reached ();
    }
}
