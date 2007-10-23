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

#include "base/tile-manager.h"

#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimpdrawablemodundo.h"


static GObject * gimp_drawable_mod_undo_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);

static gint64    gimp_drawable_mod_undo_get_memsize (GimpObject            *object,
                                                     gint64                *gui_size);

static void      gimp_drawable_mod_undo_pop         (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode,
                                                     GimpUndoAccumulator   *accum);
static void      gimp_drawable_mod_undo_free        (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpDrawableModUndo, gimp_drawable_mod_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_drawable_mod_undo_parent_class


static void
gimp_drawable_mod_undo_class_init (GimpDrawableModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructor      = gimp_drawable_mod_undo_constructor;

  gimp_object_class->get_memsize = gimp_drawable_mod_undo_get_memsize;

  undo_class->pop                = gimp_drawable_mod_undo_pop;
  undo_class->free               = gimp_drawable_mod_undo_free;
}

static void
gimp_drawable_mod_undo_init (GimpDrawableModUndo *undo)
{
}

static GObject *
gimp_drawable_mod_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpDrawableModUndo *drawable_mod_undo;
  GimpDrawable        *drawable;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);

  g_assert (GIMP_IS_DRAWABLE (GIMP_ITEM_UNDO (object)->item));

  drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (object)->item);

  drawable_mod_undo->tiles    = tile_manager_ref (drawable->tiles);
  drawable_mod_undo->type     = drawable->type;
  drawable_mod_undo->offset_x = GIMP_ITEM (drawable)->offset_x;
  drawable_mod_undo->offset_y = GIMP_ITEM (drawable)->offset_y;

  return object;
}

static gint64
gimp_drawable_mod_undo_get_memsize (GimpObject *object,
                                    gint64     *gui_size)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);
  gint64               memsize           = 0;

  if (drawable_mod_undo->tiles)
    memsize += tile_manager_get_memsize (drawable_mod_undo->tiles, FALSE);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_drawable_mod_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (undo);
  GimpDrawable        *drawable          = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);
  TileManager         *tiles;
  GimpImageType        type;
  gint                 offset_x, offset_y;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  tiles    = drawable_mod_undo->tiles;
  type     = drawable_mod_undo->type;
  offset_x = drawable_mod_undo->offset_x;
  offset_y = drawable_mod_undo->offset_y;

  drawable_mod_undo->tiles    = tile_manager_ref (drawable->tiles);
  drawable_mod_undo->type     = drawable->type;
  drawable_mod_undo->offset_x = GIMP_ITEM (drawable)->offset_x;
  drawable_mod_undo->offset_y = GIMP_ITEM (drawable)->offset_y;

  gimp_drawable_set_tiles_full (drawable, FALSE, NULL,
                                tiles, type, offset_x, offset_y);
  tile_manager_unref (tiles);
}

static void
gimp_drawable_mod_undo_free (GimpUndo     *undo,
                             GimpUndoMode  undo_mode)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (undo);

  if (drawable_mod_undo->tiles)
    {
      tile_manager_unref (drawable_mod_undo->tiles);
      drawable_mod_undo->tiles = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
