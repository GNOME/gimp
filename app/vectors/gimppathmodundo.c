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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "path-types.h"

#include "gimppath.h"
#include "gimppathmodundo.h"


static void     gimp_path_mod_undo_constructed (GObject             *object);

static gint64   gimp_path_mod_undo_get_memsize (GimpObject          *object,
                                                gint64              *gui_size);

static void     gimp_path_mod_undo_pop         (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode,
                                                GimpUndoAccumulator *accum);
static void     gimp_path_mod_undo_free        (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpPathModUndo, gimp_path_mod_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_path_mod_undo_parent_class


static void
gimp_path_mod_undo_class_init (GimpPathModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_path_mod_undo_constructed;

  gimp_object_class->get_memsize = gimp_path_mod_undo_get_memsize;

  undo_class->pop                = gimp_path_mod_undo_pop;
  undo_class->free               = gimp_path_mod_undo_free;
}

static void
gimp_path_mod_undo_init (GimpPathModUndo *undo)
{
}

static void
gimp_path_mod_undo_constructed (GObject *object)
{
  GimpPathModUndo *path_mod_undo = GIMP_PATH_MOD_UNDO (object);
  GimpPath        *path;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_PATH (GIMP_ITEM_UNDO (object)->item));

  path = GIMP_PATH (GIMP_ITEM_UNDO (object)->item);

  path_mod_undo->path =
    GIMP_PATH (gimp_item_duplicate (GIMP_ITEM (path),
                                    G_TYPE_FROM_INSTANCE (path)));
}

static gint64
gimp_path_mod_undo_get_memsize (GimpObject *object,
                                gint64     *gui_size)
{
  GimpPathModUndo *path_mod_undo = GIMP_PATH_MOD_UNDO (object);
  gint64           memsize          = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (path_mod_undo->path),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_path_mod_undo_pop (GimpUndo            *undo,
                        GimpUndoMode         undo_mode,
                        GimpUndoAccumulator *accum)
{
  GimpPathModUndo *path_mod_undo = GIMP_PATH_MOD_UNDO (undo);
  GimpPath        *path          = GIMP_PATH (GIMP_ITEM_UNDO (undo)->item);
  GimpPath        *temp;
  gint             offset_x;
  gint             offset_y;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  temp = path_mod_undo->path;

  path_mod_undo->path =
    GIMP_PATH (gimp_item_duplicate (GIMP_ITEM (path),
                                    G_TYPE_FROM_INSTANCE (path)));

  gimp_path_freeze (path);

  gimp_path_copy_strokes (temp, path);

  gimp_item_get_offset (GIMP_ITEM (temp), &offset_x, &offset_y);
  gimp_item_set_offset (GIMP_ITEM (path), offset_x, offset_y);

  gimp_item_set_size (GIMP_ITEM (path),
                      gimp_item_get_width  (GIMP_ITEM (temp)),
                      gimp_item_get_height (GIMP_ITEM (temp)));

  g_object_unref (temp);

  gimp_path_thaw (path);
}

static void
gimp_path_mod_undo_free (GimpUndo     *undo,
                         GimpUndoMode  undo_mode)
{
  GimpPathModUndo *path_mod_undo = GIMP_PATH_MOD_UNDO (undo);

  g_clear_object (&path_mod_undo->path);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
