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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-grid.h"
#include "gimpimageundo.h"
#include "gimpparasitelist.h"


enum
{
  PROP_0,
  PROP_GRID,
  PROP_PARASITE_NAME
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

static gint64    gimp_image_undo_get_memsize  (GimpObject            *object,
                                               gint64                *gui_size);

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
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructor      = gimp_image_undo_constructor;
  object_class->set_property     = gimp_image_undo_set_property;
  object_class->get_property     = gimp_image_undo_get_property;

  gimp_object_class->get_memsize = gimp_image_undo_get_memsize;

  undo_class->pop                = gimp_image_undo_pop;
  undo_class->free               = gimp_image_undo_free;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        GIMP_TYPE_GRID,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
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
      break;

    case GIMP_UNDO_IMAGE_COLORMAP:
      image_undo->num_colors = gimp_image_get_colormap_size (image);
      image_undo->colormap   = g_memdup (gimp_image_get_colormap (image),
                                         image_undo->num_colors * 3);
      break;

    case GIMP_UNDO_PARASITE_ATTACH:
    case GIMP_UNDO_PARASITE_REMOVE:
      g_assert (image_undo->parasite_name != NULL);

      image_undo->parasite = gimp_parasite_copy
        (gimp_image_parasite_find (image, image_undo->parasite_name));
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
    case PROP_PARASITE_NAME:
      image_undo->parasite_name = g_value_dup_string (value);
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
    case PROP_PARASITE_NAME:
      g_value_set_string (value, image_undo->parasite_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_image_undo_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (object);
  gint64         memsize    = 0;

  if (image_undo->colormap)
    memsize += image_undo->num_colors * 3;

  if (image_undo->grid)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (image_undo->grid),
                                        gui_size);

  if (image_undo->parasite_name)
    memsize += strlen (image_undo->parasite_name) + 1;

  if (image_undo->parasite)
    memsize += (sizeof (GimpParasite) +
                strlen (image_undo->parasite->name) + 1 +
                image_undo->parasite->size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_image_undo_pop (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  GimpImageUndo *image_undo = GIMP_IMAGE_UNDO (undo);
  GimpImage     *image      = undo->image;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_IMAGE_TYPE:
      {
        GimpImageBaseType base_type;

        base_type = image_undo->base_type;
        image_undo->base_type = image->base_type;
        g_object_set (image, "base-type", base_type, NULL);

        gimp_image_colormap_changed (image, -1);

        if (image_undo->base_type != image->base_type)
          accum->mode_changed = TRUE;
      }
      break;

    case GIMP_UNDO_IMAGE_SIZE:
      {
        gint width;
        gint height;

        width  = image_undo->width;
        height = image_undo->height;

        image_undo->width  = image->width;
        image_undo->height = image->height;

        g_object_set (image,
                      "width",  width,
                      "height", height,
                      NULL);

        gimp_drawable_invalidate_boundary
          (GIMP_DRAWABLE (gimp_image_get_mask (image)));

        if (image->width  != image_undo->width ||
            image->height != image_undo->height)
          accum->size_changed = TRUE;
      }
      break;

    case GIMP_UNDO_IMAGE_RESOLUTION:
      if (ABS (image_undo->xresolution - image->xresolution) >= 1e-5 ||
          ABS (image_undo->yresolution - image->yresolution) >= 1e-5)
        {
          gdouble xres;
          gdouble yres;

          xres = image->xresolution;
          yres = image->yresolution;

          image->xresolution = image_undo->xresolution;
          image->yresolution = image_undo->yresolution;

          image_undo->xresolution = xres;
          image_undo->yresolution = yres;

          accum->resolution_changed = TRUE;
        }

      if (image_undo->resolution_unit != image->resolution_unit)
        {
          GimpUnit unit;

          unit = image->resolution_unit;
          image->resolution_unit = image_undo->resolution_unit;
          image_undo->resolution_unit = unit;

          accum->unit_changed = TRUE;
        }
      break;

    case GIMP_UNDO_IMAGE_GRID:
      {
        GimpGrid *grid;

        grid = gimp_config_duplicate (GIMP_CONFIG (image->grid));

        gimp_image_set_grid (image, image_undo->grid, FALSE);

        g_object_unref (image_undo->grid);
        image_undo->grid = grid;
      }
      break;

    case GIMP_UNDO_IMAGE_COLORMAP:
      {
        guchar *colormap;
        gint    num_colors;

        num_colors = gimp_image_get_colormap_size (image);
        colormap   = g_memdup (gimp_image_get_colormap (image),
                               num_colors * 3);

        gimp_image_set_colormap (image,
                                 image_undo->colormap, image_undo->num_colors,
                                 FALSE);

        if (image_undo->colormap)
          g_free (image_undo->colormap);

        image_undo->num_colors = num_colors;
        image_undo->colormap   = colormap;
      }
      break;

    case GIMP_UNDO_PARASITE_ATTACH:
    case GIMP_UNDO_PARASITE_REMOVE:
      {
        GimpParasite *parasite = image_undo->parasite;
        const gchar  *name;

        image_undo->parasite = gimp_parasite_copy
          (gimp_image_parasite_find (image, image_undo->parasite_name));

        if (parasite)
          gimp_parasite_list_add (image->parasites, parasite);
        else
          gimp_parasite_list_remove (image->parasites,
                                     image_undo->parasite_name);

        name = parasite ? parasite->name : image_undo->parasite_name;

        if (strcmp (name, "icc-profile") == 0)
          gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));

        if (parasite)
          gimp_parasite_free (parasite);
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

  if (image_undo->parasite_name)
    {
      g_free (image_undo->parasite_name);
      image_undo->parasite_name = NULL;
    }

  if (image_undo->parasite)
    {
      gimp_parasite_free (image_undo->parasite);
      image_undo->parasite = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
