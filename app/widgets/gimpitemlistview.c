/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemlistview.c
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplay-foreach.h"

#include "gimpchannellistview.h"
#include "gimpdnd.h"
#include "gimpitemlistview.h"
#include "gimpitemfactory.h"
#include "gimplayerlistview.h"
#include "gimplistitem.h"
#include "gimppreview.h"

#include "libgimp/gimpintl.h"


enum
{
  SET_IMAGE,
  LAST_SIGNAL
};


static void   gimp_item_list_view_class_init (GimpItemListViewClass *klass);
static void   gimp_item_list_view_init       (GimpItemListView      *view);
static void   gimp_item_list_view_destroy    (GtkObject             *object);

static void   gimp_item_list_view_real_set_image    (GimpItemListView  *view,
                                                     GimpImage         *gimage);

static void   gimp_item_list_view_select_item       (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);
static void   gimp_item_list_view_activate_item     (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);
static void   gimp_item_list_view_context_item      (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);

static void   gimp_item_list_view_new_clicked       (GtkWidget         *widget,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_new_dropped       (GtkWidget         *widget,
                                                     GimpViewable      *viewable,
                                                     gpointer           data);

static void   gimp_item_list_view_raise_clicked     (GtkWidget         *widget,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_raise_extended_clicked
                                                    (GtkWidget         *widget,
                                                     guint              state,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_lower_clicked     (GtkWidget         *widget,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_lower_extended_clicked
                                                    (GtkWidget         *widget,
                                                     guint              state,
                                                     GimpItemListView  *view);

static void   gimp_item_list_view_duplicate_clicked (GtkWidget         *widget,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_edit_clicked      (GtkWidget         *widget,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_delete_clicked    (GtkWidget         *widget,
                                                     GimpItemListView  *view);

static void   gimp_item_list_view_item_changed      (GimpImage         *gimage,
                                                     GimpItemListView  *view);
static void   gimp_item_list_view_size_changed      (GimpImage         *gimage,
                                                     GimpItemListView  *view);


static guint  view_signals[LAST_SIGNAL] = { 0 };

static GimpContainerListViewClass *parent_class = NULL;


GType
gimp_item_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpItemListViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_item_list_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpItemListView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_item_list_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_LIST_VIEW,
                                          "GimpItemListView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_item_list_view_class_init (GimpItemListViewClass *klass)
{
  GtkObjectClass         *object_class;
  GimpContainerViewClass *container_view_class;

  object_class         = GTK_OBJECT_CLASS (klass);
  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  view_signals[SET_IMAGE] =
    g_signal_new ("set_image",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpItemListViewClass, set_image),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  object_class->destroy               = gimp_item_list_view_destroy;

  container_view_class->select_item   = gimp_item_list_view_select_item;
  container_view_class->activate_item = gimp_item_list_view_activate_item;
  container_view_class->context_item  = gimp_item_list_view_context_item;

  klass->set_image                    = gimp_item_list_view_real_set_image;
}

static void
gimp_item_list_view_init (GimpItemListView *view)
{
  GimpContainerView *container_view;

  container_view = GIMP_CONTAINER_VIEW (view);

  view->gimage        = NULL;
  view->item_type = G_TYPE_NONE;
  view->signal_name   = NULL;

  view->new_button =
    gimp_container_view_add_button (container_view,
				    GTK_STOCK_NEW,
				    _("New"), NULL,
				    G_CALLBACK (gimp_item_list_view_new_clicked),
				    NULL,
				    view);

  view->raise_button =
    gimp_container_view_add_button (container_view,
				    GTK_STOCK_GO_UP,
				    _("Raise\n"
				      "<Shift> To Top"), NULL,
				    G_CALLBACK (gimp_item_list_view_raise_clicked),
				    G_CALLBACK (gimp_item_list_view_raise_extended_clicked),
				    view);

  view->lower_button =
    gimp_container_view_add_button (container_view,
				    GTK_STOCK_GO_DOWN,
				    _("Lower\n"
				      "<Shift> To Bottom"), NULL,
				    G_CALLBACK (gimp_item_list_view_lower_clicked),
				    G_CALLBACK (gimp_item_list_view_lower_extended_clicked),
				    view);

  view->duplicate_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_DUPLICATE,
				    _("Duplicate"), NULL,
				    G_CALLBACK (gimp_item_list_view_duplicate_clicked),
				    NULL,
				    view);

  view->edit_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_EDIT,
				    _("Edit"), NULL,
				    G_CALLBACK (gimp_item_list_view_edit_clicked),
				    NULL,
				    view);

  view->delete_button =
    gimp_container_view_add_button (container_view,
				    GTK_STOCK_DELETE,
				    _("Delete"), NULL,
				    G_CALLBACK (gimp_item_list_view_delete_clicked),
				    NULL,
				    view);

  gtk_widget_set_sensitive (view->new_button,       FALSE);
  gtk_widget_set_sensitive (view->raise_button,     FALSE);
  gtk_widget_set_sensitive (view->lower_button,     FALSE);
  gtk_widget_set_sensitive (view->duplicate_button, FALSE);
  gtk_widget_set_sensitive (view->edit_button,      FALSE);
  gtk_widget_set_sensitive (view->delete_button,    FALSE);
}

static void
gimp_item_list_view_destroy (GtkObject *object)
{
  GimpItemListView *view;

  view = GIMP_ITEM_LIST_VIEW (object);

  if (view->gimage)
    gimp_item_list_view_set_image (view, NULL);

  if (view->signal_name)
    {
      g_free (view->signal_name);
      view->signal_name = NULL;
    }

  if (view->item_factory)
    {
      g_object_unref (G_OBJECT (view->item_factory));
      view->item_factory = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_item_list_view_new (gint                  preview_size,
                         GimpImage            *gimage,
                         GType                 item_type,
                         const gchar          *signal_name,
                         GimpGetContainerFunc  get_container_func,
                         GimpGetItemFunc       get_item_func,
                         GimpSetItemFunc       set_item_func,
                         GimpReorderItemFunc   reorder_item_func,
                         GimpAddItemFunc       add_item_func,
                         GimpRemoveItemFunc    remove_item_func,
                         GimpCopyItemFunc      copy_item_func,
                         GimpConvertItemFunc   convert_item_func,
                         GimpNewItemFunc       new_item_func,
                         GimpEditItemFunc      edit_item_func,
                         GimpItemFactory      *item_factory)
{
  GimpItemListView  *list_view;
  GimpContainerView *view;

  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);
  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (signal_name != NULL, NULL);
  g_return_val_if_fail (get_container_func != NULL, NULL);
  g_return_val_if_fail (get_item_func != NULL, NULL);
  g_return_val_if_fail (get_item_func != NULL, NULL);
  g_return_val_if_fail (reorder_item_func != NULL, NULL);
  g_return_val_if_fail (add_item_func != NULL, NULL);
  g_return_val_if_fail (remove_item_func != NULL, NULL);
  g_return_val_if_fail (copy_item_func != NULL, NULL);
  /*  convert_item_func may be NULL  */
  g_return_val_if_fail (new_item_func != NULL, NULL);
  g_return_val_if_fail (edit_item_func != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_ITEM_FACTORY (item_factory), NULL);

  if (item_type == GIMP_TYPE_LAYER)
    {
      list_view = g_object_new (GIMP_TYPE_LAYER_LIST_VIEW, NULL);
    }
  else if (item_type == GIMP_TYPE_CHANNEL)
    {
      list_view = g_object_new (GIMP_TYPE_CHANNEL_LIST_VIEW, NULL);
    }
  else
    {
      list_view = g_object_new (GIMP_TYPE_ITEM_LIST_VIEW, NULL);
    }

  view = GIMP_CONTAINER_VIEW (list_view);

  view->preview_size = preview_size;
  view->reorderable  = TRUE;

  list_view->item_type          = item_type;
  list_view->signal_name        = g_strdup (signal_name);
  list_view->get_container_func = get_container_func;
  list_view->get_item_func      = get_item_func;
  list_view->set_item_func      = set_item_func;
  list_view->reorder_item_func  = reorder_item_func;
  list_view->add_item_func      = add_item_func;
  list_view->remove_item_func   = remove_item_func;
  list_view->copy_item_func     = copy_item_func;
  list_view->convert_item_func  = convert_item_func;
  list_view->new_item_func      = new_item_func;
  list_view->edit_item_func     = edit_item_func;

  list_view->item_factory = item_factory;
  g_object_ref (G_OBJECT (list_view->item_factory));

  /*  connect "drop to new" manually as it makes a difference whether
   *  it was clicked or dropped
   */
  gimp_gtk_drag_dest_set_by_type (list_view->new_button,
				  GTK_DEST_DEFAULT_ALL,
				  item_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (list_view->new_button,
			      item_type,
			      gimp_item_list_view_new_dropped,
			      view);

  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->duplicate_button),
				  item_type);
  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->edit_button),
				  item_type);
  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->delete_button),
				  item_type);

  gimp_item_list_view_set_image (list_view, gimage);

  return GTK_WIDGET (list_view);
}

void
gimp_item_list_view_set_image (GimpItemListView *view,
                               GimpImage        *gimage)
{
  g_return_if_fail (GIMP_IS_ITEM_LIST_VIEW (view));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (view), view_signals[SET_IMAGE], 0,
		 gimage);
}

static void
gimp_item_list_view_real_set_image (GimpItemListView *view,
                                    GimpImage        *gimage)
{
  g_return_if_fail (GIMP_IS_ITEM_LIST_VIEW (view));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (view->gimage == gimage)
    return;

  if (view->gimage)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_item_list_view_item_changed,
					    view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_item_list_view_size_changed,
					    view);

      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view), NULL);
    }

  view->gimage = gimage;

  if (view->gimage)
    {
      GimpContainer *container;

      container = view->get_container_func (view->gimage);

      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view), container);

      g_signal_connect (G_OBJECT (view->gimage), view->signal_name,
			G_CALLBACK (gimp_item_list_view_item_changed),
			view);
      g_signal_connect (G_OBJECT (view->gimage), "size_changed",
			G_CALLBACK (gimp_item_list_view_size_changed),
			view);

      gimp_item_list_view_item_changed (view->gimage, view);
    }

  gtk_widget_set_sensitive (view->new_button, (view->gimage != NULL));
}


/*  GimpContainerView methods  */

static void
gimp_item_list_view_select_item (GimpContainerView *view,
                                 GimpViewable      *item,
                                 gpointer           insert_data)
{
  GimpItemListView *list_view;
  gboolean          raise_sensitive     = FALSE;
  gboolean          lower_sensitive     = FALSE;
  gboolean          duplicate_sensitive = FALSE;
  gboolean          edit_sensitive      = FALSE;
  gboolean          delete_sensitive    = FALSE;

  list_view = GIMP_ITEM_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  if (item)
    {
      GimpViewable *active_viewable;
      gint          index;

      active_viewable = list_view->get_item_func (list_view->gimage);

      if (active_viewable != item)
	{
	  list_view->set_item_func (list_view->gimage, item);

	  gdisplays_flush ();
	}

      index = gimp_container_get_child_index (view->container,
					      GIMP_OBJECT (item));

      if (view->container->num_children > 1)
	{
	  if (index > 0)
	    raise_sensitive = TRUE;

	  if (index < (view->container->num_children - 1))
	    lower_sensitive = TRUE;
	}

      duplicate_sensitive = TRUE;
      edit_sensitive      = TRUE;
      delete_sensitive    = TRUE;
    }

  gtk_widget_set_sensitive (list_view->raise_button,     raise_sensitive);
  gtk_widget_set_sensitive (list_view->lower_button,     lower_sensitive);
  gtk_widget_set_sensitive (list_view->duplicate_button, duplicate_sensitive);
  gtk_widget_set_sensitive (list_view->edit_button,      edit_sensitive);
  gtk_widget_set_sensitive (list_view->delete_button,    delete_sensitive);
}

static void
gimp_item_list_view_activate_item (GimpContainerView *view,
                                   GimpViewable      *item,
                                   gpointer           insert_data)
{
  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->activate_item (view,
							     item,
							     insert_data);

  gtk_button_clicked (GTK_BUTTON (GIMP_ITEM_LIST_VIEW (view)->edit_button));
}

static void
gimp_item_list_view_context_item (GimpContainerView *view,
                                  GimpViewable      *item,
                                  gpointer           insert_data)
{
  GimpItemListView *item_view;

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->context_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->context_item (view,
							    item,
							    insert_data);

  item_view = GIMP_ITEM_LIST_VIEW (view);

  if (item_view->item_factory)
    {
      GimpImage *gimage;

      gimage = gimp_item_get_image (GIMP_ITEM (item));

      gimp_item_factory_popup_with_data (item_view->item_factory,
                                         gimage,
                                         NULL);
    }
}


/*  "New" functions  */

static void
gimp_item_list_view_new_clicked (GtkWidget        *widget,
                                 GimpItemListView *view)
{
  view->new_item_func (view->gimage, NULL);
}

static void
gimp_item_list_view_new_dropped (GtkWidget    *widget,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GimpItemListView *view;

  view = (GimpItemListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      view->new_item_func (view->gimage, viewable);
    }
}


/*  "Duplicate" functions  */

static void
gimp_item_list_view_duplicate_clicked (GtkWidget        *widget,
                                       GimpItemListView *view)
{
  GimpViewable *viewable;
  GimpViewable *new_viewable;

  viewable = view->get_item_func (view->gimage);

  new_viewable = view->copy_item_func (viewable,
                                       G_TYPE_FROM_INSTANCE (viewable),
                                       TRUE);

  if (GIMP_IS_VIEWABLE (new_viewable))
    {
      view->add_item_func (view->gimage, new_viewable, -1);

      gdisplays_flush ();
    }
}


/*  "Raise/Lower" functions  */

static void
gimp_item_list_view_raise_clicked (GtkWidget        *widget,
                                   GimpItemListView *view)
{
  GimpContainer *container;
  GimpViewable  *viewable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  viewable  = view->get_item_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (viewable));

  if (index > 0)
    {
      view->reorder_item_func (view->gimage, viewable, index - 1, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_item_list_view_raise_extended_clicked (GtkWidget        *widget,
                                            guint             state,
                                            GimpItemListView *view)
{
  GimpContainer *container;
  GimpViewable  *viewable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  viewable  = view->get_item_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (viewable));

  if ((state & GDK_SHIFT_MASK) && (index > 0))
    {
      view->reorder_item_func (view->gimage, viewable, 0, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_item_list_view_lower_clicked (GtkWidget        *widget,
                                   GimpItemListView *view)
{
  GimpContainer *container;
  GimpViewable  *viewable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  viewable  = view->get_item_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (viewable));

  if (index < container->num_children - 1)
    {
      view->reorder_item_func (view->gimage, viewable, index + 1, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_item_list_view_lower_extended_clicked (GtkWidget        *widget,
                                            guint             state,
                                            GimpItemListView *view)
{
  GimpContainer *container;
  GimpViewable  *viewable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  viewable  = view->get_item_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (viewable));

  if ((state & GDK_SHIFT_MASK) && (index < container->num_children - 1))
    {
      view->reorder_item_func (view->gimage, viewable,
                               container->num_children - 1, TRUE);

      gdisplays_flush ();
    }
}


/*  "Edit" functions  */

static void
gimp_item_list_view_edit_clicked (GtkWidget        *widget,
                                  GimpItemListView *view)
{
  GimpViewable *viewable;

  viewable = view->get_item_func (view->gimage);

  view->edit_item_func (viewable);
}


/*  "Delete" functions  */

static void
gimp_item_list_view_delete_clicked (GtkWidget        *widget,
                                    GimpItemListView *view)
{
  GimpViewable *viewable;

  viewable = view->get_item_func (view->gimage);

  view->remove_item_func (view->gimage, viewable);

  gdisplays_flush ();
}


/*  GimpImage callbacks  */

static void
gimp_item_list_view_item_changed (GimpImage        *gimage,
                                  GimpItemListView *view)
{
  GimpViewable *viewable;

  viewable = view->get_item_func (gimage);

  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (view), viewable);
}

static void
gimp_item_list_view_size_changed (GimpImage        *gimage,
                                  GimpItemListView *view)
{
  gint preview_size;

  preview_size = GIMP_CONTAINER_VIEW (view)->preview_size;

  gimp_container_view_set_preview_size (GIMP_CONTAINER_VIEW (view),
					preview_size);
}
