/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistitem.c
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

#include "apptypes.h"

#include "gdisplay.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpdnd.h"
#include "gimpdrawablelistitem.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimppreview.h"
#include "gimpviewable.h"


static void   gimp_drawable_list_item_class_init (GimpDrawableListItemClass *klass);
static void   gimp_drawable_list_item_init       (GimpDrawableListItem      *list_item);

static gboolean   gimp_drawable_list_item_drag_motion (GtkWidget         *widget,
                                                       GdkDragContext    *context,
                                                       gint               x,
                                                       gint               y,
                                                       guint              time);
static gboolean   gimp_drawable_list_item_drag_drop   (GtkWidget         *widget,
                                                       GdkDragContext    *context,
                                                       gint               x,
                                                       gint               y,
                                                       guint              time);


static GimpListItemClass *parent_class = NULL;


GtkType
gimp_drawable_list_item_get_type (void)
{
  static GtkType list_item_type = 0;

  if (! list_item_type)
    {
      static const GtkTypeInfo list_item_info =
      {
	"GimpDrawableListItem",
	sizeof (GimpDrawableListItem),
	sizeof (GimpDrawableListItemClass),
	(GtkClassInitFunc) gimp_drawable_list_item_class_init,
	(GtkObjectInitFunc) gimp_drawable_list_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      list_item_type = gtk_type_unique (GIMP_TYPE_LIST_ITEM, &list_item_info);
    }

  return list_item_type;
}

static void
gimp_drawable_list_item_class_init (GimpDrawableListItemClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_LIST_ITEM);

  widget_class->drag_motion = gimp_drawable_list_item_drag_motion;
  widget_class->drag_drop   = gimp_drawable_list_item_drag_drop;
}

static void
gimp_drawable_list_item_init (GimpDrawableListItem *list_item)
{
}

static gboolean
gimp_drawable_list_item_drag_motion (GtkWidget      *widget,
                                     GdkDragContext *context,
                                     gint            x,
                                     gint            y,
                                     guint           time)
{
  GimpListItem  *list_item;
  GimpViewable  *src_viewable;
  gint           dest_index;
  GdkDragAction  drag_action;
  GimpDropType   drop_type;
  gboolean       return_val;

  list_item = GIMP_LIST_ITEM (widget);

  return_val = gimp_list_item_check_drag (list_item, context, x, y,
                                          &src_viewable,
                                          &dest_index,
                                          &drag_action,
                                          &drop_type);

  gdk_drag_status (context, drag_action, time);

  list_item->drop_type = drop_type;

  return return_val;
}

static gboolean
gimp_drawable_list_item_drag_drop (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
{
  GimpListItem  *list_item;
  GimpViewable  *src_viewable;
  gint           dest_index;
  GdkDragAction  drag_action;
  GimpDropType   drop_type;
  gboolean       return_val;

  list_item = GIMP_LIST_ITEM (widget);

  return_val = gimp_list_item_check_drag (list_item, context, x, y,
                                          &src_viewable,
                                          &dest_index,
                                          &drag_action,
                                          &drop_type);

  gtk_drag_finish (context, return_val, FALSE, time);

  list_item->drop_type = GIMP_DROP_NONE;

  if (return_val)
    {
      if (GIMP_IS_LAYER (src_viewable))
        {
          gimp_image_position_layer (gimp_drawable_gimage (GIMP_DRAWABLE (src_viewable)),
                                     GIMP_LAYER (src_viewable),
                                     dest_index,
                                     TRUE);
          gdisplays_flush ();
        }
      else if (GIMP_IS_CHANNEL (src_viewable))
        {
          gimp_image_position_channel (gimp_drawable_gimage (GIMP_DRAWABLE (src_viewable)),
                                       GIMP_CHANNEL (src_viewable),
                                       dest_index);
          gdisplays_flush ();
        }
    }

  return return_val;
}
