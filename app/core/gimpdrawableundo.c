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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/tile-manager.h"

#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimpdrawableundo.h"


enum
{
  PROP_0,
  PROP_TILES,
  PROP_SPARSE,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};


static GObject * gimp_drawable_undo_constructor  (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);
static void      gimp_drawable_undo_set_property (GObject               *object,
                                                  guint                  property_id,
                                                  const GValue          *value,
                                                  GParamSpec            *pspec);
static void      gimp_drawable_undo_get_property (GObject               *object,
                                                  guint                  property_id,
                                                  GValue                *value,
                                                  GParamSpec            *pspec);

static void      gimp_drawable_undo_pop          (GimpUndo              *undo,
                                                  GimpUndoMode           undo_mode,
                                                  GimpUndoAccumulator   *accum);
static void      gimp_drawable_undo_free         (GimpUndo              *undo,
                                                  GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpDrawableUndo, gimp_drawable_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_drawable_undo_parent_class


static void
gimp_drawable_undo_class_init (GimpDrawableUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor  = gimp_drawable_undo_constructor;
  object_class->set_property = gimp_drawable_undo_set_property;
  object_class->get_property = gimp_drawable_undo_get_property;

  undo_class->pop            = gimp_drawable_undo_pop;
  undo_class->free           = gimp_drawable_undo_free;

  g_object_class_install_property (object_class, PROP_TILES,
                                   g_param_spec_pointer ("tiles", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SPARSE,
                                   g_param_spec_boolean ("sparse", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x", NULL, NULL,
                                                     0, GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y", NULL, NULL,
                                                     0, GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     0, GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     0, GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_drawable_undo_init (GimpDrawableUndo *undo)
{
}

static GObject *
gimp_drawable_undo_constructor (GType                  type,
                                guint                  n_params,
                                GObjectConstructParam *params)
{
  GObject          *object;
  GimpDrawableUndo *drawable_undo;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  drawable_undo = GIMP_DRAWABLE_UNDO (object);

  g_assert (GIMP_IS_DRAWABLE (GIMP_ITEM_UNDO (object)->item));

  return object;
}

static void
gimp_drawable_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDrawableUndo *drawable_undo = GIMP_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_TILES:
      drawable_undo->tiles = tile_manager_ref (g_value_get_pointer (value));
      break;
    case PROP_SPARSE:
      drawable_undo->sparse = g_value_get_boolean (value);
      break;
    case PROP_X:
      drawable_undo->x = g_value_get_int (value);
      break;
    case PROP_Y:
      drawable_undo->y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      drawable_undo->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      drawable_undo->height = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDrawableUndo *drawable_undo = GIMP_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_TILES:
      g_value_set_pointer (value, drawable_undo->tiles);
      break;
    case PROP_SPARSE:
      g_value_set_boolean (value, drawable_undo->sparse);
      break;
    case PROP_X:
      g_value_set_int (value, drawable_undo->x);
      break;
    case PROP_Y:
      g_value_set_int (value, drawable_undo->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, drawable_undo->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, drawable_undo->height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_undo_pop (GimpUndo            *undo,
                        GimpUndoMode         undo_mode,
                        GimpUndoAccumulator *accum)
{
  GimpDrawableUndo *drawable_undo = GIMP_DRAWABLE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  undo->size -= tile_manager_get_memsize (drawable_undo->tiles,
                                          drawable_undo->sparse);

  gimp_drawable_swap_pixels (GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item),
                             drawable_undo->tiles,
                             drawable_undo->sparse,
                             drawable_undo->x,
                             drawable_undo->y,
                             drawable_undo->width,
                             drawable_undo->height);

  undo->size += tile_manager_get_memsize (drawable_undo->tiles,
                                          drawable_undo->sparse);
}

static void
gimp_drawable_undo_free (GimpUndo     *undo,
                         GimpUndoMode  undo_mode)
{
  GimpDrawableUndo *drawable_undo = GIMP_DRAWABLE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);

  if (drawable_undo->tiles)
    {
      tile_manager_unref (drawable_undo->tiles);
      drawable_undo->tiles = NULL;
    }
}
