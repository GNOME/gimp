/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplistitem.c
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

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimplayer.h"
#include "core/gimpviewable.h"

#include "gimpchannellistitem.h"
#include "gimpdnd.h"
#include "gimpdrawablelistitem.h"
#include "gimplayerlistitem.h"
#include "gimplistitem.h"
#include "gimppreview.h"


static void           gimp_list_item_class_init    (GimpListItemClass *klass);
static void           gimp_list_item_init          (GimpListItem      *list_item);

static void           gimp_list_item_real_set_viewable (GimpListItem  *list_item,
                                                        GimpViewable  *viewable);
static void       gimp_list_item_real_set_preview_size (GimpListItem  *list_item);

static gboolean       gimp_list_item_expose_event  (GtkWidget         *widget,
                                                    GdkEventExpose    *eevent);
static void           gimp_list_item_drag_leave    (GtkWidget         *widget,
                                                    GdkDragContext    *context,
                                                    guint              time);
static gboolean       gimp_list_item_drag_motion   (GtkWidget         *widget,
                                                    GdkDragContext    *context,
                                                    gint               x,
                                                    gint               y,
                                                    guint              time);
static gboolean       gimp_list_item_drag_drop     (GtkWidget         *widget,
                                                    GdkDragContext    *context,
                                                    gint               x,
                                                    gint               y,
                                                    guint              time);

static void           gimp_list_item_name_changed  (GimpViewable      *viewable,
                                                    GimpListItem      *list_item);
static GimpViewable * gimp_list_item_drag_viewable (GtkWidget         *widget,
                                                    gpointer           data);


static GtkListItemClass *parent_class = NULL;


GType
gimp_list_item_get_type (void)
{
  static GType list_item_type = 0;

  if (! list_item_type)
    {
      static const GTypeInfo list_item_info =
      {
        sizeof (GimpListItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_list_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpListItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_list_item_init,
      };

      list_item_type = g_type_register_static (GTK_TYPE_LIST_ITEM,
                                               "GimpListItem",
                                               &list_item_info, 0);
    }

  return list_item_type;
}

static void
gimp_list_item_class_init (GimpListItemClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->expose_event = gimp_list_item_expose_event;
  widget_class->drag_leave   = gimp_list_item_drag_leave;
  widget_class->drag_motion  = gimp_list_item_drag_motion;
  widget_class->drag_drop    = gimp_list_item_drag_drop;

  klass->set_viewable        = gimp_list_item_real_set_viewable;
  klass->set_preview_size    = gimp_list_item_real_set_preview_size;
}

static void
gimp_list_item_init (GimpListItem *list_item)
{
  list_item->hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (list_item->hbox), 2);
  gtk_container_add (GTK_CONTAINER (list_item), list_item->hbox);
  gtk_widget_show (list_item->hbox);

  list_item->preview       = NULL;
  list_item->name_label    = NULL;

  list_item->preview_size  = 0;

  list_item->reorderable   = FALSE;
  list_item->convertable   = FALSE;
  list_item->drop_type     = GIMP_DROP_NONE;
  list_item->container     = NULL;
  list_item->get_name_func = NULL;
}

static gboolean
gimp_list_item_expose_event (GtkWidget      *widget,
			     GdkEventExpose *eevent)
{
  GimpListItem *list_item;

  list_item = GIMP_LIST_ITEM (widget);

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);

  if (list_item->drop_type != GIMP_DROP_NONE)
    {
      gint x, y;

      x = list_item->name_label->allocation.x;

      y = ((list_item->drop_type == GIMP_DROP_ABOVE) ?
	   3 : widget->allocation.height - 4);

      gdk_gc_set_line_attributes (widget->style->black_gc, 5, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_MITER);

      gdk_gc_set_clip_rectangle (widget->style->black_gc, &eevent->area);

      gdk_draw_line (widget->window, widget->style->black_gc,
		     x, y,
                     widget->allocation.width - 3, y);

      gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);

      gdk_gc_set_line_attributes (widget->style->black_gc, 0, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_MITER);
    }

  return TRUE;
}

static void
gimp_list_item_drag_leave (GtkWidget      *widget,
                           GdkDragContext *context,
                           guint           time)
{
  GimpListItem *list_item;

  list_item = GIMP_LIST_ITEM (widget);

  list_item->drop_type = GIMP_DROP_NONE;
}

static gboolean
gimp_list_item_drag_motion (GtkWidget      *widget,
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

  if (list_item->drop_type != drop_type)
    {
      list_item->drop_type = drop_type;

      gtk_widget_queue_draw (widget);
    }

  return return_val;
}

static gboolean
gimp_list_item_drag_drop (GtkWidget      *widget,
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
      gimp_container_reorder (list_item->container, GIMP_OBJECT (src_viewable),
                              dest_index);
    }

  return return_val;
}

GtkWidget *
gimp_list_item_new (GimpViewable  *viewable,
                    gint           preview_size)
{
  GimpListItem *list_item;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 256, NULL);

  if (GIMP_IS_LAYER (viewable))
    {
      list_item = g_object_new (GIMP_TYPE_LAYER_LIST_ITEM, NULL);
    }
  else if (GIMP_IS_CHANNEL (viewable))
    {
      list_item = g_object_new (GIMP_TYPE_CHANNEL_LIST_ITEM, NULL);
    }
  else if (GIMP_IS_DRAWABLE (viewable))
    {
      list_item = g_object_new (GIMP_TYPE_DRAWABLE_LIST_ITEM, NULL);
    }
  else if (GIMP_IS_ITEM (viewable))
    {
      list_item = g_object_new (GIMP_TYPE_ITEM_LIST_ITEM, NULL);
    }
  else
    {
      list_item = g_object_new (GIMP_TYPE_LIST_ITEM, NULL);
    }

  list_item->preview_size = preview_size;

  gimp_list_item_set_viewable (list_item, viewable);

  return GTK_WIDGET (list_item);
}

void
gimp_list_item_set_viewable (GimpListItem *list_item,
                             GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));

  GIMP_LIST_ITEM_GET_CLASS (list_item)->set_viewable (list_item, viewable);
}

static void
gimp_list_item_real_set_viewable (GimpListItem *list_item,
                                  GimpViewable *viewable)
{
  list_item->preview = gimp_preview_new (viewable, list_item->preview_size,
                                         1, FALSE);
  gtk_box_pack_start (GTK_BOX (list_item->hbox), list_item->preview,
                      FALSE, FALSE, 0);
  gtk_widget_show (list_item->preview);

  list_item->name_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (list_item->hbox), list_item->name_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (list_item->name_label);

  gimp_list_item_name_changed (viewable, list_item);

  g_signal_connect_object (G_OBJECT (viewable), "name_changed",
                           G_CALLBACK (gimp_list_item_name_changed),
                           list_item, 0);

  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (list_item),
				    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				    G_TYPE_FROM_INSTANCE (viewable),
				    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gimp_dnd_viewable_source_set (GTK_WIDGET (list_item),
                                G_TYPE_FROM_INSTANCE (viewable),
				gimp_list_item_drag_viewable,
				NULL);
}

void
gimp_list_item_set_preview_size (GimpListItem *list_item,
				 gint          preview_size)
{
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));
  g_return_if_fail (preview_size > 0 && preview_size <= 256 /* FIXME: 64 */);

  list_item->preview_size = preview_size;

  GIMP_LIST_ITEM_GET_CLASS (list_item)->set_preview_size (list_item);
}

static void
gimp_list_item_real_set_preview_size (GimpListItem *list_item)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (list_item->preview);

  gimp_preview_set_size (preview,
			 list_item->preview_size, preview->border_width);
}

void
gimp_list_item_set_reorderable (GimpListItem  *list_item,
                                gboolean       reorderable,
                                GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));

  g_return_if_fail (! reorderable || container != NULL);
  g_return_if_fail (! container || GIMP_IS_CONTAINER (container));

  list_item->reorderable = reorderable ? TRUE : FALSE;;

  if (reorderable)
    {
      list_item->container = container;

      gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_item),
                                      GTK_DEST_DEFAULT_ALL,
                                      container->children_type,
                                      GDK_ACTION_MOVE | GDK_ACTION_COPY);
    }
  else
    {
      list_item->container = NULL;

      gtk_drag_dest_unset (GTK_WIDGET (list_item));
    }
}

void
gimp_list_item_set_convertable  (GimpListItem *list_item,
                                 gboolean      convertable)
{
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));

  list_item->convertable = convertable ? TRUE : FALSE;;
}

void
gimp_list_item_set_name_func (GimpListItem        *list_item,
			      GimpItemGetNameFunc  get_name_func)
{
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));

  if (list_item->get_name_func != get_name_func)
    {
      GimpViewable *viewable;

      list_item->get_name_func = get_name_func;

      viewable = GIMP_PREVIEW (list_item->preview)->viewable;

      if (viewable)
	gimp_list_item_name_changed (viewable, list_item);
    }
}

gboolean
gimp_list_item_check_drag (GimpListItem   *list_item,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           GimpViewable  **src_viewable,
                           gint           *dest_index,
                           GdkDragAction  *drag_action,
                           GimpDropType   *drop_type)
{
  GtkWidget     *src_widget;
  GimpViewable  *my_src_viewable  = NULL;
  GimpViewable  *my_dest_viewable = NULL;
  gint           my_src_index     = -1;
  gint           my_dest_index    = -1;
  GdkDragAction  my_drag_action   = GDK_ACTION_DEFAULT;
  GimpDropType   my_drop_type     = GIMP_DROP_NONE;
  gboolean       return_val       = FALSE;

  src_widget = gtk_drag_get_source_widget (context);

  if (list_item->reorderable &&
      list_item->container   &&
      src_widget             &&
      src_widget != GTK_WIDGET (list_item))
    {
      GimpListItem *src_list_item = NULL;

      if (GIMP_IS_LIST_ITEM (src_widget))
        src_list_item = GIMP_LIST_ITEM (src_widget);

      my_src_viewable  = gimp_dnd_get_drag_data (src_widget);
      my_dest_viewable = GIMP_PREVIEW (list_item->preview)->viewable;

      if (my_src_viewable          &&
          my_dest_viewable         &&
          list_item->container)
        {
          GimpContainer *src_container = NULL;

          if (src_list_item)
            src_container = src_list_item->container;

          if (! src_container)
            src_container = list_item->container;

          my_src_index =
            gimp_container_get_child_index (src_container,
                                            GIMP_OBJECT (my_src_viewable));
          my_dest_index =
            gimp_container_get_child_index (list_item->container,
                                            GIMP_OBJECT (my_dest_viewable));

          if (my_src_viewable  && my_src_index  != -1 &&
              my_dest_viewable && my_dest_index != -1)
            {
              if (src_container == list_item->container)
                {
                  gint difference;

                  difference = my_dest_index - my_src_index;

                  if (y < GTK_WIDGET (list_item)->allocation.height / 2)
                    my_drop_type = GIMP_DROP_ABOVE;
                  else
                    my_drop_type = GIMP_DROP_BELOW;

                  if (difference < 0 && my_drop_type == GIMP_DROP_BELOW)
                    {
                      my_dest_index++;
                    }
                  else if (difference > 0 && my_drop_type == GIMP_DROP_ABOVE)
                    {
                      my_dest_index--;
                    }

                  if (my_src_index != my_dest_index)
                    {
                      my_drag_action = GDK_ACTION_MOVE;
                      return_val  = TRUE;
                    }
                  else
                    {
                      my_drop_type = GIMP_DROP_NONE;
                    }
                }
              else if (src_list_item &&
                       src_list_item->convertable &&
                       src_container &&
                       src_container->children_type ==
                       list_item->container->children_type)
                {
                  if (y < GTK_WIDGET (list_item)->allocation.height / 2)
                    {
                      my_drop_type = GIMP_DROP_ABOVE;
                    }
                  else
                    {
                      my_drop_type = GIMP_DROP_BELOW;
                      my_dest_index++;
                    }

                  my_drag_action = GDK_ACTION_COPY;
                  return_val  = TRUE;
                }
            }
        }
    }

  *src_viewable = my_src_viewable;
  *dest_index   = my_dest_index;
  *drag_action  = my_drag_action;
  *drop_type    = my_drop_type;

  return return_val;
}

void
gimp_list_item_button_realize (GtkWidget *widget,
			       gpointer   data)
{
  gdk_window_set_back_pixmap (widget->window, NULL, TRUE);
}

void
gimp_list_item_button_state_changed (GtkWidget    *widget,
				     GtkStateType  previous_state,
				     gpointer      data)
{
  GtkWidget *list_item;

  list_item = GTK_WIDGET (data);

  if (widget->state != list_item->state)
    {
      switch (widget->state)
        {
        case GTK_STATE_NORMAL:
        case GTK_STATE_ACTIVE:
          /*  beware: recursion  */
          gtk_widget_set_state (widget, list_item->state);
          break;

        default:
          break;
        }
    }
  else if (list_item->state == GTK_STATE_INSENSITIVE)
    {
      /*  Don't look here, no, please...
       * 
       *  I said NO ...
       */
      if (GTK_WIDGET_DRAWABLE (list_item))
	{
	  GdkRectangle rect;

	  rect.x      = widget->allocation.x;
	  rect.y      = widget->allocation.y;
	  rect.width  = widget->allocation.width;
	  rect.height = widget->allocation.height;

	  gdk_window_invalidate_rect (list_item->window, &rect, FALSE);
	  gdk_window_process_updates (list_item->window, FALSE);
	}
    }
}

static void
gimp_list_item_name_changed (GimpViewable *viewable,
                             GimpListItem *list_item)
{
  if (list_item->get_name_func)
    {
      gchar *name;
      gchar *tooltip;

      name = list_item->get_name_func (GTK_WIDGET (list_item), &tooltip);

      gtk_label_set_text (GTK_LABEL (list_item->name_label), name);

      g_free (name);

      if (tooltip)
	{
	  gimp_help_set_help_data (GTK_WIDGET (list_item), tooltip, NULL);

	  g_free (tooltip);
	}
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (list_item->name_label),
			  gimp_object_get_name (GIMP_OBJECT (viewable)));
    }
}

static GimpViewable *
gimp_list_item_drag_viewable (GtkWidget *widget,
                              gpointer   data)
{
  return GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview)->viewable;
}
