/* Gimp - The GNU Image Manipulation Program
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

#include "gimpimage.h"
#include "gimpchannel.h"
#include "gimpchannelpropundo.h"


static void   gimp_channel_prop_undo_constructed (GObject             *object);
static void   gimp_channel_prop_undo_finalize    (GObject             *object);

static void   gimp_channel_prop_undo_pop         (GimpUndo            *undo,
                                                  GimpUndoMode         undo_mode,
                                                  GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpChannelPropUndo, gimp_channel_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_channel_prop_undo_parent_class


static void
gimp_channel_prop_undo_class_init (GimpChannelPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_channel_prop_undo_constructed;
  object_class->finalize    = gimp_channel_prop_undo_finalize;

  undo_class->pop           = gimp_channel_prop_undo_pop;
}

static void
gimp_channel_prop_undo_init (GimpChannelPropUndo *undo)
{
}

static void
gimp_channel_prop_undo_constructed (GObject *object)
{
  GimpChannelPropUndo *channel_prop_undo = GIMP_CHANNEL_PROP_UNDO (object);
  GimpChannel         *channel;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CHANNEL (GIMP_ITEM_UNDO (object)->item));

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_CHANNEL_COLOR:
      channel_prop_undo->color = gegl_color_duplicate (gimp_channel_get_color (channel));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_channel_prop_undo_finalize (GObject *object)
{
  GimpChannelPropUndo *channel_prop_undo = GIMP_CHANNEL_PROP_UNDO (object);

  g_clear_object (&channel_prop_undo->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case GIMP_UNDO_CHANNEL_COLOR:
      {
        GeglColor *color;

        color = gegl_color_duplicate (gimp_channel_get_color (channel));
        gimp_channel_set_color (channel, channel_prop_undo->color, FALSE);
        g_clear_object (&channel_prop_undo->color);
        channel_prop_undo->color = color;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
