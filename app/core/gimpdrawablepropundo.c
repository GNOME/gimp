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

#include "core-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "ligma-memsize.h"
#include "ligmaimage.h"
#include "ligmadrawable.h"
#include "ligmadrawablepropundo.h"


enum
{
  PROP_0
};


static void   ligma_drawable_prop_undo_constructed  (GObject             *object);
static void   ligma_drawable_prop_undo_set_property (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void   ligma_drawable_prop_undo_get_property (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static void   ligma_drawable_prop_undo_pop          (LigmaUndo            *undo,
                                                    LigmaUndoMode         undo_mode,
                                                    LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaDrawablePropUndo, ligma_drawable_prop_undo,
               LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_drawable_prop_undo_parent_class


static void
ligma_drawable_prop_undo_class_init (LigmaDrawablePropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed  = ligma_drawable_prop_undo_constructed;
  object_class->set_property = ligma_drawable_prop_undo_set_property;
  object_class->get_property = ligma_drawable_prop_undo_get_property;

  undo_class->pop            = ligma_drawable_prop_undo_pop;
}

static void
ligma_drawable_prop_undo_init (LigmaDrawablePropUndo *undo)
{
}

static void
ligma_drawable_prop_undo_constructed (GObject *object)
{
  LigmaDrawablePropUndo *drawable_prop_undo = LIGMA_DRAWABLE_PROP_UNDO (object);
  LigmaDrawable         *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_DRAWABLE (LIGMA_ITEM_UNDO (object)->item));

  drawable = LIGMA_DRAWABLE (LIGMA_ITEM_UNDO (object)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_DRAWABLE_FORMAT:
      drawable_prop_undo->format = ligma_drawable_get_format (drawable);
      break;

    default:
      g_return_if_reached ();
   }
}

static void
ligma_drawable_prop_undo_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_prop_undo_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_prop_undo_pop (LigmaUndo            *undo,
                             LigmaUndoMode         undo_mode,
                             LigmaUndoAccumulator *accum)
{
  LigmaDrawablePropUndo *drawable_prop_undo = LIGMA_DRAWABLE_PROP_UNDO (undo);
  LigmaDrawable         *drawable;

  drawable = LIGMA_DRAWABLE (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_DRAWABLE_FORMAT:
      {
        const Babl *format = ligma_drawable_get_format (drawable);

        ligma_drawable_set_format (drawable,
                                  drawable_prop_undo->format,
                                  TRUE, FALSE);

        drawable_prop_undo->format = format;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
