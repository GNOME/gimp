/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmaimage.h"
#include "ligmadrawable.h"
#include "ligmadrawableundo.h"


enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_X,
  PROP_Y
};


static void     ligma_drawable_undo_constructed  (GObject             *object);
static void     ligma_drawable_undo_set_property (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void     ligma_drawable_undo_get_property (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static gint64   ligma_drawable_undo_get_memsize  (LigmaObject          *object,
                                                 gint64              *gui_size);

static void     ligma_drawable_undo_pop          (LigmaUndo            *undo,
                                                 LigmaUndoMode         undo_mode,
                                                 LigmaUndoAccumulator *accum);
static void     ligma_drawable_undo_free         (LigmaUndo            *undo,
                                                 LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaDrawableUndo, ligma_drawable_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_drawable_undo_parent_class


static void
ligma_drawable_undo_class_init (LigmaDrawableUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_drawable_undo_constructed;
  object_class->set_property     = ligma_drawable_undo_set_property;
  object_class->get_property     = ligma_drawable_undo_get_property;

  ligma_object_class->get_memsize = ligma_drawable_undo_get_memsize;

  undo_class->pop                = ligma_drawable_undo_pop;
  undo_class->free               = ligma_drawable_undo_free;

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer", NULL, NULL,
                                                        GEGL_TYPE_BUFFER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x", NULL, NULL,
                                                     0, LIGMA_MAX_IMAGE_SIZE, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y", NULL, NULL,
                                                     0, LIGMA_MAX_IMAGE_SIZE, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_drawable_undo_init (LigmaDrawableUndo *undo)
{
}

static void
ligma_drawable_undo_constructed (GObject *object)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_DRAWABLE (LIGMA_ITEM_UNDO (object)->item));
  ligma_assert (GEGL_IS_BUFFER (drawable_undo->buffer));
}

static void
ligma_drawable_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      drawable_undo->buffer = g_value_dup_object (value);
      break;
    case PROP_X:
      drawable_undo->x = g_value_get_int (value);
      break;
    case PROP_Y:
      drawable_undo->y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, drawable_undo->buffer);
      break;
    case PROP_X:
      g_value_set_int (value, drawable_undo->x);
      break;
    case PROP_Y:
      g_value_set_int (value, drawable_undo->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_drawable_undo_get_memsize (LigmaObject *object,
                                gint64     *gui_size)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (object);
  gint64            memsize       = 0;

  memsize += ligma_gegl_buffer_get_memsize (drawable_undo->buffer);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_drawable_undo_pop (LigmaUndo            *undo,
                        LigmaUndoMode         undo_mode,
                        LigmaUndoAccumulator *accum)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (undo);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  ligma_drawable_swap_pixels (LIGMA_DRAWABLE (LIGMA_ITEM_UNDO (undo)->item),
                             drawable_undo->buffer,
                             drawable_undo->x,
                             drawable_undo->y);
}

static void
ligma_drawable_undo_free (LigmaUndo     *undo,
                         LigmaUndoMode  undo_mode)
{
  LigmaDrawableUndo *drawable_undo = LIGMA_DRAWABLE_UNDO (undo);

  g_clear_object (&drawable_undo->buffer);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
