/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainermenuimpl.c
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "gimpcontainer.h"
#include "gimpcontainermenuimpl.h"
#include "gimpcontext.h"
#include "gimplist.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"


static void     gimp_container_menu_impl_class_init   (GimpContainerMenuImplClass *klass);
static void     gimp_container_menu_impl_init         (GimpContainerMenuImpl      *panel);
static void     gimp_container_menu_impl_destroy      (GtkObject                  *object);

static gpointer gimp_container_menu_impl_insert_item  (GimpContainerMenu      *view,
						       GimpViewable           *viewable,
						       gint                    index);
static void     gimp_container_menu_impl_remove_item  (GimpContainerMenu      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_menu_impl_reorder_item (GimpContainerMenu      *view,
						       GimpViewable           *viewable,
						       gint                    new_index,
						       gpointer                insert_data);
static void     gimp_container_menu_impl_select_item  (GimpContainerMenu      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_menu_impl_clear_items  (GimpContainerMenu      *view);
static void     gimp_container_menu_impl_set_preview_size (GimpContainerMenu  *view);

static void     gimp_container_menu_impl_set_history  (GimpContainerMenu      *view,
						       gint                    history);

static void    gimp_container_menu_impl_item_selected (GtkWidget              *widget,
						       gpointer                data);


static GimpContainerMenuClass *parent_class = NULL;


GtkType
gimp_container_menu_impl_get_type (void)
{
  static guint menu_impl_type = 0;

  if (! menu_impl_type)
    {
      GtkTypeInfo menu_impl_info =
      {
	"GimpContainerMenuImpl",
	sizeof (GimpContainerMenuImpl),
	sizeof (GimpContainerMenuImplClass),
	(GtkClassInitFunc) gimp_container_menu_impl_class_init,
	(GtkObjectInitFunc) gimp_container_menu_impl_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      menu_impl_type = gtk_type_unique (GIMP_TYPE_CONTAINER_MENU,
					&menu_impl_info);
    }

  return menu_impl_type;
}

static void
gimp_container_menu_impl_class_init (GimpContainerMenuImplClass *klass)
{
  GtkObjectClass         *object_class;
  GimpContainerMenuClass *container_menu_class;

  object_class         = (GtkObjectClass *) klass;
  container_menu_class = (GimpContainerMenuClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER_MENU);

  object_class->destroy = gimp_container_menu_impl_destroy;

  container_menu_class->insert_item      = gimp_container_menu_impl_insert_item;
  container_menu_class->remove_item      = gimp_container_menu_impl_remove_item;
  container_menu_class->reorder_item     = gimp_container_menu_impl_reorder_item;
  container_menu_class->select_item      = gimp_container_menu_impl_select_item;
  container_menu_class->clear_items      = gimp_container_menu_impl_clear_items;
  container_menu_class->set_preview_size = gimp_container_menu_impl_set_preview_size;
}

static void
gimp_container_menu_impl_init (GimpContainerMenuImpl *menu_impl)
{
  menu_impl->empty_item = NULL;
}

static void
gimp_container_menu_impl_destroy (GtkObject *object)
{
  GimpContainerMenuImpl *menu_impl;

  menu_impl = GIMP_CONTAINER_MENU_IMPL (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_container_menu_new (GimpContainer *container,
			 GimpContext   *context,
			 gint           preview_size)
{
  GimpContainerMenuImpl *menu_impl;
  GimpContainerMenu     *menu;

  g_return_val_if_fail (! container || GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);

  menu_impl = gtk_type_new (GIMP_TYPE_CONTAINER_MENU_IMPL);

  menu = GIMP_CONTAINER_MENU (menu_impl);

  menu->preview_size = preview_size;

  menu_impl->empty_item = gtk_menu_item_new_with_label ("(none)");
  gtk_widget_set_usize (menu_impl->empty_item,
			-1,
			preview_size +
			2 * menu_impl->empty_item->style->klass->ythickness);
  gtk_widget_set_sensitive (menu_impl->empty_item, FALSE);
  gtk_widget_show (menu_impl->empty_item);

  gtk_menu_append (GTK_MENU (menu), menu_impl->empty_item);

  if (container)
    gimp_container_menu_set_container (menu, container);

  if (context)
    gimp_container_menu_set_context (menu, context);

  return GTK_WIDGET (menu_impl);
}


/*  GimpContainerMenu methods  */

static gpointer
gimp_container_menu_impl_insert_item (GimpContainerMenu *menu,
				      GimpViewable      *viewable,
				      gint               index)
{
  GtkWidget *menu_item;

  menu_item = gimp_menu_item_new (viewable, menu->preview_size);

  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (gimp_container_menu_impl_item_selected),
		      menu);

  gtk_menu_insert (GTK_MENU (menu), menu_item, index);
  gtk_widget_show (menu_item);

  gtk_menu_reorder_child (GTK_MENU (menu),
			  GIMP_CONTAINER_MENU_IMPL (menu)->empty_item, -1);

  if (g_list_length (GTK_MENU_SHELL (menu)->children) == 2)
    gtk_widget_hide (GIMP_CONTAINER_MENU_IMPL (menu)->empty_item);

  return (gpointer) menu_item;
}

static void
gimp_container_menu_impl_remove_item (GimpContainerMenu *menu,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GtkWidget *menu_item;

  if (insert_data)
    menu_item = GTK_WIDGET (insert_data);
  else
    menu_item = NULL;

  g_print ("remove %p %p\n", viewable, menu_item);

  if (menu_item)
    {
      if (g_list_length (GTK_MENU_SHELL (menu)->children) == 2)
	gtk_widget_show (GIMP_CONTAINER_MENU_IMPL (menu)->empty_item);

      gtk_container_remove (GTK_CONTAINER (menu), menu_item);

      if (g_list_length (GTK_MENU_SHELL (menu)->children) == 1)
	{
	  gtk_widget_show (GIMP_CONTAINER_MENU_IMPL (menu)->empty_item);

	  gimp_container_menu_impl_set_history (menu, 0);
	}
    }
}

static void
gimp_container_menu_impl_reorder_item (GimpContainerMenu *menu,
				       GimpViewable      *viewable,
				       gint               new_index,
				       gpointer           insert_data)
{
  GtkWidget *menu_item;
  gboolean   active;

  if (insert_data)
    menu_item = GTK_WIDGET (insert_data);
  else
    menu_item = NULL;

  if (menu_item)
    {
      active = (gtk_menu_get_active (GTK_MENU (menu)) == menu_item);

      gtk_menu_reorder_child (GTK_MENU (menu),
			      GTK_WIDGET (menu_item), new_index);

      if (active)
	gimp_container_menu_impl_set_history (menu, new_index);
    }
}

static void
gimp_container_menu_impl_select_item (GimpContainerMenu *menu,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GtkWidget *menu_item;

  if (insert_data)
    menu_item = GTK_WIDGET (insert_data);
  else
    menu_item = NULL;

  if (menu_item)
    {
      gint index;

      index = gimp_container_get_child_index (menu->container,
					      GIMP_OBJECT (viewable));

      gimp_container_menu_impl_set_history (menu, index);
    }
}

static void
gimp_container_menu_impl_clear_items (GimpContainerMenu *menu)
{
  while (GTK_MENU_SHELL (menu)->children)
    {
      gtk_container_remove (GTK_CONTAINER (menu),
			    GTK_WIDGET (GTK_MENU_SHELL (menu)->children->data));
    }

  gtk_widget_show (GIMP_CONTAINER_MENU_IMPL (menu)->empty_item);

  if (GIMP_CONTAINER_MENU_CLASS (parent_class)->clear_items)
    GIMP_CONTAINER_MENU_CLASS (parent_class)->clear_items (menu);
}

static void
gimp_container_menu_impl_set_preview_size (GimpContainerMenu *menu)
{
  GList *list;

  for (list = GTK_MENU_SHELL (menu)->children;
       list;
       list = g_list_next (list))
    {
      GimpMenuItem *menu_item;

      menu_item = GIMP_MENU_ITEM (list->data);

      gimp_preview_set_size (GIMP_PREVIEW (menu_item->preview),
			     menu->preview_size,
			     GIMP_PREVIEW (menu_item->preview)->border_width);
    }
}

static void
gimp_container_menu_impl_set_history (GimpContainerMenu *menu,
				      gint               history)
{
  GtkWidget *parent;

  parent = gtk_menu_get_attach_widget (GTK_MENU (menu));

  if (parent && GTK_IS_OPTION_MENU (parent))
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (parent), history);
    }
  else
    {
      gtk_menu_set_active (GTK_MENU (menu), history);
    }
}


/*  GtkMenuItem callbacks  */

static void
gimp_container_menu_impl_item_selected (GtkWidget *widget,
					gpointer   data)
{
  GimpMenuItem *menu_item;

  menu_item = GIMP_MENU_ITEM (widget);

  gimp_container_menu_item_selected (GIMP_CONTAINER_MENU (data),
				     GIMP_PREVIEW (menu_item->preview)->viewable);
}
