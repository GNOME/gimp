/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemlistitem.c
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "gimpdnd.h"
#include "gimpitemlistitem.h"
#include "gimpitemlistview.h"
#include "gimppreview.h"


static void   gimp_item_list_item_class_init (GimpItemListItemClass *klass);
static void   gimp_item_list_item_init       (GimpItemListItem      *list_item);

static gboolean   gimp_item_list_item_drag_drop (GtkWidget         *widget,
                                                 GdkDragContext    *context,
                                                 gint               x,
                                                 gint               y,
                                                 guint              time);


static GimpListItemClass *parent_class = NULL;


GType
gimp_item_list_item_get_type (void)
{
  static GType list_item_type = 0;

  if (! list_item_type)
    {
      static const GTypeInfo list_item_info =
      {
        sizeof (GimpItemListItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_item_list_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpItemListItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_item_list_item_init,
      };

      list_item_type = g_type_register_static (GIMP_TYPE_LIST_ITEM,
                                               "GimpItemListItem",
                                               &list_item_info, 0);
    }

  return list_item_type;
}

static void
gimp_item_list_item_class_init (GimpItemListItemClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->drag_drop = gimp_item_list_item_drag_drop;
}

static void
gimp_item_list_item_init (GimpItemListItem *list_item)
{
}

static gboolean
gimp_item_list_item_drag_drop (GtkWidget      *widget,
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
      if (widget->parent && /* EEK */
          widget->parent->parent && /* EEEEK */
          widget->parent->parent->parent && /* EEEEEEK */
          widget->parent->parent->parent->parent && /* EEEEEEEEK */
	  GIMP_IS_ITEM_LIST_VIEW (widget->parent->parent->parent->parent))
        {
          GimpItemListView      *item_view;
          GimpItemListViewClass *item_view_class;

          item_view =
	    GIMP_ITEM_LIST_VIEW (widget->parent->parent->parent->parent);

          item_view_class = GIMP_ITEM_LIST_VIEW_GET_CLASS (item_view);

          if (item_view->gimage == gimp_item_get_image (GIMP_ITEM (src_viewable)))
            {
              item_view_class->reorder_item (item_view->gimage,
                                             GIMP_ITEM (src_viewable),
                                             dest_index,
                                             TRUE, NULL);
            }
          else if (item_view_class->convert_item)
            {
              GimpItem *new_item;

              new_item = item_view_class->convert_item (GIMP_ITEM (src_viewable),
                                                        item_view->gimage);

              item_view_class->add_item (item_view->gimage,
                                         new_item,
                                         dest_index);
            }

          gimp_image_flush (item_view->gimage);
        }
      else
        {
          g_warning ("%s: GimpItemListItem is not "
                     "part of a GimpItemListView", G_GNUC_FUNCTION);
        }
    }

  return return_val;
}
