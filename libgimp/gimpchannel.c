/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpchannel.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"


/**
 * gimp_channel_new:
 * @image_ID: The image to which to add the channel.
 * @name: The channel name.
 * @width: The channel width.
 * @height: The channel height.
 * @opacity: The channel opacity.
 * @color: The channel compositing color.
 *
 * Create a new channel.
 *
 * This procedure creates a new channel with the specified width and
 * height. Name, opacity, and color are also supplied parameters. The
 * new channel still needs to be added to the image, as this is not
 * automatic. Add the new channel with the gimp_image_insert_channel()
 * command. Other attributes such as channel show masked, should be
 * set with explicit procedure calls. The channel's contents are
 * undefined initially.
 *
 * Returns: The newly created channel.
 */
gint32
gimp_channel_new (gint32         image_ID,
                  const gchar   *name,
                  guint          width,
                  guint          height,
                  gdouble        opacity,
                  const GimpRGB *color)
{
  return _gimp_channel_new (image_ID,
                            width,
                            height,
                            name,
                            opacity,
                            color);
}
