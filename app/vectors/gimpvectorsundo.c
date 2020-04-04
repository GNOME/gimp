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

#include "vectors-types.h"

#include "libgimpbase/gimpbase.h"

#include "core/gimpimage.h"

#include "gimpvectors.h"
#include "gimpvectorsundo.h"


enum
{
  PROP_0,
  PROP_PREV_PARENT,
  PROP_PREV_POSITION,
  PROP_PREV_VECTORS
};


static void     gimp_vectors_undo_constructed  (GObject             *object);
static void     gimp_vectors_undo_finalize     (GObject             *object);
static void     gimp_vectors_undo_set_property (GObject             *object,
                                                guint                property_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);
static void     gimp_vectors_undo_get_property (GObject             *object,
                                                guint                property_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);

static gint64   gimp_vectors_undo_get_memsize  (GimpObject          *object,
                                                gint64              *gui_size);

static void     gimp_vectors_undo_pop          (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode,
                                                GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpVectorsUndo, gimp_vectors_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_vectors_undo_parent_class


static void
gimp_vectors_undo_class_init (GimpVectorsUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_vectors_undo_constructed;
  object_class->finalize         = gimp_vectors_undo_finalize;
  object_class->set_property     = gimp_vectors_undo_set_property;
  object_class->get_property     = gimp_vectors_undo_get_property;

  gimp_object_class->get_memsize = gimp_vectors_undo_get_memsize;

  undo_class->pop                = gimp_vectors_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_PARENT,
                                   g_param_spec_object ("prev-parent",
                                                        NULL, NULL,
                                                        GIMP_TYPE_VECTORS,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_POSITION,
                                   g_param_spec_int ("prev-position", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_VECTORS,
                                   g_param_spec_pointer ("prev-vectors", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_vectors_undo_init (GimpVectorsUndo *undo)
{
  undo->prev_vectors = NULL;
}

static void
gimp_vectors_undo_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_VECTORS (GIMP_ITEM_UNDO (object)->item));
}

static void
gimp_vectors_undo_finalize (GObject *object)
{
  GimpVectorsUndo *undo = GIMP_VECTORS_UNDO (object);

  g_clear_pointer (&undo->prev_vectors, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vectors_undo_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpVectorsUndo *vectors_undo = GIMP_VECTORS_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      vectors_undo->prev_parent = g_value_get_object (value);
      break;
    case PROP_PREV_POSITION:
      vectors_undo->prev_position = g_value_get_int (value);
      break;
    case PROP_PREV_VECTORS:
      vectors_undo->prev_vectors = g_list_copy (g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vectors_undo_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpVectorsUndo *vectors_undo = GIMP_VECTORS_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      g_value_set_object (value, vectors_undo->prev_parent);
      break;
    case PROP_PREV_POSITION:
      g_value_set_int (value, vectors_undo->prev_position);
      break;
    case PROP_PREV_VECTORS:
      g_value_set_pointer (value, vectors_undo->prev_vectors);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_vectors_undo_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpItemUndo *item_undo = GIMP_ITEM_UNDO (object);
  gint64        memsize   = 0;

  if (! gimp_item_is_attached (item_undo->item))
    memsize += gimp_object_get_memsize (GIMP_OBJECT (item_undo->item),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_vectors_undo_pop (GimpUndo            *undo,
                       GimpUndoMode         undo_mode,
                       GimpUndoAccumulator *accum)
{
  GimpVectorsUndo *vectors_undo = GIMP_VECTORS_UNDO (undo);
  GimpVectors     *vectors      = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_REMOVE))
    {
      /*  remove vectors  */

      /*  record the current parent and position  */
      vectors_undo->prev_parent   = gimp_vectors_get_parent (vectors);
      vectors_undo->prev_position = gimp_item_get_index (GIMP_ITEM (vectors));

      gimp_image_remove_vectors (undo->image, vectors, FALSE,
                                 vectors_undo->prev_vectors);
    }
  else
    {
      /*  restore vectors  */

      /*  record the active vectors  */
      g_clear_pointer (&vectors_undo->prev_vectors, g_list_free);
      vectors_undo->prev_vectors = g_list_copy (gimp_image_get_selected_vectors (undo->image));

      gimp_image_add_vectors (undo->image, vectors,
                              vectors_undo->prev_parent,
                              vectors_undo->prev_position, FALSE);
    }
}
