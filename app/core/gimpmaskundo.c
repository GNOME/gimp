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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "ligma-memsize.h"
#include "ligmachannel.h"
#include "ligmamaskundo.h"


enum
{
  PROP_0,
  PROP_CONVERT_FORMAT
};


static void     ligma_mask_undo_constructed  (GObject             *object);
static void     ligma_mask_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     ligma_mask_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   ligma_mask_undo_get_memsize  (LigmaObject          *object,
                                             gint64              *gui_size);

static void     ligma_mask_undo_pop          (LigmaUndo            *undo,
                                             LigmaUndoMode         undo_mode,
                                             LigmaUndoAccumulator *accum);
static void     ligma_mask_undo_free         (LigmaUndo            *undo,
                                             LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaMaskUndo, ligma_mask_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_mask_undo_parent_class


static void
ligma_mask_undo_class_init (LigmaMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_mask_undo_constructed;
  object_class->set_property     = ligma_mask_undo_set_property;
  object_class->get_property     = ligma_mask_undo_get_property;

  ligma_object_class->get_memsize = ligma_mask_undo_get_memsize;

  undo_class->pop                = ligma_mask_undo_pop;
  undo_class->free               = ligma_mask_undo_free;

  g_object_class_install_property (object_class, PROP_CONVERT_FORMAT,
                                   g_param_spec_boolean ("convert-format",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_mask_undo_init (LigmaMaskUndo *undo)
{
}

static void
ligma_mask_undo_constructed (GObject *object)
{
  LigmaMaskUndo *mask_undo = LIGMA_MASK_UNDO (object);
  LigmaItem     *item;
  LigmaDrawable *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_CHANNEL (LIGMA_ITEM_UNDO (object)->item));

  item     = LIGMA_ITEM_UNDO (object)->item;
  drawable = LIGMA_DRAWABLE (item);

  mask_undo->format = ligma_drawable_get_format (drawable);

  if (ligma_item_bounds (item,
                        &mask_undo->bounds.x,
                        &mask_undo->bounds.y,
                        &mask_undo->bounds.width,
                        &mask_undo->bounds.height))
    {
      GeglBuffer    *buffer = ligma_drawable_get_buffer (drawable);
      GeglRectangle  rect;

      gegl_rectangle_align_to_buffer (&rect, &mask_undo->bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      mask_undo->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                           rect.width,
                                                           rect.height),
                                           mask_undo->format);

      ligma_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             mask_undo->buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      mask_undo->x = rect.x;
      mask_undo->y = rect.y;
    }
}

static void
ligma_mask_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaMaskUndo *mask_undo = LIGMA_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_CONVERT_FORMAT:
      mask_undo->convert_format = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_mask_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaMaskUndo *mask_undo = LIGMA_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_CONVERT_FORMAT:
      g_value_set_boolean (value, mask_undo->convert_format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_mask_undo_get_memsize (LigmaObject *object,
                            gint64     *gui_size)
{
  LigmaMaskUndo *mask_undo = LIGMA_MASK_UNDO (object);
  gint64        memsize   = 0;

  memsize += ligma_gegl_buffer_get_memsize (mask_undo->buffer);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_mask_undo_pop (LigmaUndo            *undo,
                    LigmaUndoMode         undo_mode,
                    LigmaUndoAccumulator *accum)
{
  LigmaMaskUndo  *mask_undo  = LIGMA_MASK_UNDO (undo);
  LigmaItem      *item       = LIGMA_ITEM_UNDO (undo)->item;
  LigmaDrawable  *drawable   = LIGMA_DRAWABLE (item);
  LigmaChannel   *channel    = LIGMA_CHANNEL (item);
  GeglBuffer    *new_buffer = NULL;
  GeglRectangle  bounds     = {};
  GeglRectangle  rect       = {};
  const Babl    *format;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  format = ligma_drawable_get_format (drawable);

  if (ligma_item_bounds (item,
                        &bounds.x,
                        &bounds.y,
                        &bounds.width,
                        &bounds.height))
    {
      GeglBuffer *buffer = ligma_drawable_get_buffer (drawable);

      gegl_rectangle_align_to_buffer (&rect, &bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    rect.width, rect.height),
                                    format);

      ligma_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             new_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      gegl_buffer_clear (buffer, &rect);
    }

  if (mask_undo->convert_format)
    {
      GeglBuffer *buffer;
      gint        width  = ligma_item_get_width  (item);
      gint        height = ligma_item_get_height (item);

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                mask_undo->format);

      ligma_drawable_set_buffer (drawable, FALSE, NULL, buffer);
      g_object_unref (buffer);
    }

  if (mask_undo->buffer)
    {
      ligma_gegl_buffer_copy (mask_undo->buffer,
                             NULL,
                             GEGL_ABYSS_NONE,
                             ligma_drawable_get_buffer (drawable),
                             GEGL_RECTANGLE (mask_undo->x, mask_undo->y, 0, 0));

      g_object_unref (mask_undo->buffer);
    }

  /* invalidate the current bounds and boundary of the mask */
  ligma_drawable_invalidate_boundary (drawable);

  if (mask_undo->buffer)
    {
      channel->empty = FALSE;
      channel->x1    = mask_undo->bounds.x;
      channel->y1    = mask_undo->bounds.y;
      channel->x2    = mask_undo->bounds.x + mask_undo->bounds.width;
      channel->y2    = mask_undo->bounds.y + mask_undo->bounds.height;
    }
  else
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = ligma_item_get_width  (item);
      channel->y2    = ligma_item_get_height (item);
    }

  /* we know the bounds */
  channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mask_undo->format = format;
  mask_undo->buffer = new_buffer;
  mask_undo->bounds = bounds;
  mask_undo->x      = rect.x;
  mask_undo->y      = rect.y;

  ligma_drawable_update (drawable, 0, 0, -1, -1);
}

static void
ligma_mask_undo_free (LigmaUndo     *undo,
                     LigmaUndoMode  undo_mode)
{
  LigmaMaskUndo *mask_undo = LIGMA_MASK_UNDO (undo);

  g_clear_object (&mask_undo->buffer);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
