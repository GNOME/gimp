/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistitem.c
 * Copyright (C) 2001 Michael Natterer
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

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "drawable.h"
#include "gdisplay.h"
#include "gimpcontainer.h"
#include "gimpdnd.h"
#include "gimpdrawablelistitem.h"
#include "gimpdrawablelistview.h"
#include "gimpimage.h"
#include "gimppreview.h"
#include "gimpviewable.h"

#include "pixmaps/eye.xpm"


static void   gimp_drawable_list_item_class_init (GimpDrawableListItemClass *klass);
static void   gimp_drawable_list_item_init       (GimpDrawableListItem      *list_item);

static void   gimp_drawable_list_item_set_viewable    (GimpListItem      *list_item,
                                                       GimpViewable      *viewable);

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

static void   gimp_drawable_list_item_eye_toggled     (GtkWidget         *widget,
                                                       gpointer           data);

static void   gimp_drawable_list_item_visibility_changed (GimpDrawable   *drawable,
                                                          gpointer        data);


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
  GtkObjectClass    *object_class;
  GtkWidgetClass    *widget_class;
  GimpListItemClass *list_item_class;

  object_class    = (GtkObjectClass *) klass;
  widget_class    = (GtkWidgetClass *) klass;
  list_item_class = (GimpListItemClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_LIST_ITEM);

  widget_class->drag_motion     = gimp_drawable_list_item_drag_motion;
  widget_class->drag_drop       = gimp_drawable_list_item_drag_drop;

  list_item_class->set_viewable = gimp_drawable_list_item_set_viewable;
}

static void
gimp_drawable_list_item_init (GimpDrawableListItem *list_item)
{
  GtkWidget *abox;
  GtkWidget *pixmap;

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GIMP_LIST_ITEM (list_item)->hbox), abox,
                      FALSE, FALSE, 0);
  gtk_widget_show (abox);

  list_item->eye_button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (list_item->eye_button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (abox), list_item->eye_button);
  gtk_widget_show (list_item->eye_button);

  gtk_signal_connect (GTK_OBJECT (list_item->eye_button), "realize",
                      GTK_SIGNAL_FUNC (gimp_drawable_list_item_button_realize),
                      list_item);

  gtk_signal_connect (GTK_OBJECT (list_item->eye_button), "state_changed",
                      GTK_SIGNAL_FUNC (gimp_drawable_list_item_button_state_changed),
                      list_item);

  pixmap = gimp_pixmap_new (eye_xpm);
  gtk_container_add (GTK_CONTAINER (list_item->eye_button), pixmap);
  gtk_widget_show (pixmap);
}

static void
gimp_drawable_list_item_set_viewable (GimpListItem *list_item,
                                      GimpViewable *viewable)
{
  GimpDrawableListItem *drawable_item;
  GimpDrawable         *drawable;
  gboolean              visible;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable (list_item, viewable);

  GIMP_PREVIEW (list_item->preview)->clickable = TRUE;

  drawable_item = GIMP_DRAWABLE_LIST_ITEM (list_item);
  drawable      = GIMP_DRAWABLE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible       = gimp_drawable_get_visible (drawable);

  if (! visible)
    {
      GtkRequisition requisition;

      gtk_widget_size_request (drawable_item->eye_button, &requisition);

      gtk_widget_set_usize (drawable_item->eye_button,
                            requisition.width,
                            requisition.height);
      gtk_widget_hide (GTK_BIN (drawable_item->eye_button)->child);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (drawable_item->eye_button),
                                visible);

  gtk_signal_connect (GTK_OBJECT (drawable_item->eye_button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_drawable_list_item_eye_toggled),
                      list_item);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (viewable), "visibility_changed",
     GTK_SIGNAL_FUNC (gimp_drawable_list_item_visibility_changed),
     list_item,
     GTK_OBJECT (list_item));
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
      if (widget->parent && GIMP_IS_DRAWABLE_LIST_VIEW (widget->parent))
        {
          GimpDrawableListView *list_view;

          list_view = GIMP_DRAWABLE_LIST_VIEW (widget->parent);

          list_view->reorder_drawable_func (list_view->gimage,
                                            GIMP_DRAWABLE (src_viewable),
                                            dest_index,
                                            TRUE);

          gdisplays_flush ();
        }
      else
        {
          g_warning ("%s(): GimpDrawableListItem is not "
                     "part of a GimpDrawableListView", G_GNUC_FUNCTION);
        }
    }

  return return_val;
}

static void
gimp_drawable_list_item_eye_toggled (GtkWidget *widget,
                                     gpointer   data)
{
  GimpListItem *list_item;
  GimpDrawable *drawable;
  gboolean      visible;

  list_item = GIMP_LIST_ITEM (data);
  drawable  = GIMP_DRAWABLE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (visible != gimp_drawable_get_visible (drawable))
    {
      if (! visible)
        {
          gtk_widget_set_usize (GTK_WIDGET (widget),
                                GTK_WIDGET (widget)->allocation.width,
                                GTK_WIDGET (widget)->allocation.height);
          gtk_widget_hide (GTK_BIN (widget)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (widget)->child);
          gtk_widget_set_usize (GTK_WIDGET (widget), -1, -1);
        }

      gtk_signal_handler_block_by_func (GTK_OBJECT (drawable),
                                        gimp_drawable_list_item_visibility_changed,
                                        list_item);

      gimp_drawable_set_visible (drawable, visible);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (drawable),
                                          gimp_drawable_list_item_visibility_changed,
                                          list_item);

      drawable_update (drawable, 0, 0,
                       drawable->width,
                       drawable->height);

      gdisplays_flush ();
    }
}

static void
gimp_drawable_list_item_visibility_changed (GimpDrawable *drawable,
                                            gpointer      data)
{
  GimpListItem    *list_item;
  GtkToggleButton *toggle;
  gboolean         visible;

  list_item = GIMP_LIST_ITEM (data);
  toggle    = GTK_TOGGLE_BUTTON (GIMP_DRAWABLE_LIST_ITEM (data)->eye_button);
  visible   = gimp_drawable_get_visible (drawable);

  if (visible != toggle->active)
    {
      if (! visible)
        {
          gtk_widget_set_usize (GTK_WIDGET (toggle),
                                GTK_WIDGET (toggle)->allocation.width,
                                GTK_WIDGET (toggle)->allocation.height);
          gtk_widget_hide (GTK_BIN (toggle)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (toggle)->child);
          gtk_widget_set_usize (GTK_WIDGET (toggle), -1, -1);
        }

      gtk_signal_handler_block_by_func (GTK_OBJECT (toggle),
                                        gimp_drawable_list_item_eye_toggled,
                                        list_item);

      gtk_toggle_button_set_active (toggle, visible);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (toggle),
                                          gimp_drawable_list_item_eye_toggled,
                                          list_item);
    }
}


/*  protected finctions  */

void
gimp_drawable_list_item_button_realize (GtkWidget *widget,
                                        gpointer   data)
{
  gdk_window_set_back_pixmap (widget->window, NULL, TRUE);
}

void
gimp_drawable_list_item_button_state_changed (GtkWidget    *widget,
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
}
