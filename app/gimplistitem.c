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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpcontainer.h"
#include "gimpdnd.h"
#include "gimpdrawable.h"
#include "gimpdrawablelistitem.h"
#include "gimplayer.h"
#include "gimplayerlistitem.h"
#include "gimplistitem.h"
#include "gimpmarshal.h"
#include "gimppreview.h"
#include "gimpviewable.h"


enum
{
  SET_VIEWABLE,
  LAST_SIGNAL
};


static void           gimp_list_item_class_init    (GimpListItemClass *klass);
static void           gimp_list_item_init          (GimpListItem      *list_item);

static void           gimp_list_item_set_viewable  (GimpListItem      *list_item,
                                                    GimpViewable      *viewable);

static void           gimp_list_item_real_set_viewable (GimpListItem  *list_item,
                                                        GimpViewable  *viewable);

static void           gimp_list_item_draw          (GtkWidget         *widget,
                                                    GdkRectangle      *area);
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
                                                    GtkLabel          *label);
static GimpViewable * gimp_list_item_drag_viewable (GtkWidget         *widget,
                                                    gpointer           data);


static guint list_item_signals[LAST_SIGNAL] = { 0 };

static GtkListItemClass *parent_class       = NULL;


GtkType
gimp_list_item_get_type (void)
{
  static GtkType list_item_type = 0;

  if (!list_item_type)
    {
      static const GtkTypeInfo list_item_info =
      {
	"GimpListItem",
	sizeof (GimpListItem),
	sizeof (GimpListItemClass),
	(GtkClassInitFunc) gimp_list_item_class_init,
	(GtkObjectInitFunc) gimp_list_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      list_item_type = gtk_type_unique (GTK_TYPE_LIST_ITEM, &list_item_info);
    }

  return list_item_type;
}

static void
gimp_list_item_class_init (GimpListItemClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_LIST_ITEM);

  list_item_signals[SET_VIEWABLE] = 
    gtk_signal_new ("set_viewable",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpListItemClass,
                                       set_viewable),
                    gtk_marshal_NONE__OBJECT,
                    GTK_TYPE_NONE, 1,
                    GIMP_TYPE_VIEWABLE);

  widget_class->draw        = gimp_list_item_draw;
  widget_class->drag_leave  = gimp_list_item_drag_leave;
  widget_class->drag_motion = gimp_list_item_drag_motion;
  widget_class->drag_drop   = gimp_list_item_drag_drop;

  klass->set_viewable       = gimp_list_item_real_set_viewable;
}

static void
gimp_list_item_init (GimpListItem *list_item)
{
  list_item->hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (list_item->hbox), 2);
  gtk_container_add (GTK_CONTAINER (list_item), list_item->hbox);
  gtk_widget_show (list_item->hbox);

  list_item->preview      = NULL;
  list_item->name_label   = NULL;

  list_item->preview_size = 0;

  list_item->reorderable  = FALSE;
  list_item->drop_type    = GIMP_DROP_NONE;
  list_item->container    = NULL;
}

static void
gimp_list_item_draw (GtkWidget    *widget,
                     GdkRectangle *area)
{
  GimpListItem *list_item;

  list_item = GIMP_LIST_ITEM (widget);

  if (GTK_WIDGET_CLASS (parent_class)->draw)
    GTK_WIDGET_CLASS (parent_class)->draw (widget, area);

  if (list_item->drop_type != GIMP_DROP_NONE)
    {
      gint x, y;

      x = list_item->name_label->allocation.x;

      y = ((list_item->drop_type == GIMP_DROP_ABOVE) ?
	   3 : widget->allocation.height - 4);

      gdk_gc_set_line_attributes (widget->style->black_gc, 5, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_MITER);

      gdk_draw_line (widget->window, widget->style->black_gc,
		     x, y,
                     widget->allocation.width - 3, y);

      gdk_gc_set_line_attributes (widget->style->black_gc, 0, GDK_LINE_SOLID,
				  GDK_CAP_BUTT, GDK_JOIN_MITER);
    }
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

  list_item->drop_type = drop_type;

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

  g_return_val_if_fail (viewable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 256, NULL);

  if (GIMP_IS_LAYER (viewable))
    {
      list_item = gtk_type_new (GIMP_TYPE_LAYER_LIST_ITEM);
    }
  else if (GIMP_IS_DRAWABLE (viewable))
    {
      list_item = gtk_type_new (GIMP_TYPE_DRAWABLE_LIST_ITEM);
    }
  else
    {
      list_item = gtk_type_new (GIMP_TYPE_LIST_ITEM);
    }

  list_item->preview_size = preview_size;

  gimp_list_item_set_viewable (list_item, viewable);

  return GTK_WIDGET (list_item);
}

static void
gimp_list_item_set_viewable (GimpListItem *list_item,
                             GimpViewable *viewable)
{
  gtk_signal_emit (GTK_OBJECT (list_item), list_item_signals[SET_VIEWABLE],
                   viewable);
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

  list_item->name_label =
    gtk_label_new (gimp_object_get_name (GIMP_OBJECT (viewable)));
  gtk_box_pack_start (GTK_BOX (list_item->hbox), list_item->name_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (list_item->name_label);

  gtk_signal_connect_while_alive (GTK_OBJECT (viewable), "name_changed",
                                  GTK_SIGNAL_FUNC (gimp_list_item_name_changed),
                                  list_item->name_label,
                                  GTK_OBJECT (list_item->name_label));

  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (list_item),
				    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				    GTK_OBJECT (viewable)->klass->type,
				    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gimp_dnd_viewable_source_set (GTK_WIDGET (list_item),
                                GTK_OBJECT (viewable)->klass->type,
				gimp_list_item_drag_viewable,
				NULL);

  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_item),
                                  GTK_DEST_DEFAULT_ALL,
                                  GTK_OBJECT (viewable)->klass->type,
                                  GDK_ACTION_MOVE | GDK_ACTION_COPY);
}

void
gimp_list_item_set_reorderable (GimpListItem  *list_item,
                                gboolean       reorderable,
                                GimpContainer *container)
{
  g_return_if_fail (list_item != NULL);
  g_return_if_fail (GIMP_IS_LIST_ITEM (list_item));

  g_return_if_fail (! reorderable || container != NULL);
  g_return_if_fail (! container || GIMP_IS_CONTAINER (container));

  list_item->reorderable = reorderable;

  if (reorderable)
    list_item->container = container;
  else
    list_item->container = NULL;
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

  if (list_item->reorderable                              &&
      list_item->container                                &&
      (src_widget = gtk_drag_get_source_widget (context)) &&
      src_widget != GTK_WIDGET (list_item))
    {
      my_src_viewable  = gimp_dnd_get_drag_data (src_widget);
      my_dest_viewable = GIMP_PREVIEW (list_item->preview)->viewable;

      if (my_src_viewable && my_dest_viewable)
        {
          my_src_index =
            gimp_container_get_child_index (list_item->container,
                                            GIMP_OBJECT (my_src_viewable));
          my_dest_index =
            gimp_container_get_child_index (list_item->container,
                                            GIMP_OBJECT (my_dest_viewable));

          if (my_src_viewable  && my_src_index  != -1 &&
              my_dest_viewable && my_dest_index != -1)
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
        }
    }

  *src_viewable = my_src_viewable;
  *dest_index   = my_dest_index;
  *drag_action  = my_drag_action;
  *drop_type    = my_drop_type;

  return return_val;
}

static void
gimp_list_item_name_changed (GimpViewable *viewable,
                             GtkLabel     *label)
{
  gtk_label_set_text (label, gimp_object_get_name (GIMP_OBJECT (viewable)));
}

static GimpViewable *
gimp_list_item_drag_viewable (GtkWidget *widget,
                              gpointer   data)
{
  return GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview)->viewable;
}
