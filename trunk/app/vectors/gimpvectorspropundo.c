/* Gimp - The GNU Image Manipulation Program
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

#include "vectors-types.h"

#include "core/gimpimage.h"

#include "gimpvectors.h"
#include "gimpvectorspropundo.h"


static GObject * gimp_vectors_prop_undo_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);

static void      gimp_vectors_prop_undo_pop         (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode,
                                                     GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpVectorsPropUndo, gimp_vectors_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_vectors_prop_undo_parent_class


static void
gimp_vectors_prop_undo_class_init (GimpVectorsPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_vectors_prop_undo_constructor;

  undo_class->pop           = gimp_vectors_prop_undo_pop;
}

static void
gimp_vectors_prop_undo_init (GimpVectorsPropUndo *undo)
{
}

static GObject *
gimp_vectors_prop_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpVectorsPropUndo *vectors_prop_undo;
  GimpImage           *image;
  GimpVectors         *vectors;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  vectors_prop_undo = GIMP_VECTORS_PROP_UNDO (object);

  g_assert (GIMP_IS_VECTORS (GIMP_ITEM_UNDO (object)->item));

  image   = GIMP_UNDO (object)->image;
  vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_VECTORS_REPOSITION:
      vectors_prop_undo->position = gimp_image_get_vectors_index (image, vectors);
      break;

    default:
      g_assert_not_reached ();
    }

  return object;
}

static void
gimp_vectors_prop_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpVectorsPropUndo *vectors_prop_undo = GIMP_VECTORS_PROP_UNDO (undo);
  GimpVectors         *vectors           = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_VECTORS_REPOSITION:
      {
        gint position;

        position = gimp_image_get_vectors_index (undo->image, vectors);
        gimp_image_position_vectors (undo->image, vectors,
                                     vectors_prop_undo->position,
                                     FALSE, NULL);
        vectors_prop_undo->position = position;
      }
      break;

    default:
      g_assert_not_reached ();
    }
}
