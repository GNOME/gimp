/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistview.c
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpmarshal.h"

#include "gimpchannellistview.h"
#include "gimpdnd.h"
#include "gimpdrawablelistview.h"
#include "gimplayerlistview.h"
#include "gimplistitem.h"
#include "gimppreview.h"

#include "gdisplay.h"

#include "libgimp/gimpintl.h"


enum
{
  SET_IMAGE,
  LAST_SIGNAL
};


static void   gimp_drawable_list_view_class_init (GimpDrawableListViewClass *klass);
static void   gimp_drawable_list_view_init       (GimpDrawableListView      *view);
static void   gimp_drawable_list_view_destroy    (GtkObject                 *object);

static void   gimp_drawable_list_view_real_set_image    (GimpDrawableListView *view,
							 GimpImage            *gimage);

static gpointer gimp_drawable_list_view_insert_item     (GimpContainerView    *view,
							 GimpViewable         *viewable,
							 gint                  index);
static void   gimp_drawable_list_view_select_item       (GimpContainerView    *view,
							 GimpViewable         *item,
							 gpointer              insert_data);
static void   gimp_drawable_list_view_activate_item     (GimpContainerView    *view,
							 GimpViewable         *item,
							 gpointer              insert_data);
static void   gimp_drawable_list_view_context_item      (GimpContainerView    *view,
							 GimpViewable         *item,
							 gpointer              insert_data);

static void   gimp_drawable_list_view_new_drawable      (GimpDrawableListView *view,
							 GimpDrawable         *drawable);
static void   gimp_drawable_list_view_new_clicked       (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_new_dropped       (GtkWidget            *widget,
							 GimpViewable         *viewable,
							 gpointer              data);

static void   gimp_drawable_list_view_raise_clicked     (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_raise_extended_clicked
                                                        (GtkWidget            *widget,
							 guint                 state,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_lower_clicked     (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_lower_extended_clicked
                                                        (GtkWidget            *widget,
							 guint                 state,
							 GimpDrawableListView *view);

static void   gimp_drawable_list_view_duplicate_clicked (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_edit_clicked      (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_delete_clicked    (GtkWidget            *widget,
							 GimpDrawableListView *view);

static void   gimp_drawable_list_view_drawable_changed  (GimpImage            *gimage,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_size_changed      (GimpImage            *gimage,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_floating_selection_changed
                                                        (GimpImage            *gimage,
							 GimpDrawableListView *view);


static guint  view_signals[LAST_SIGNAL] = { 0 };

static GimpContainerListViewClass *parent_class = NULL;


GType
gimp_drawable_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpDrawableListView",
	sizeof (GimpDrawableListView),
	sizeof (GimpDrawableListViewClass),
	(GtkClassInitFunc) gimp_drawable_list_view_class_init,
	(GtkObjectInitFunc) gimp_drawable_list_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GIMP_TYPE_CONTAINER_LIST_VIEW, &view_info);
    }

  return view_type;
}

static void
gimp_drawable_list_view_class_init (GimpDrawableListViewClass *klass)
{
  GtkObjectClass         *object_class;
  GimpContainerViewClass *container_view_class;

  object_class         = (GtkObjectClass *) klass;
  container_view_class = (GimpContainerViewClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  view_signals[SET_IMAGE] =
    g_signal_new ("set_image",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpDrawableListViewClass, set_image),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  object_class->destroy               = gimp_drawable_list_view_destroy;

  container_view_class->insert_item   = gimp_drawable_list_view_insert_item;
  container_view_class->select_item   = gimp_drawable_list_view_select_item;
  container_view_class->activate_item = gimp_drawable_list_view_activate_item;
  container_view_class->context_item  = gimp_drawable_list_view_context_item;

  klass->set_image                    = gimp_drawable_list_view_real_set_image;
}

static void
gimp_drawable_list_view_init (GimpDrawableListView *view)
{
  GimpContainerView *container_view;

  container_view = GIMP_CONTAINER_VIEW (view);

  view->gimage        = NULL;
  view->drawable_type = GTK_TYPE_NONE;
  view->signal_name   = NULL;

  view->new_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_NEW,
				    _("New"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_new_clicked),
				    NULL,
				    view);

  view->raise_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_RAISE,
				    _("Raise\n"
				      "<Shift> To Top"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_raise_clicked),
				    G_CALLBACK (gimp_drawable_list_view_raise_extended_clicked),
				    view);

  view->lower_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_LOWER,
				    _("Lower\n"
				      "<Shift> To Bottom"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_lower_clicked),
				    G_CALLBACK (gimp_drawable_list_view_lower_extended_clicked),
				    view);

  view->duplicate_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_DUPLICATE,
				    _("Duplicate"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_duplicate_clicked),
				    NULL,
				    view);

  view->edit_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_EDIT,
				    _("Edit"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_edit_clicked),
				    NULL,
				    view);

  view->delete_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_DELETE,
				    _("Delete"), NULL,
				    G_CALLBACK (gimp_drawable_list_view_delete_clicked),
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
gimp_drawable_list_view_destroy (GtkObject *object)
{
  GimpDrawableListView *view;

  view = GIMP_DRAWABLE_LIST_VIEW (object);

  if (view->gimage)
    gimp_drawable_list_view_set_image (view, NULL);

  if (view->signal_name)
    {
      g_free (view->signal_name);
      view->signal_name = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_drawable_list_view_new (gint                     preview_size,
			     GimpImage               *gimage,
			     GtkType                  drawable_type,
			     const gchar             *signal_name,
			     GimpGetContainerFunc     get_container_func,
			     GimpGetDrawableFunc      get_drawable_func,
			     GimpSetDrawableFunc      set_drawable_func,
			     GimpReorderDrawableFunc  reorder_drawable_func,
			     GimpAddDrawableFunc      add_drawable_func,
			     GimpRemoveDrawableFunc   remove_drawable_func,
			     GimpCopyDrawableFunc     copy_drawable_func,
			     GimpNewDrawableFunc      new_drawable_func,
			     GimpEditDrawableFunc     edit_drawable_func,
			     GimpDrawableContextFunc  drawable_context_func)
{
  GimpDrawableListView *list_view;
  GimpContainerView    *view;

  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);
  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (signal_name != NULL, NULL);
  g_return_val_if_fail (get_container_func != NULL, NULL);
  g_return_val_if_fail (get_drawable_func != NULL, NULL);
  g_return_val_if_fail (get_drawable_func != NULL, NULL);
  g_return_val_if_fail (reorder_drawable_func != NULL, NULL);
  g_return_val_if_fail (add_drawable_func != NULL, NULL);
  g_return_val_if_fail (remove_drawable_func != NULL, NULL);
  g_return_val_if_fail (copy_drawable_func != NULL, NULL);
  g_return_val_if_fail (new_drawable_func != NULL, NULL);
  g_return_val_if_fail (edit_drawable_func != NULL, NULL);
  g_return_val_if_fail (drawable_context_func != NULL, NULL);

  if (drawable_type == GIMP_TYPE_LAYER)
    {
      list_view = g_object_new (GIMP_TYPE_LAYER_LIST_VIEW, NULL);
    }
  else if (drawable_type == GIMP_TYPE_CHANNEL)
    {
      list_view = g_object_new (GIMP_TYPE_CHANNEL_LIST_VIEW, NULL);
    }
  else
    {
      list_view = g_object_new (GIMP_TYPE_DRAWABLE_LIST_VIEW, NULL);
    }

  view = GIMP_CONTAINER_VIEW (list_view);

  view->preview_size = preview_size;

  list_view->drawable_type         = drawable_type;
  list_view->signal_name           = g_strdup (signal_name);
  list_view->get_container_func    = get_container_func;
  list_view->get_drawable_func     = get_drawable_func;
  list_view->set_drawable_func     = set_drawable_func;
  list_view->reorder_drawable_func = reorder_drawable_func;
  list_view->add_drawable_func     = add_drawable_func;
  list_view->remove_drawable_func  = remove_drawable_func;
  list_view->copy_drawable_func    = copy_drawable_func;
  list_view->new_drawable_func     = new_drawable_func;
  list_view->edit_drawable_func    = edit_drawable_func;
  list_view->drawable_context_func = drawable_context_func;

  /*  connect "drop to new" manually as it makes a difference whether
   *  it was clicked or dropped
   */
  gimp_gtk_drag_dest_set_by_type (list_view->new_button,
				  GTK_DEST_DEFAULT_ALL,
				  drawable_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (list_view->new_button,
			      drawable_type,
			      gimp_drawable_list_view_new_dropped,
			      view);

  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->duplicate_button),
				  drawable_type);
  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->edit_button),
				  drawable_type);
  gimp_container_view_enable_dnd (view,
				  GTK_BUTTON (list_view->delete_button),
				  drawable_type);

  gimp_drawable_list_view_set_image (list_view, gimage);

  return GTK_WIDGET (list_view);
}

void
gimp_drawable_list_view_set_image (GimpDrawableListView *view,
				   GimpImage            *gimage)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_LIST_VIEW (view));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (view), view_signals[SET_IMAGE], 0,
		 gimage);
}

static void
gimp_drawable_list_view_real_set_image (GimpDrawableListView *view,
					GimpImage            *gimage)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_LIST_VIEW (view));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (view->gimage == gimage)
    return;

  if (view->gimage)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_drawable_list_view_drawable_changed,
					    view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_drawable_list_view_size_changed,
					    view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_drawable_list_view_floating_selection_changed,
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
			G_CALLBACK (gimp_drawable_list_view_drawable_changed),
			view);
      g_signal_connect (G_OBJECT (view->gimage), "size_changed",
			G_CALLBACK (gimp_drawable_list_view_size_changed),
			view);
      g_signal_connect (G_OBJECT (view->gimage), "floating_selection_changed",
			G_CALLBACK (gimp_drawable_list_view_floating_selection_changed),
			view);

      gimp_drawable_list_view_drawable_changed (view->gimage, view);

      if (gimp_image_floating_sel (view->gimage))
	gimp_drawable_list_view_floating_selection_changed (view->gimage, view);
    }

  gtk_widget_set_sensitive (view->new_button, (view->gimage != NULL));
}


/*  GimpContainerView methods  */

static gpointer
gimp_drawable_list_view_insert_item (GimpContainerView *view,
				     GimpViewable      *viewable,
				     gint               index)
{
  gpointer list_item = NULL;

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->insert_item)
    list_item = GIMP_CONTAINER_VIEW_CLASS (parent_class)->insert_item (view,
								       viewable,
								       index);

  if (list_item)
    gimp_list_item_set_reorderable (GIMP_LIST_ITEM (list_item),
				    TRUE, view->container);

  return list_item;
}

static void
gimp_drawable_list_view_select_item (GimpContainerView *view,
				     GimpViewable      *item,
				     gpointer           insert_data)
{
  GimpDrawableListView *list_view;
  gboolean              raise_sensitive     = FALSE;
  gboolean              lower_sensitive     = FALSE;
  gboolean              duplicate_sensitive = FALSE;
  gboolean              edit_sensitive      = FALSE;
  gboolean              delete_sensitive    = FALSE;

  list_view = GIMP_DRAWABLE_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  if (item)
    {
      GimpDrawable  *drawable;
      gint           index;

      drawable  = list_view->get_drawable_func (list_view->gimage);

      if (drawable != GIMP_DRAWABLE (item))
	{
	  list_view->set_drawable_func (list_view->gimage,
					GIMP_DRAWABLE (item));

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
gimp_drawable_list_view_activate_item (GimpContainerView *view,
				       GimpViewable      *item,
				       gpointer           insert_data)
{
  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->activate_item (view,
							     item,
							     insert_data);

  gtk_button_clicked (GTK_BUTTON (GIMP_DRAWABLE_LIST_VIEW (view)->edit_button));
}

static void
gimp_drawable_list_view_context_item (GimpContainerView *view,
				      GimpViewable      *item,
				      gpointer           insert_data)
{
  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->context_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->context_item (view,
							    item,
							    insert_data);

  GIMP_DRAWABLE_LIST_VIEW (view)->drawable_context_func
    (gimp_drawable_gimage (GIMP_DRAWABLE (item)));
}


/*  "New" functions  */

static void
gimp_drawable_list_view_new_drawable (GimpDrawableListView *view,
				      GimpDrawable         *drawable)
{
  if (drawable)
    {
      g_print ("new with \"%s\"'s properties\n", GIMP_OBJECT (drawable)->name);
    }
  else
    {
      view->new_drawable_func (view->gimage);
    }
}

static void
gimp_drawable_list_view_new_clicked (GtkWidget            *widget,
				     GimpDrawableListView *view)
{
  gimp_drawable_list_view_new_drawable (view, NULL);
}

static void
gimp_drawable_list_view_new_dropped (GtkWidget    *widget,
				     GimpViewable *viewable,
				     gpointer      data)
{
  GimpDrawableListView *view;

  view = (GimpDrawableListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_drawable_list_view_new_drawable (view, GIMP_DRAWABLE (viewable));
    }
}


/*  "Duplicate" functions  */

static void
gimp_drawable_list_view_duplicate_clicked (GtkWidget            *widget,
					   GimpDrawableListView *view)
{
  GimpDrawable *drawable;
  GimpDrawable *new_drawable;

  drawable = view->get_drawable_func (view->gimage);

  new_drawable = view->copy_drawable_func (drawable, TRUE);
  view->add_drawable_func (view->gimage, new_drawable, -1);

  gdisplays_flush ();
}


/*  "Raise/Lower" functions  */

static void
gimp_drawable_list_view_raise_clicked (GtkWidget            *widget,
				       GimpDrawableListView *view)
{
  GimpContainer *container;
  GimpDrawable  *drawable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  drawable  = view->get_drawable_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (drawable));

  if (index > 0)
    {
      view->reorder_drawable_func (view->gimage, drawable, index - 1, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_drawable_list_view_raise_extended_clicked (GtkWidget            *widget,
						guint                 state,
						GimpDrawableListView *view)
{
  GimpContainer *container;
  GimpDrawable  *drawable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  drawable  = view->get_drawable_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (drawable));

  if ((state & GDK_SHIFT_MASK) && (index > 0))
    {
      view->reorder_drawable_func (view->gimage, drawable, 0, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_drawable_list_view_lower_clicked (GtkWidget            *widget,
				       GimpDrawableListView *view)
{
  GimpContainer *container;
  GimpDrawable  *drawable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  drawable  = view->get_drawable_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (drawable));

  if (index < container->num_children - 1)
    {
      view->reorder_drawable_func (view->gimage, drawable, index + 1, TRUE);

      gdisplays_flush ();
    }
}

static void
gimp_drawable_list_view_lower_extended_clicked (GtkWidget            *widget,
						guint                 state,
						GimpDrawableListView *view)
{
  GimpContainer *container;
  GimpDrawable  *drawable;
  gint           index;

  container = GIMP_CONTAINER_VIEW (view)->container;
  drawable  = view->get_drawable_func (view->gimage);

  index = gimp_container_get_child_index (container, GIMP_OBJECT (drawable));

  if ((state & GDK_SHIFT_MASK) && (index < container->num_children - 1))
    {
      view->reorder_drawable_func (view->gimage, drawable,
				   container->num_children - 1, TRUE);

      gdisplays_flush ();
    }
}


/*  "Edit" functions  */

static void
gimp_drawable_list_view_edit_clicked (GtkWidget            *widget,
				      GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (view->gimage);

  view->edit_drawable_func (drawable);
}


/*  "Delete" functions  */

static void
gimp_drawable_list_view_delete_clicked (GtkWidget            *widget,
					GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (view->gimage);

  view->remove_drawable_func (view->gimage, drawable);

  gdisplays_flush ();
}


/*  GimpImage callbacks  */

static void
gimp_drawable_list_view_drawable_changed (GimpImage            *gimage,
					  GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (gimage);

  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (view),
				   (GimpViewable *) drawable);
}

static void
gimp_drawable_list_view_size_changed (GimpImage            *gimage,
				      GimpDrawableListView *view)
{
  gint preview_size;

  preview_size = GIMP_CONTAINER_VIEW (view)->preview_size;

  gimp_container_view_set_preview_size (GIMP_CONTAINER_VIEW (view),
					preview_size);
}

static void
gimp_drawable_list_view_floating_selection_changed (GimpImage            *gimage,
						    GimpDrawableListView *view)
{
  GimpViewable *floating_sel;
  GList        *list;

  floating_sel = (GimpViewable *) gimp_image_floating_sel (gimage);

  for (list = GTK_LIST (GIMP_CONTAINER_LIST_VIEW (view)->gtk_list)->children;
       list;
       list = g_list_next (list))
    {
      if (! (GIMP_PREVIEW (GIMP_LIST_ITEM (list->data)->preview)->viewable ==
	     floating_sel))
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (list->data),
				    floating_sel == NULL);
	}
    }

  /*  update button states  */
  gimp_drawable_list_view_drawable_changed (gimage, view);
}
