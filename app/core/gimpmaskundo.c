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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "gimp-memsize.h"
#include "gimpchannel.h"
#include "gimpmaskundo.h"


enum
{
  PROP_0,
  PROP_CONVERT_FORMAT
};


static void     gimp_mask_undo_constructed  (GObject             *object);
static void     gimp_mask_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     gimp_mask_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   gimp_mask_undo_get_memsize  (GimpObject          *object,
                                             gint64              *gui_size);

static void     gimp_mask_undo_pop          (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode,
                                             GimpUndoAccumulator *accum);
static void     gimp_mask_undo_free         (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpMaskUndo, gimp_mask_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_mask_undo_parent_class


static void
gimp_mask_undo_class_init (GimpMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_mask_undo_constructed;
  object_class->set_property     = gimp_mask_undo_set_property;
  object_class->get_property     = gimp_mask_undo_get_property;

  gimp_object_class->get_memsize = gimp_mask_undo_get_memsize;

  undo_class->pop                = gimp_mask_undo_pop;
  undo_class->free               = gimp_mask_undo_free;

  g_object_class_install_property (object_class, PROP_CONVERT_FORMAT,
                                   g_param_spec_boolean ("convert-format",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_mask_undo_init (GimpMaskUndo *undo)
{
}

static void
gimp_mask_undo_constructed (GObject *object)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (object);
  GimpItem     *item;
  GimpDrawable *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CHANNEL (GIMP_ITEM_UNDO (object)->item));

  item     = GIMP_ITEM_UNDO (object)->item;
  drawable = GIMP_DRAWABLE (item);

  mask_undo->format = gimp_drawable_get_format (drawable);

  if (gimp_item_bounds (item,
                        &mask_undo->bounds.x,
                        &mask_undo->bounds.y,
                        &mask_undo->bounds.width,
                        &mask_undo->bounds.height))
    {
      GeglBuffer    *buffer = gimp_drawable_get_buffer (drawable);
      GeglRectangle  rect;

      gegl_rectangle_align_to_buffer (&rect, &mask_undo->bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      mask_undo->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                           rect.width,
                                                           rect.height),
                                           mask_undo->format);

      gimp_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             mask_undo->buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      mask_undo->x = rect.x;
      mask_undo->y = rect.y;
    }
}

static void
gimp_mask_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (object);

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
gimp_mask_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (object);

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
gimp_mask_undo_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (object);
  gint64        memsize   = 0;

  memsize += gimp_gegl_buffer_get_memsize (mask_undo->buffer);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_mask_undo_pop (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  GimpMaskUndo  *mask_undo  = GIMP_MASK_UNDO (undo);
  GimpItem      *item       = GIMP_ITEM_UNDO (undo)->item;
  GimpDrawable  *drawable   = GIMP_DRAWABLE (item);
  GimpChannel   *channel    = GIMP_CHANNEL (item);
  GeglBuffer    *new_buffer = NULL;
  GeglRectangle  bounds     = {};
  GeglRectangle  rect       = {};
  const Babl    *format;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  format = gimp_drawable_get_format (drawable);

  if (gimp_item_bounds (item,
                        &bounds.x,
                        &bounds.y,
                        &bounds.width,
                        &bounds.height))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (drawable);

      gegl_rectangle_align_to_buffer (&rect, &bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    rect.width, rect.height),
                                    format);

      gimp_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             new_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      gegl_buffer_clear (buffer, &rect);
    }

  if (mask_undo->convert_format)
    {
      GeglBuffer *buffer;
      gint        width  = gimp_item_get_width  (item);
      gint        height = gimp_item_get_height (item);

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                mask_undo->format);

      gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
      g_object_unref (buffer);
    }

  if (mask_undo->buffer)
    {
      gimp_gegl_buffer_copy (mask_undo->buffer,
                             NULL,
                             GEGL_ABYSS_NONE,
                             gimp_drawable_get_buffer (drawable),
                             GEGL_RECTANGLE (mask_undo->x, mask_undo->y, 0, 0));

      g_object_unref (mask_undo->buffer);
    }

  /* invalidate the current bounds and boundary of the mask */
  gimp_drawable_invalidate_boundary (drawable);

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
      channel->x2    = gimp_item_get_width  (item);
      channel->y2    = gimp_item_get_height (item);
    }

  /* we know the bounds */
  channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mask_undo->format = format;
  mask_undo->buffer = new_buffer;
  mask_undo->bounds = bounds;
  mask_undo->x      = rect.x;
  mask_undo->y      = rect.y;

  gimp_drawable_update (drawable, 0, 0, -1, -1);
}

static void
gimp_mask_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (undo);

  g_clear_object (&mask_undo->buffer);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
