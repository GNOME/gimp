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

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp-memsize.h"
#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimpdrawablepropundo.h"


enum
{
  PROP_0
};


static void   gimp_drawable_prop_undo_constructed  (GObject             *object);
static void   gimp_drawable_prop_undo_set_property (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void   gimp_drawable_prop_undo_get_property (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static void   gimp_drawable_prop_undo_pop          (GimpUndo            *undo,
                                                    GimpUndoMode         undo_mode,
                                                    GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpDrawablePropUndo, gimp_drawable_prop_undo,
               GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_drawable_prop_undo_parent_class


static void
gimp_drawable_prop_undo_class_init (GimpDrawablePropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_drawable_prop_undo_constructed;
  object_class->set_property = gimp_drawable_prop_undo_set_property;
  object_class->get_property = gimp_drawable_prop_undo_get_property;

  undo_class->pop            = gimp_drawable_prop_undo_pop;
}

static void
gimp_drawable_prop_undo_init (GimpDrawablePropUndo *undo)
{
}

static void
gimp_drawable_prop_undo_constructed (GObject *object)
{
  GimpDrawablePropUndo *drawable_prop_undo = GIMP_DRAWABLE_PROP_UNDO (object);
  GimpDrawable         *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_DRAWABLE (GIMP_ITEM_UNDO (object)->item));

  drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_DRAWABLE_FORMAT:
      drawable_prop_undo->format = gimp_drawable_get_format (drawable);
      break;

    default:
      g_return_if_reached ();
   }
}

static void
gimp_drawable_prop_undo_set_property (GObject      *object,
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
gimp_drawable_prop_undo_get_property (GObject    *object,
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
gimp_drawable_prop_undo_pop (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  GimpDrawablePropUndo *drawable_prop_undo = GIMP_DRAWABLE_PROP_UNDO (undo);
  GimpDrawable         *drawable;

  drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_DRAWABLE_FORMAT:
      {
        const Babl *format = gimp_drawable_get_format (drawable);

        gimp_drawable_set_format (drawable,
                                  drawable_prop_undo->format,
                                  TRUE, FALSE);

        drawable_prop_undo->format = format;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
