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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmadrawable.h"
#include "ligmagrid.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-grid.h"
#include "ligmaimage-metadata.h"
#include "ligmaimage-private.h"
#include "ligmaimageundo.h"


enum
{
  PROP_0,
  PROP_PREVIOUS_ORIGIN_X,
  PROP_PREVIOUS_ORIGIN_Y,
  PROP_PREVIOUS_WIDTH,
  PROP_PREVIOUS_HEIGHT,
  PROP_GRID,
  PROP_PARASITE_NAME
};


static void     ligma_image_undo_constructed  (GObject             *object);
static void     ligma_image_undo_set_property (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void     ligma_image_undo_get_property (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static gint64   ligma_image_undo_get_memsize  (LigmaObject          *object,
                                              gint64              *gui_size);

static void     ligma_image_undo_pop          (LigmaUndo            *undo,
                                              LigmaUndoMode         undo_mode,
                                              LigmaUndoAccumulator *accum);
static void     ligma_image_undo_free         (LigmaUndo            *undo,
                                              LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaImageUndo, ligma_image_undo, LIGMA_TYPE_UNDO)

#define parent_class ligma_image_undo_parent_class


static void
ligma_image_undo_class_init (LigmaImageUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_image_undo_constructed;
  object_class->set_property     = ligma_image_undo_set_property;
  object_class->get_property     = ligma_image_undo_get_property;

  ligma_object_class->get_memsize = ligma_image_undo_get_memsize;

  undo_class->pop                = ligma_image_undo_pop;
  undo_class->free               = ligma_image_undo_free;

  g_object_class_install_property (object_class, PROP_PREVIOUS_ORIGIN_X,
                                   g_param_spec_int ("previous-origin-x",
                                                     NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE,
                                                     0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_ORIGIN_Y,
                                   g_param_spec_int ("previous-origin-y",
                                                     NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE,
                                                     0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_WIDTH,
                                   g_param_spec_int ("previous-width",
                                                     NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE,
                                                     0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_HEIGHT,
                                   g_param_spec_int ("previous-height",
                                                     NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE,
                                                     0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        LIGMA_TYPE_GRID,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_image_undo_init (LigmaImageUndo *undo)
{
}

static void
ligma_image_undo_constructed (GObject *object)
{
  LigmaImageUndo *image_undo = LIGMA_IMAGE_UNDO (object);
  LigmaImage     *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  image = LIGMA_UNDO (object)->image;

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_IMAGE_TYPE:
      image_undo->base_type = ligma_image_get_base_type (image);
      break;

    case LIGMA_UNDO_IMAGE_PRECISION:
      image_undo->precision = ligma_image_get_precision (image);
      break;

    case LIGMA_UNDO_IMAGE_SIZE:
      image_undo->width  = ligma_image_get_width  (image);
      image_undo->height = ligma_image_get_height (image);
      break;

    case LIGMA_UNDO_IMAGE_RESOLUTION:
      ligma_image_get_resolution (image,
                                 &image_undo->xresolution,
                                 &image_undo->yresolution);
      image_undo->resolution_unit = ligma_image_get_unit (image);
      break;

    case LIGMA_UNDO_IMAGE_GRID:
      ligma_assert (LIGMA_IS_GRID (image_undo->grid));
      break;

    case LIGMA_UNDO_IMAGE_COLORMAP:
      image_undo->num_colors = ligma_image_get_colormap_size (image);
      image_undo->colormap   = ligma_image_get_colormap (image);
      break;

    case LIGMA_UNDO_IMAGE_HIDDEN_PROFILE:
      g_set_object (&image_undo->hidden_profile,
                    _ligma_image_get_hidden_profile (image));
      break;

    case LIGMA_UNDO_IMAGE_METADATA:
      image_undo->metadata =
        ligma_metadata_duplicate (ligma_image_get_metadata (image));
      break;

    case LIGMA_UNDO_PARASITE_ATTACH:
    case LIGMA_UNDO_PARASITE_REMOVE:
      ligma_assert (image_undo->parasite_name != NULL);

      image_undo->parasite = ligma_parasite_copy
        (ligma_image_parasite_find (image, image_undo->parasite_name));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_image_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaImageUndo *image_undo = LIGMA_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_PREVIOUS_ORIGIN_X:
      image_undo->previous_origin_x = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_ORIGIN_Y:
      image_undo->previous_origin_y = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_WIDTH:
      image_undo->previous_width = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_HEIGHT:
      image_undo->previous_height = g_value_get_int (value);
      break;
    case PROP_GRID:
      {
        LigmaGrid *grid = g_value_get_object (value);

        if (grid)
          image_undo->grid = ligma_config_duplicate (LIGMA_CONFIG (grid));
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
ligma_image_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaImageUndo *image_undo = LIGMA_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_PREVIOUS_ORIGIN_X:
      g_value_set_int (value, image_undo->previous_origin_x);
      break;
    case PROP_PREVIOUS_ORIGIN_Y:
      g_value_set_int (value, image_undo->previous_origin_y);
      break;
    case PROP_PREVIOUS_WIDTH:
      g_value_set_int (value, image_undo->previous_width);
      break;
    case PROP_PREVIOUS_HEIGHT:
      g_value_set_int (value, image_undo->previous_height);
      break;
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
ligma_image_undo_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaImageUndo *image_undo = LIGMA_IMAGE_UNDO (object);
  gint64         memsize    = 0;

  if (image_undo->colormap)
    memsize += LIGMA_IMAGE_COLORMAP_SIZE;

  if (image_undo->metadata)
    memsize += ligma_g_object_get_memsize (G_OBJECT (image_undo->metadata));

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (image_undo->grid),
                                      gui_size);
  memsize += ligma_string_get_memsize (image_undo->parasite_name);
  memsize += ligma_parasite_get_memsize (image_undo->parasite, gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_image_undo_pop (LigmaUndo            *undo,
                     LigmaUndoMode         undo_mode,
                     LigmaUndoAccumulator *accum)
{
  LigmaImageUndo    *image_undo = LIGMA_IMAGE_UNDO (undo);
  LigmaImage        *image      = undo->image;
  LigmaImagePrivate *private    = LIGMA_IMAGE_GET_PRIVATE (image);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_IMAGE_TYPE:
      {
        LigmaImageBaseType base_type;

        base_type = image_undo->base_type;
        image_undo->base_type = ligma_image_get_base_type (image);
        g_object_set (image, "base-type", base_type, NULL);

        ligma_image_colormap_changed (image, -1);

        if (image_undo->base_type != ligma_image_get_base_type (image))
          {
            if ((image_undo->base_type            == LIGMA_GRAY) ||
                (ligma_image_get_base_type (image) == LIGMA_GRAY))
              {
                /* in case there was no profile undo, we need to emit
                 * profile-changed anyway
                 */
                ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (image));
              }

            accum->mode_changed = TRUE;
          }
      }
      break;

    case LIGMA_UNDO_IMAGE_PRECISION:
      {
        LigmaPrecision precision;

        precision = image_undo->precision;
        image_undo->precision = ligma_image_get_precision (image);
        g_object_set (image, "precision", precision, NULL);

        if (image_undo->precision != ligma_image_get_precision (image))
          accum->precision_changed = TRUE;
      }
      break;

    case LIGMA_UNDO_IMAGE_SIZE:
      {
        gint width;
        gint height;
        gint previous_origin_x;
        gint previous_origin_y;
        gint previous_width;
        gint previous_height;

        width             = image_undo->width;
        height            = image_undo->height;
        previous_origin_x = image_undo->previous_origin_x;
        previous_origin_y = image_undo->previous_origin_y;
        previous_width    = image_undo->previous_width;
        previous_height   = image_undo->previous_height;

        /* Transform to a redo */
        image_undo->width             = ligma_image_get_width  (image);
        image_undo->height            = ligma_image_get_height (image);
        image_undo->previous_origin_x = -previous_origin_x;
        image_undo->previous_origin_y = -previous_origin_y;
        image_undo->previous_width    = width;
        image_undo->previous_height   = height;

        g_object_set (image,
                      "width",  width,
                      "height", height,
                      NULL);

        ligma_drawable_invalidate_boundary
          (LIGMA_DRAWABLE (ligma_image_get_mask (image)));

        if (ligma_image_get_width  (image) != image_undo->width ||
            ligma_image_get_height (image) != image_undo->height)
          {
            accum->size_changed      = TRUE;
            accum->previous_origin_x = previous_origin_x;
            accum->previous_origin_y = previous_origin_y;
            accum->previous_width    = previous_width;
            accum->previous_height   = previous_height;
          }
      }
      break;

    case LIGMA_UNDO_IMAGE_RESOLUTION:
      {
        gdouble xres;
        gdouble yres;

        ligma_image_get_resolution (image, &xres, &yres);

        if (ABS (image_undo->xresolution - xres) >= 1e-5 ||
            ABS (image_undo->yresolution - yres) >= 1e-5)
          {
            private->xresolution = image_undo->xresolution;
            private->yresolution = image_undo->yresolution;

            image_undo->xresolution = xres;
            image_undo->yresolution = yres;

            accum->resolution_changed = TRUE;
          }
      }

      if (image_undo->resolution_unit != ligma_image_get_unit (image))
        {
          LigmaUnit unit;

          unit = ligma_image_get_unit (image);
          private->resolution_unit = image_undo->resolution_unit;
          image_undo->resolution_unit = unit;

          accum->unit_changed = TRUE;
        }
      break;

    case LIGMA_UNDO_IMAGE_GRID:
      {
        LigmaGrid *grid;

        grid = ligma_config_duplicate (LIGMA_CONFIG (ligma_image_get_grid (image)));

        ligma_image_set_grid (image, image_undo->grid, FALSE);

        g_object_unref (image_undo->grid);
        image_undo->grid = grid;
      }
      break;

    case LIGMA_UNDO_IMAGE_COLORMAP:
      {
        guchar *colormap;
        gint    num_colors;

        num_colors = ligma_image_get_colormap_size (image);
        colormap   = ligma_image_get_colormap (image);

        if (image_undo->colormap)
          ligma_image_set_colormap (image,
                                   image_undo->colormap, image_undo->num_colors,
                                   FALSE);
        else
          ligma_image_unset_colormap (image, FALSE);

        if (image_undo->colormap)
          g_free (image_undo->colormap);

        image_undo->num_colors = num_colors;
        image_undo->colormap   = colormap;
      }
      break;

    case LIGMA_UNDO_IMAGE_HIDDEN_PROFILE:
      {
        LigmaColorProfile *hidden_profile = NULL;

        g_set_object (&hidden_profile, _ligma_image_get_hidden_profile (image));
        _ligma_image_set_hidden_profile (image, image_undo->hidden_profile,
                                        FALSE);
        image_undo->hidden_profile = hidden_profile;
      }
      break;

    case LIGMA_UNDO_IMAGE_METADATA:
      {
        LigmaMetadata *metadata;

        metadata = ligma_metadata_duplicate (ligma_image_get_metadata (image));

        ligma_image_set_metadata (image, image_undo->metadata, FALSE);

        if (image_undo->metadata)
          g_object_unref (image_undo->metadata);
        image_undo->metadata = metadata;
      }
      break;

    case LIGMA_UNDO_PARASITE_ATTACH:
    case LIGMA_UNDO_PARASITE_REMOVE:
      {
        LigmaParasite *parasite = image_undo->parasite;

        image_undo->parasite = ligma_parasite_copy
          (ligma_image_parasite_find (image, image_undo->parasite_name));

        if (parasite)
          ligma_image_parasite_attach (image, parasite, FALSE);
        else
          ligma_image_parasite_detach (image, image_undo->parasite_name, FALSE);

        if (parasite)
          ligma_parasite_free (parasite);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_image_undo_free (LigmaUndo     *undo,
                      LigmaUndoMode  undo_mode)
{
  LigmaImageUndo *image_undo = LIGMA_IMAGE_UNDO (undo);

  g_clear_object (&image_undo->grid);
  g_clear_pointer (&image_undo->colormap, g_free);
  g_clear_object (&image_undo->hidden_profile);
  g_clear_object (&image_undo->metadata);
  g_clear_pointer (&image_undo->parasite_name, g_free);
  g_clear_pointer (&image_undo->parasite, ligma_parasite_free);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
