/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainermenu.c
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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpcontainermenu.h"
#include "gimpcontainerview-utils.h"


enum
{
  SET_CONTAINER,
  INSERT_ITEM,
  REMOVE_ITEM,
  REORDER_ITEM,
  SELECT_ITEM,
  ACTIVATE_ITEM,
  CONTEXT_ITEM,
  CLEAR_ITEMS,
  SET_PREVIEW_SIZE,
  LAST_SIGNAL
};


static void   gimp_container_menu_class_init  (GimpContainerMenuClass *klass);
static void   gimp_container_menu_init        (GimpContainerMenu      *panel);
static void   gimp_container_menu_destroy     (GtkObject              *object);

static void   gimp_container_menu_real_set_container (GimpContainerMenu *menu,
						      GimpContainer     *container);

static void   gimp_container_menu_clear_items (GimpContainerMenu      *menu);
static void   gimp_container_menu_real_clear_items (GimpContainerMenu *menu);

static void   gimp_container_menu_add_foreach (GimpViewable           *viewable,
					       GimpContainerMenu      *menu);
static void   gimp_container_menu_add         (GimpContainerMenu      *menu,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_menu_remove      (GimpContainerMenu      *menu,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_menu_reorder     (GimpContainerMenu      *menu,
					       GimpViewable           *viewable,
					       gint                    new_index,
					       GimpContainer          *container);

static void   gimp_container_menu_context_changed (GimpContext        *context,
						   GimpViewable       *viewable,
						   GimpContainerMenu  *menu);


static guint  menu_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


GtkType
gimp_container_menu_get_type (void)
{
  static guint menu_type = 0;

  if (! menu_type)
    {
      GtkTypeInfo menu_info =
      {
	"GimpContainerMenu",
	sizeof (GimpContainerMenu),
	sizeof (GimpContainerMenuClass),
	(GtkClassInitFunc) gimp_container_menu_class_init,
	(GtkObjectInitFunc) gimp_container_menu_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      menu_type = gtk_type_unique (GTK_TYPE_MENU, &menu_info);
    }

  return menu_type;
}

static void
gimp_container_menu_class_init (GimpContainerMenuClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;
  
  parent_class = g_type_class_peek_parent (klass);

  menu_signals[SET_CONTAINER] =
    g_signal_new ("set_container",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, set_container),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_OBJECT);

  menu_signals[INSERT_ITEM] =
    g_signal_new ("insert_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, insert_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_POINTER__OBJECT_INT,
		  G_TYPE_POINTER, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_INT);

  menu_signals[REMOVE_ITEM] =
    g_signal_new ("remove_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, remove_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[REORDER_ITEM] =
    g_signal_new ("reorder_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, reorder_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_INT_POINTER,
		  G_TYPE_NONE, 3,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_INT,
		  G_TYPE_POINTER);

  menu_signals[SELECT_ITEM] =
    g_signal_new ("select_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, select_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, activate_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[CONTEXT_ITEM] =
    g_signal_new ("context_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, context_item),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[CLEAR_ITEMS] =
    g_signal_new ("clear_items",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, clear_items),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  menu_signals[SET_PREVIEW_SIZE] =
    g_signal_new ("set_preview_size",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, set_preview_size),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy   = gimp_container_menu_destroy;

  klass->set_container    = gimp_container_menu_real_set_container;
  klass->insert_item      = NULL;
  klass->remove_item      = NULL;
  klass->reorder_item     = NULL;
  klass->select_item      = NULL;
  klass->activate_item    = NULL;
  klass->context_item     = NULL;
  klass->clear_items      = gimp_container_menu_real_clear_items;
  klass->set_preview_size = NULL;
}

static void
gimp_container_menu_init (GimpContainerMenu *menu)
{
  menu->container    = NULL;
  menu->context      = NULL;

  menu->hash_table   = g_hash_table_new (g_direct_hash, g_direct_equal);

  menu->preview_size = 0;
}

static void
gimp_container_menu_destroy (GtkObject *object)
{
  GimpContainerMenu *menu;

  menu = GIMP_CONTAINER_MENU (object);

  if (menu->container)
    gimp_container_menu_set_container (menu, NULL);

  if (menu->context)
    gimp_container_menu_set_context (menu, NULL);

  if (menu->hash_table)
    {
      g_hash_table_destroy (menu->hash_table);
      menu->hash_table = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

void
gimp_container_menu_set_container (GimpContainerMenu *menu,
				   GimpContainer     *container)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (!container || GIMP_IS_CONTAINER (container));

  if (container == menu->container)
    return;

  g_signal_emit (G_OBJECT (menu), menu_signals[SET_CONTAINER], 0,
		 container);
}

static void
gimp_container_menu_real_set_container (GimpContainerMenu *menu,
					GimpContainer     *container)
{
  if (menu->container)
    {
      gimp_container_menu_select_item (menu, NULL);

      gimp_container_menu_clear_items (menu);

      g_signal_handlers_disconnect_by_func (G_OBJECT (menu->container),
					    gimp_container_menu_add,
					    menu);
      g_signal_handlers_disconnect_by_func (G_OBJECT (menu->container),
					    gimp_container_menu_remove,
					    menu);
      g_signal_handlers_disconnect_by_func (G_OBJECT (menu->container),
					    gimp_container_menu_reorder,
					    menu);

      g_hash_table_destroy (menu->hash_table);

      menu->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      if (menu->context)
	{
	  g_signal_handlers_disconnect_by_func (G_OBJECT (menu->context),
						gimp_container_menu_context_changed,
						menu);
	}

      if (menu->get_name_func &&
	  gimp_container_view_is_built_in_name_func (menu->get_name_func))
	{
	  gimp_container_menu_set_name_func (menu, NULL);
	}
    }

  menu->container = container;

  if (menu->container)
    {
      if (! menu->get_name_func)
	{
	  GimpItemGetNameFunc get_name_func;

	  get_name_func = gimp_container_view_get_built_in_name_func (menu->container->children_type);

	  gimp_container_menu_set_name_func (menu, get_name_func);
	}

      gimp_container_foreach (menu->container,
			      (GFunc) gimp_container_menu_add_foreach,
			      menu);

      g_signal_connect_swapped (G_OBJECT (menu->container), "add",
				G_CALLBACK (gimp_container_menu_add),
				menu);

      g_signal_connect_swapped (G_OBJECT (menu->container), "remove",
				G_CALLBACK (gimp_container_menu_remove),
				menu);

      g_signal_connect_swapped (G_OBJECT (menu->container), "reorder",
				G_CALLBACK (gimp_container_menu_reorder),
				menu);

      if (menu->context)
	{
	  GimpObject  *object;
	  const gchar *signal_name;

	  signal_name =
	    gimp_context_type_to_signal_name (menu->container->children_type);

	  g_signal_connect (G_OBJECT (menu->context), signal_name,
			    G_CALLBACK (gimp_container_menu_context_changed),
			    menu);

	  object = gimp_context_get_by_type (menu->context,
					     menu->container->children_type);

	  gimp_container_menu_select_item (menu,
					   object ?
					   GIMP_VIEWABLE (object): NULL);
	}
    }
}

void
gimp_container_menu_set_context (GimpContainerMenu *menu,
				 GimpContext       *context)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (context == menu->context)
    return;

  if (menu->context && menu->container)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (menu->context),
					    gimp_container_menu_context_changed,
					    menu);
    }

  menu->context = context;

  if (menu->context && menu->container)
    {
      GimpObject  *object;
      const gchar *signal_name;

      signal_name =
	gimp_context_type_to_signal_name (menu->container->children_type);

      g_signal_connect (G_OBJECT (menu->context), signal_name,
			G_CALLBACK (gimp_container_menu_context_changed),
			menu);

      object = gimp_context_get_by_type (menu->context,
					 menu->container->children_type);

      gimp_container_menu_select_item (menu,
				       object ? GIMP_VIEWABLE (object) : NULL);
    }
}

void
gimp_container_menu_set_preview_size (GimpContainerMenu *menu,
				      gint               preview_size)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (preview_size > 0 && preview_size <= 256 /* FIXME: 64 */);

  if (menu->preview_size != preview_size)
    {
      menu->preview_size = preview_size;

      g_signal_emit (G_OBJECT (menu), menu_signals[SET_PREVIEW_SIZE], 0);
    }
}

void
gimp_container_menu_set_name_func (GimpContainerMenu   *menu,
				   GimpItemGetNameFunc  get_name_func)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));

  if (menu->get_name_func != get_name_func)
    {
      menu->get_name_func = get_name_func;
    }
}

void
gimp_container_menu_select_item (GimpContainerMenu *menu,
				 GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  g_signal_emit (G_OBJECT (menu), menu_signals[SELECT_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_menu_activate_item (GimpContainerMenu *menu,
				   GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  g_signal_emit (G_OBJECT (menu), menu_signals[ACTIVATE_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_menu_context_item (GimpContainerMenu *menu,
				  GimpViewable      *viewable)
{
  gpointer insert_data;

  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  g_signal_emit (G_OBJECT (menu), menu_signals[CONTEXT_ITEM], 0,
		 viewable, insert_data);
}

void
gimp_container_menu_item_selected (GimpContainerMenu *menu,
				   GimpViewable      *viewable)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (menu->container && menu->context)
    {
      gimp_context_set_by_type (menu->context,
				menu->container->children_type,
				GIMP_OBJECT (viewable));
    }

  gimp_container_menu_select_item (menu, viewable);
}

void
gimp_container_menu_item_activated (GimpContainerMenu *menu,
				    GimpViewable      *viewable)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_menu_activate_item (menu, viewable);
}

void
gimp_container_menu_item_context (GimpContainerMenu *menu,
				  GimpViewable      *viewable)
{
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (viewable != NULL);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_menu_context_item (menu, viewable);
}

static void
gimp_container_menu_clear_items (GimpContainerMenu *menu)
{
  g_signal_emit (G_OBJECT (menu), menu_signals[CLEAR_ITEMS], 0);
}

static void
gimp_container_menu_real_clear_items (GimpContainerMenu *menu)
{
  g_hash_table_destroy (menu->hash_table);

  menu->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
gimp_container_menu_add_foreach (GimpViewable      *viewable,
				 GimpContainerMenu *menu)
{
  gpointer insert_data = NULL;

  g_signal_emit (G_OBJECT (menu), menu_signals[INSERT_ITEM], 0,
		 viewable, -1, &insert_data);

  g_hash_table_insert (menu->hash_table, viewable, insert_data);
}

static void
gimp_container_menu_add (GimpContainerMenu *menu,
			 GimpViewable      *viewable,
			 GimpContainer     *container)
{
  gpointer insert_data = NULL;
  gint     index;

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  g_signal_emit (G_OBJECT (menu), menu_signals[INSERT_ITEM], 0,
		 viewable, index, &insert_data);

  g_hash_table_insert (menu->hash_table, viewable, insert_data);
}

static void
gimp_container_menu_remove (GimpContainerMenu *menu,
			    GimpViewable      *viewable,
			    GimpContainer     *container)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  if (insert_data)
    {
      g_hash_table_remove (menu->hash_table, viewable);

      g_signal_emit (G_OBJECT (menu), menu_signals[REMOVE_ITEM], 0,
		     viewable, insert_data);
    }
}

static void
gimp_container_menu_reorder (GimpContainerMenu *menu,
			     GimpViewable      *viewable,
			     gint               new_index,
			     GimpContainer     *container)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  if (insert_data)
    {
      g_signal_emit (G_OBJECT (menu), menu_signals[REORDER_ITEM], 0,
		     viewable, new_index, insert_data);
    }
}

static void
gimp_container_menu_context_changed (GimpContext       *context,
				     GimpViewable      *viewable,
				     GimpContainerMenu *menu)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  g_signal_emit (G_OBJECT (menu), menu_signals[SELECT_ITEM], 0,
		 viewable, insert_data);
}
