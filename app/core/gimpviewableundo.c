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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpviewable.h"
#include "gimpviewableundo.h"


enum
{
  PROP_0,
  PROP_VIEWABLE
};


static void   gimp_viewable_undo_constructed  (GObject      *object);
static void   gimp_viewable_undo_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void   gimp_viewable_undo_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

static void   gimp_viewable_undo_free         (GimpUndo     *undo,
                                           GimpUndoMode  undo_mode);


G_DEFINE_TYPE (GimpViewableUndo, gimp_viewable_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_viewable_undo_parent_class


static void
gimp_viewable_undo_class_init (GimpViewableUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_viewable_undo_constructed;
  object_class->set_property = gimp_viewable_undo_set_property;
  object_class->get_property = gimp_viewable_undo_get_property;

  undo_class->pop            = gimp_viewable_undo_pop;
  undo_class->free           = gimp_viewable_undo_free;

  g_object_class_install_property (object_class, PROP_VIEWABLE,
                                   g_param_spec_object ("viewable", NULL, NULL,
                                                        GIMP_TYPE_VIEWABLE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_viewable_undo_init (GimpViewableUndo *undo)
{
}

static void
gimp_viewable_undo_constructed (GObject *object)
{
  GimpViewableUndo *viewable_undo = GIMP_VIEWABLE_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_VIEWABLE (viewable_undo->viewable));
}

static void
gimp_viewable_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpViewableUndo *viewable_undo = GIMP_VIEWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_VIEWABLE:
      viewable_undo->viewable = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_viewable_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpViewableUndo *viewable_undo = GIMP_VIEWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_VIEWABLE:
      g_value_set_object (value, viewable_undo->viewable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_viewable_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpViewableUndo *viewable_undo = GIMP_VIEWABLE_UNDO (undo);

  if (viewable_undo->viewable)
    {
      g_object_unref (viewable_undo->viewable);
      viewable_undo->viewable = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}

static void
gimp_viewable_undo_pop (GimpUndo            *undo,
                        GimpUndoMode         undo_mode,
                        GimpUndoAccumulator *accum)
{
  GimpViewableUndo *viewable_undo  = GIMP_VIEWABLE_UNDO (undo);
  GimpViewable     *viewable       = GIMP_VIEWABLE_UNDO (undo)->viewable;

  switch (undo->undo_type)
    {
    case GIMP_UNDO_IMAGE_ATTRIBUTES:
      {
        GimpAttributes *attributes;

        attributes = gimp_attributes_duplicate (gimp_image_get_attributes (image));

        gimp_image_set_attributes (image, image_undo->attributes, FALSE);

        if (image_undo->attributes)
          g_object_unref (image_undo->attributes);
        image_undo->attributes = attributes;
      }
      break;

    default:
      g_assert_not_reached ();

    }
}