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
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-grid.h"
#include "gimpimageundo.h"


enum
{
  PROP_0,
  PROP_GRID
};


static GObject * gimp_image_undo_constructor  (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);
static void      gimp_image_undo_set_property (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void      gimp_image_undo_get_property (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);

static void      gimp_image_undo_pop          (GimpUndo              *undo,
                                               GimpUndoMode           undo_mode,
                                               GimpUndoAccumulator   *accum);
static void      gimp_image_undo_free         (GimpUndo              *undo,
                                               GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpImageUndo, gimp_image_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_image_undo_parent_class


static void
gimp_image_undo_class_init (GimpImageUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor  = gimp_image_undo_constructor;
  object_class->set_property = gimp_image_undo_set_property;
  object_class->get_property = gimp_image_undo_get_property;

  undo_class->pop            = gimp_image_undo_pop;
  undo_class->free           = gimp_image_undo_free;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        GIMP_TYPE_GRID,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
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

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_IMAGE_TYPE:
      image_undo->base_type = image->base_type;
      break;

    case GIMP_UNDO_IMAGE_SIZE:
      image_undo->width  = image->width;
      image_undo->height = image->height;
      break;

    case GIMP_UNDO_IMAGE_RESOLUTION:
      image_undo->xresolution     = image->xresolution;
      image_undo->yresolution     = image->yresolution;
      image_undo->resolution_unit = image->resolution_unit;
      break;

    case GIMP_UNDO_IMAGE_GRID:
      g_assert (GIMP_IS_GRID (image_undo->grid));

      GIMP_UNDO (object)->size +=
        gimp_object_get_memsize (GIMP_OBJECT (image_undo->grid), NULL);
      break;

    case GIMP_UNDO_IMAGE_COLORMAP:
      image_undo->num_colors = gimp_image_get_colormap_size (image);
      image_undo->colormap   = g_memdup (gimp_image_get_colormap (image),
                                         image_undo->num_colors * 3);

      GIMP_UNDO (object)->size += image_undo->num_colors * 3;
      break;

    default:
      g_assert_not_reached ();
    }

  return object;
}

static void
gimp_image_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_GRID:
      {
        GimpGrid *grid = g_value_get_object (value);

        if (grid)
          image_undo->grid = gimp_config_duplicate (GIMP_CONFIG (grid));
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_GRID:
      g_value_set_object (value, image_undo->grid);
       break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_undo_pop (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_IMAGE_TYPE:
      {
        GimpImageBaseType base_type;

        base_type = image_undo->base_type;
        image_undo->base_type = undo->image->base_type;
        g_object_set (undo->image, "base-type", base_type, NULL);

        gimp_image_colormap_changed (undo->image, -1);

        if (image_undo->base_type != undo->image->base_type)
          accum->mode_changed = TRUE;
      }
      break;

    case GIMP_UNDO_IMAGE_SIZE:
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
      break;

    case GIMP_UNDO_IMAGE_RESOLUTION:
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
      break;

    case GIMP_UNDO_IMAGE_GRID:
      {
        GimpGrid *grid;

        undo->size -= gimp_object_get_memsize (GIMP_OBJECT (image_undo->grid),
                                               NULL);

        grid = gimp_config_duplicate (GIMP_CONFIG (undo->image->grid));

        gimp_image_set_grid (undo->image, image_undo->grid, FALSE);

        g_object_unref (image_undo->grid);
        image_undo->grid = grid;

        undo->size += gimp_object_get_memsize (GIMP_OBJECT (image_undo->grid),
                                               NULL);
      }
      break;

    case GIMP_UNDO_IMAGE_COLORMAP:
      {
        guchar *colormap;
        gint    num_colors;

        undo->size -= image_undo->num_colors * 3;

        num_colors = gimp_image_get_colormap_size (undo->image);
        colormap   = g_memdup (gimp_image_get_colormap (undo->image),
                               num_colors * 3);

        gimp_image_set_colormap (undo->image,
                                 image_undo->colormap, image_undo->num_colors,
                                 FALSE);

        if (image_undo->colormap)
          g_free (image_undo->colormap);

        image_undo->num_colors = num_colors;
        image_undo->colormap   = colormap;

        undo->size += image_undo->num_colors * 3;
      }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
gimp_image_undo_free (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (undo);

  if (image_undo->grid)
    {
      g_object_unref (image_undo->grid);
      image_undo->grid = NULL;
    }

  if (image_undo->colormap)
    {
      g_free (image_undo->colormap);
      image_undo->colormap = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
