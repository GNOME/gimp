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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimageundo.h"


static GObject * gimp_image_undo_constructor (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);

static void      gimp_image_undo_pop         (GimpUndo              *undo,
                                              GimpUndoMode           undo_mode,
                                              GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpImageUndo, gimp_image_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_image_undo_parent_class


static void
gimp_image_undo_class_init (GimpImageUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_image_undo_constructor;

  undo_class->pop           = gimp_image_undo_pop;
}

static void
gimp_image_undo_init (GimpImageUndo *undo)
{
}

static GObject *
gimp_image_undo_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpImageUndo *image_undo;
  GimpImage     *image;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  image_undo = GIMP_IMAGE_UNDO (object);

  image = GIMP_UNDO (object)->image;

  image_undo->base_type       = image->base_type;
  image_undo->width           = image->width;
  image_undo->height          = image->height;
  image_undo->xresolution     = image->xresolution;
  image_undo->yresolution     = image->yresolution;
  image_undo->resolution_unit = image->resolution_unit;

  return object;
}

static void
gimp_image_undo_pop (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (undo->undo_type == GIMP_UNDO_IMAGE_TYPE)
    {
      GimpImageBaseType base_type;

      base_type = image_undo->base_type;
      image_undo->base_type = undo->image->base_type;
      g_object_set (undo->image, "base-type", base_type, NULL);

      gimp_image_colormap_changed (undo->image, -1);

      if (image_undo->base_type != undo->image->base_type)
        accum->mode_changed = TRUE;
    }
  else if (undo->undo_type == GIMP_UNDO_IMAGE_SIZE)
    {
      gint width;
      gint height;

      width  = image_undo->width;
      height = image_undo->height;

      image_undo->width  = undo->image->width;
      image_undo->height = undo->image->height;

      g_object_set (undo->image,
                    "width",  width,
                    "height", height,
                    NULL);

      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimp_image_get_mask (undo->image)));

      if (undo->image->width  != image_undo->width ||
          undo->image->height != image_undo->height)
        accum->size_changed = TRUE;
    }
  else if (undo->undo_type == GIMP_UNDO_IMAGE_RESOLUTION)
    {
      if (ABS (image_undo->xresolution - undo->image->xresolution) >= 1e-5 ||
          ABS (image_undo->yresolution - undo->image->yresolution) >= 1e-5)
        {
          gdouble xres;
          gdouble yres;

          xres = undo->image->xresolution;
          yres = undo->image->yresolution;

          undo->image->xresolution = image_undo->xresolution;
          undo->image->yresolution = image_undo->yresolution;

          image_undo->xresolution = xres;
          image_undo->yresolution = yres;

          accum->resolution_changed = TRUE;
        }

      if (image_undo->resolution_unit != undo->image->resolution_unit)
        {
          GimpUnit unit;

          unit = undo->image->resolution_unit;
          undo->image->resolution_unit = image_undo->resolution_unit;
          image_undo->resolution_unit = unit;

          accum->unit_changed = TRUE;
        }
    }
  else
    {
      g_assert_not_reached ();
    }
}
