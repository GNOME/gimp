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

#include "pixmaps/delete.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/new.xpm"
#include "pixmaps/edit.xpm"


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

static void  gimp_drawable_list_view_duplicate_drawable (GimpDrawableListView *view,
							 GimpDrawable         *drawable);
static void   gimp_drawable_list_view_duplicate_clicked (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_duplicate_dropped (GtkWidget            *widget,
							 GimpViewable         *viewable,
							 gpointer              drawable);

static void   gimp_drawable_list_view_edit_drawable     (GimpDrawableListView *view,
							 GimpDrawable         *drawable);
static void   gimp_drawable_list_view_edit_clicked      (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_edit_dropped      (GtkWidget            *widget,
							 GimpViewable         *viewable,
							 gpointer              drawable);

static void   gimp_drawable_list_view_delete_drawable   (GimpDrawableListView *view,
							 GimpDrawable         *drawable);
static void   gimp_drawable_list_view_delete_clicked    (GtkWidget            *widget,
							 GimpDrawableListView *view);
static void   gimp_drawable_list_view_delete_dropped    (GtkWidget            *widget,
							 GimpViewable         *viewable,
							 gpointer              drawable);

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
  GtkWidget *pixmap;

  view->gimage        = NULL;
  view->drawable_type = GTK_TYPE_NONE;
  view->signal_name   = NULL;

  gtk_box_set_spacing (GTK_BOX (view), 2);

  view->button_box = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (view), view->button_box, FALSE, FALSE, 0);
  gtk_widget_show (view->button_box);

  /* new */

  view->new_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->new_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->new_button);

  gimp_help_set_help_data (view->new_button, _("New"), NULL);

  g_signal_connect (G_OBJECT (view->new_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_new_clicked),
		    view);

  pixmap = gimp_pixmap_new (new_xpm);
  gtk_container_add (GTK_CONTAINER (view->new_button), pixmap);
  gtk_widget_show (pixmap);

  /* raise */

  view->raise_button = gimp_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->raise_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->raise_button);

  gimp_help_set_help_data (view->raise_button, _("Raise          \n"
						 "<Shift> To Top"), NULL);

  g_signal_connect (G_OBJECT (view->raise_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_raise_clicked),
		    view);
  g_signal_connect (G_OBJECT (view->raise_button), "extended_clicked",
		    G_CALLBACK (gimp_drawable_list_view_raise_extended_clicked),
		    view);

  pixmap = gimp_pixmap_new (raise_xpm);
  gtk_container_add (GTK_CONTAINER (view->raise_button), pixmap);
  gtk_widget_show (pixmap);

  /* lower */

  view->lower_button = gimp_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->lower_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->lower_button);

  gimp_help_set_help_data (view->lower_button, _("Lower             \n"
						 "<Shift> To Bottom"), NULL);

  g_signal_connect (G_OBJECT (view->lower_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_lower_clicked),
		    view);
  g_signal_connect (G_OBJECT (view->lower_button), "extended_clicked",
		    G_CALLBACK (gimp_drawable_list_view_lower_extended_clicked),
		    view);

  pixmap = gimp_pixmap_new (lower_xpm);
  gtk_container_add (GTK_CONTAINER (view->lower_button), pixmap);
  gtk_widget_show (pixmap);

  /* duplicate */

  view->duplicate_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->duplicate_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->duplicate_button);

  gimp_help_set_help_data (view->duplicate_button, _("Duplicate"), NULL);

  g_signal_connect (G_OBJECT (view->duplicate_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_duplicate_clicked),
		    view);  

  pixmap = gimp_pixmap_new (duplicate_xpm);
  gtk_container_add (GTK_CONTAINER (view->duplicate_button), pixmap);
  gtk_widget_show (pixmap);

  /* edit */

  view->edit_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->edit_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->edit_button);

  gimp_help_set_help_data (view->edit_button, _("Edit"), NULL);

  g_signal_connect (G_OBJECT (view->edit_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_edit_clicked),
		    view);  

  pixmap = gimp_pixmap_new (edit_xpm);
  gtk_container_add (GTK_CONTAINER (view->edit_button), pixmap);
  gtk_widget_show (pixmap);

  /* delete */

  view->delete_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (view->button_box), view->delete_button,
		      TRUE, TRUE, 0);
  gtk_widget_show (view->delete_button);

  gimp_help_set_help_data (view->delete_button, _("Delete"), NULL);

  g_signal_connect (G_OBJECT (view->delete_button), "clicked",
		    G_CALLBACK (gimp_drawable_list_view_delete_clicked),
		    view);  

  pixmap = gimp_pixmap_new (delete_xpm);
  gtk_container_add (GTK_CONTAINER (view->delete_button), pixmap);
  gtk_widget_show (pixmap);

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
      list_view = gtk_type_new (GIMP_TYPE_LAYER_LIST_VIEW);
    }
  else if (drawable_type == GIMP_TYPE_CHANNEL)
    {
      list_view = gtk_type_new (GIMP_TYPE_CHANNEL_LIST_VIEW);
    }
  else
    {
      list_view = gtk_type_new (GIMP_TYPE_DRAWABLE_LIST_VIEW);
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

  /*  drop to "new"  */
  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_view->new_button),
				  GTK_DEST_DEFAULT_ALL,
				  drawable_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (list_view->new_button),
			      drawable_type,
			      gimp_drawable_list_view_new_dropped,
			      list_view);

  /*  drop to "duplicate"  */
  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_view->duplicate_button),
				  GTK_DEST_DEFAULT_ALL,
				  drawable_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (list_view->duplicate_button),
			      drawable_type,
			      gimp_drawable_list_view_duplicate_dropped,
			      list_view);

  /*  drop to "edit"  */
  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_view->edit_button),
				  GTK_DEST_DEFAULT_ALL,
				  drawable_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (list_view->edit_button),
			      drawable_type,
			      gimp_drawable_list_view_edit_dropped,
			      list_view);

  /*  drop to "delete"  */
  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (list_view->delete_button),
				  GTK_DEST_DEFAULT_ALL,
				  drawable_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (list_view->delete_button),
			      drawable_type,
			      gimp_drawable_list_view_delete_dropped,
			      list_view);

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

  gimp_drawable_list_view_edit_drawable (GIMP_DRAWABLE_LIST_VIEW (view),
					 GIMP_DRAWABLE (item));
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
gimp_drawable_list_view_duplicate_drawable (GimpDrawableListView *view,
					    GimpDrawable         *drawable)
{
  GimpDrawable *new_drawable;

  new_drawable = view->copy_drawable_func (drawable, TRUE);
  view->add_drawable_func (view->gimage, new_drawable, -1);

  gdisplays_flush ();
}

static void
gimp_drawable_list_view_duplicate_clicked (GtkWidget            *widget,
					   GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (view->gimage);

  gimp_drawable_list_view_duplicate_drawable (view, drawable);
}

static void
gimp_drawable_list_view_duplicate_dropped (GtkWidget    *widget,
					   GimpViewable *viewable,
					   gpointer      data)
{
  GimpDrawableListView *view;

  view = (GimpDrawableListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_drawable_list_view_duplicate_drawable (view,
						  GIMP_DRAWABLE (viewable));
    }
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
gimp_drawable_list_view_edit_drawable (GimpDrawableListView *view,
				       GimpDrawable         *drawable)
{
  view->edit_drawable_func (drawable);
}

static void
gimp_drawable_list_view_edit_clicked (GtkWidget            *widget,
				      GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (view->gimage);

  gimp_drawable_list_view_edit_drawable (view, drawable);
}

static void
gimp_drawable_list_view_edit_dropped (GtkWidget    *widget,
				      GimpViewable *viewable,
				      gpointer      data)
{
  GimpDrawableListView *view;

  view = (GimpDrawableListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_drawable_list_view_edit_drawable (view, GIMP_DRAWABLE (viewable));
    }
}


/*  "Delete" functions  */

static void
gimp_drawable_list_view_delete_drawable (GimpDrawableListView *view,
					 GimpDrawable         *drawable)
{
  view->remove_drawable_func (view->gimage, drawable);

  gdisplays_flush ();
}

static void
gimp_drawable_list_view_delete_clicked (GtkWidget            *widget,
					GimpDrawableListView *view)
{
  GimpDrawable *drawable;

  drawable = view->get_drawable_func (view->gimage);

  gimp_drawable_list_view_delete_drawable (view, drawable);
}

static void
gimp_drawable_list_view_delete_dropped (GtkWidget    *widget,
					GimpViewable *viewable,
					gpointer      data)
{
  GimpDrawableListView *view;

  view = (GimpDrawableListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_drawable_list_view_delete_drawable (view, GIMP_DRAWABLE (viewable));
    }
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
