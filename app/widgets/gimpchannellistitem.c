/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchannellistitem.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "gimpdnd.h"
#include "gimpchannellistitem.h"
#include "gimppreview.h"


static void   gimp_channel_list_item_class_init (GimpChannelListItemClass *klass);
static void   gimp_channel_list_item_init       (GimpChannelListItem      *list_item);

static void   gimp_channel_list_item_drop_color (GtkWidget     *widget,
						 const GimpRGB *color,
						 gpointer       data);


GType
gimp_channel_list_item_get_type (void)
{
  static GType list_item_type = 0;

  if (! list_item_type)
    {
      static const GTypeInfo list_item_info =
      {
        sizeof (GimpChannelListItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_channel_list_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpChannelListItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_channel_list_item_init,
      };

      list_item_type = g_type_register_static (GIMP_TYPE_DRAWABLE_LIST_ITEM,
                                               "GimpChannelListItem",
                                               &list_item_info, 0);
    }

  return list_item_type;
}

static void
gimp_channel_list_item_class_init (GimpChannelListItemClass *klass)
{
}

static void
gimp_channel_list_item_init (GimpChannelListItem *list_item)
{
  gimp_dnd_color_dest_add (GTK_WIDGET (list_item),
			   gimp_channel_list_item_drop_color, NULL);
}

static void
gimp_channel_list_item_drop_color (GtkWidget     *widget,
				   const GimpRGB *color,
				   gpointer       data)
{
  GimpChannel *channel;

  channel =
    GIMP_CHANNEL (GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview)->viewable);

  gimp_channel_set_color (channel, color);

  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (channel)));
}
