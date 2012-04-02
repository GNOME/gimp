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

#include <gegl.h>

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp-utils.h"
#include "gimpchannel.h"
#include "gimpmaskundo.h"


static void     gimp_mask_undo_constructed (GObject             *object);

static gint64   gimp_mask_undo_get_memsize (GimpObject          *object,
                                            gint64              *gui_size);

static void     gimp_mask_undo_pop         (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     gimp_mask_undo_free        (GimpUndo            *undo,
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

  gimp_object_class->get_memsize = gimp_mask_undo_get_memsize;

  undo_class->pop                = gimp_mask_undo_pop;
  undo_class->free               = gimp_mask_undo_free;
}

static void
gimp_mask_undo_init (GimpMaskUndo *undo)
{
}

static void
gimp_mask_undo_constructed (GObject *object)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (object);
  GimpChannel  *channel;
  gint          x1, y1, x2, y2;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_CHANNEL (GIMP_ITEM_UNDO (object)->item));

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (object)->item);

  if (gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    {
      GimpDrawable  *drawable    = GIMP_DRAWABLE (channel);

      mask_undo->buffer = gegl_buffer_new (GEGL_RECTANGLE(0,0,x2-x1, y2-y1),
                                           babl_format ("Y float"));

      gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                        GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                        mask_undo->buffer,
                        GEGL_RECTANGLE (0, 0, 0, 0));

      mask_undo->x = x1;
      mask_undo->y = y1;
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
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (undo);
  GimpChannel  *channel   = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);
  GimpDrawable *drawable  = GIMP_DRAWABLE (channel);
  GeglBuffer   *new_buffer;
  gint          x1, y1, x2, y2;
  gint          width  = 0;
  gint          height = 0;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    {
      GeglRectangle  src_rect    = { x1, y1, x2 - x1, y2 - y1 };

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                                    babl_format ("Y float"));

      gegl_buffer_copy (gimp_drawable_get_buffer (drawable), &src_rect,
                        new_buffer, GEGL_RECTANGLE (0,0,0,0));

      gegl_buffer_clear (gimp_drawable_get_buffer (drawable), &src_rect);
    }
  else
    {
      new_buffer = NULL;
    }

  if (mask_undo->buffer)
    {
      GeglRectangle  dest_rect = { mask_undo->x, mask_undo->y, 0, 0 };

      width  = gegl_buffer_get_width  (mask_undo->buffer);
      height = gegl_buffer_get_height (mask_undo->buffer);

      gegl_buffer_copy (mask_undo->buffer, NULL,
                        gimp_drawable_get_buffer (drawable), &dest_rect);

      g_object_unref (mask_undo->buffer);
    }

  /* invalidate the current bounds and boundary of the mask */
  gimp_drawable_invalidate_boundary (drawable);

  if (mask_undo->buffer)
    {
      channel->empty = FALSE;
      channel->x1    = mask_undo->x;
      channel->y1    = mask_undo->y;
      channel->x2    = mask_undo->x + width;
      channel->y2    = mask_undo->y + height;
    }
  else
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = gimp_item_get_width  (GIMP_ITEM (channel));
      channel->y2    = gimp_item_get_height (GIMP_ITEM (channel));
    }

  /* we know the bounds */
  channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mask_undo->buffer = new_buffer;
  mask_undo->x      = x1;
  mask_undo->y      = y1;

  gimp_drawable_update (GIMP_DRAWABLE (channel),
                        0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_mask_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpMaskUndo *mask_undo = GIMP_MASK_UNDO (undo);

  if (mask_undo->buffer)
    {
      g_object_unref (mask_undo->buffer);
      mask_undo->buffer = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
