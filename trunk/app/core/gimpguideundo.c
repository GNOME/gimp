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

#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpguide.h"
#include "gimpguideundo.h"


enum
{
  PROP_0,
  PROP_GUIDE
};


static GObject * gimp_guide_undo_constructor  (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);
static void      gimp_guide_undo_set_property (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void      gimp_guide_undo_get_property (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);

static void      gimp_guide_undo_pop          (GimpUndo              *undo,
                                               GimpUndoMode           undo_mode,
                                               GimpUndoAccumulator   *accum);
static void      gimp_guide_undo_free         (GimpUndo              *undo,
                                               GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpGuideUndo, gimp_guide_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_guide_undo_parent_class


static void
gimp_guide_undo_class_init (GimpGuideUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor  = gimp_guide_undo_constructor;
  object_class->set_property = gimp_guide_undo_set_property;
  object_class->get_property = gimp_guide_undo_get_property;

  undo_class->pop            = gimp_guide_undo_pop;
  undo_class->free           = gimp_guide_undo_free;

  g_object_class_install_property (object_class, PROP_GUIDE,
                                   g_param_spec_object ("guide", NULL, NULL,
                                                        GIMP_TYPE_GUIDE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_guide_undo_init (GimpGuideUndo *undo)
{
}

static GObject *
gimp_guide_undo_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpGuideUndo *guide_undo;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  guide_undo = GIMP_GUIDE_UNDO (object);

  g_assert (GIMP_IS_GUIDE (guide_undo->guide));

  guide_undo->orientation = gimp_guide_get_orientation (guide_undo->guide);
  guide_undo->position    = gimp_guide_get_position (guide_undo->guide);

  return object;
}

static void
gimp_guide_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpGuideUndo *guide_undo = GIMP_GUIDE_UNDO (object);

  switch (property_id)
    {
    case PROP_GUIDE:
      guide_undo->guide = (GimpGuide *) g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_guide_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpGuideUndo *guide_undo = GIMP_GUIDE_UNDO (object);

  switch (property_id)
    {
    case PROP_GUIDE:
      g_value_set_object (value, guide_undo->guide);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_guide_undo_pop (GimpUndo              *undo,
                     GimpUndoMode           undo_mode,
                     GimpUndoAccumulator   *accum)
{
  GimpGuideUndo       *guide_undo = GIMP_GUIDE_UNDO (undo);
  GimpOrientationType  orientation;
  gint                 position;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  orientation = gimp_guide_get_orientation (guide_undo->guide);
  position    = gimp_guide_get_position (guide_undo->guide);

  /*  add and move guides manually (nor using the gimp_image_guide
   *  API), because we might be in the middle of an image resizing
   *  undo group and the guide's position might be temporarily out of
   *  image.
   */

  if (position == -1)
    {
      undo->image->guides = g_list_prepend (undo->image->guides,
                                            guide_undo->guide);
      gimp_guide_set_position (guide_undo->guide, guide_undo->position);
      g_object_ref (guide_undo->guide);
      gimp_image_update_guide (undo->image, guide_undo->guide);
    }
  else if (guide_undo->position == -1)
    {
      gimp_image_remove_guide (undo->image, guide_undo->guide, FALSE);
    }
  else
    {
      gimp_image_update_guide (undo->image, guide_undo->guide);
      gimp_guide_set_position (guide_undo->guide, guide_undo->position);
      gimp_image_update_guide (undo->image, guide_undo->guide);
    }

  gimp_guide_set_orientation (guide_undo->guide, guide_undo->orientation);

  guide_undo->position    = position;
  guide_undo->orientation = orientation;
}

static void
gimp_guide_undo_free (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  GimpGuideUndo *guide_undo = GIMP_GUIDE_UNDO (undo);

  if (guide_undo->guide)
    {
      g_object_unref (guide_undo->guide);
      guide_undo->guide = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
