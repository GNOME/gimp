/* Gimp - The GNU Image Manipulation Program
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

#include "gimpimage.h"
#include "gimpchannel.h"
#include "gimpchannelpropundo.h"


static GObject * gimp_channel_prop_undo_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);

static void      gimp_channel_prop_undo_pop         (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode,
                                                     GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpChannelPropUndo, gimp_channel_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_channel_prop_undo_parent_class


static void
gimp_channel_prop_undo_class_init (GimpChannelPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_channel_prop_undo_constructor;

  undo_class->pop           = gimp_channel_prop_undo_pop;
}

static void
gimp_channel_prop_undo_init (GimpChannelPropUndo *undo)
{
}

static GObject *
gimp_channel_prop_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpChannelPropUndo *channel_prop_undo;
  GimpImage           *image;
  GimpChannel         *channel;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  channel_prop_undo = GIMP_CHANNEL_PROP_UNDO (object);

  g_assert (GIMP_IS_CHANNEL (GIMP_ITEM_UNDO (object)->item));

  image   = GIMP_UNDO (object)->image;
  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_CHANNEL_REPOSITION:
      channel_prop_undo->position = gimp_image_get_channel_index (image, channel);
      break;

    case GIMP_UNDO_CHANNEL_COLOR:
      gimp_channel_get_color (channel, &channel_prop_undo->color);
      break;

    default:
      g_assert_not_reached ();
    }

  return object;
}

static void
gimp_channel_prop_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpChannelPropUndo *channel_prop_undo = GIMP_CHANNEL_PROP_UNDO (undo);
  GimpChannel         *channel           = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_CHANNEL_REPOSITION:
      {
        gint position;

        position = gimp_image_get_channel_index (undo->image, channel);
        gimp_image_position_channel (undo->image, channel,
                                     channel_prop_undo->position,
                                     FALSE, NULL);
        channel_prop_undo->position = position;
      }
      break;

    case GIMP_UNDO_CHANNEL_COLOR:
      {
        GimpRGB color;

        gimp_channel_get_color (channel, &color);
        gimp_channel_set_color (channel, &channel_prop_undo->color, FALSE);
        channel_prop_undo->color = color;
      }
      break;

    default:
      g_assert_not_reached ();
    }
}
