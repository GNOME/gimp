/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainermenu.c
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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpcontainermenu.h"
#include "gimpcontainerview-utils.h"


enum
{
  SELECT_ITEM,
  ACTIVATE_ITEM,
  CONTEXT_ITEM,
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


GType
gimp_container_menu_get_type (void)
{
  static GType menu_type = 0;

  if (! menu_type)
    {
      static const GTypeInfo menu_info =
      {
        sizeof (GimpContainerMenuClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_menu_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerMenu),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_menu_init,
      };

      menu_type = g_type_register_static (GTK_TYPE_MENU,
                                          "GimpContainerMenu",
                                          &menu_info, 0);
    }

  return menu_type;
}

static void
gimp_container_menu_class_init (GimpContainerMenuClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  menu_signals[SELECT_ITEM] =
    g_signal_new ("select_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, select_item),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, activate_item),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  menu_signals[CONTEXT_ITEM] =
    g_signal_new ("context_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerMenuClass, context_item),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  object_class->destroy   = gimp_container_menu_destroy;

  klass->select_item      = NULL;
  klass->activate_item    = NULL;
  klass->context_item     = NULL;

  klass->set_container    = gimp_container_menu_real_set_container;
  klass->insert_item      = NULL;
  klass->remove_item      = NULL;
  klass->reorder_item     = NULL;
  klass->clear_items      = gimp_container_menu_real_clear_items;
  klass->set_preview_size = NULL;
}

static void
gimp_container_menu_init (GimpContainerMenu *menu)
{
  menu->container            = NULL;
  menu->context              = NULL;
  menu->hash_table           = g_hash_table_new (g_direct_hash, g_direct_equal);
  menu->preview_size         = 0;
  menu->preview_border_width = 1;
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
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (container == NULL || GIMP_IS_CONTAINER (container));

  if (container != menu->container)
    GIMP_CONTAINER_MENU_GET_CLASS (menu)->set_container (menu, container);
}

static void
gimp_container_menu_real_set_container (GimpContainerMenu *menu,
					GimpContainer     *container)
{
  if (menu->container)
    {
      gimp_container_menu_select_item (menu, NULL);

      gimp_container_menu_clear_items (menu);

      g_signal_handlers_disconnect_by_func (menu->container,
					    gimp_container_menu_add,
					    menu);
      g_signal_handlers_disconnect_by_func (menu->container,
					    gimp_container_menu_remove,
					    menu);
      g_signal_handlers_disconnect_by_func (menu->container,
					    gimp_container_menu_reorder,
					    menu);

      g_hash_table_destroy (menu->hash_table);

      menu->hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      if (menu->context)
	{
	  g_signal_handlers_disconnect_by_func (menu->context,
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

      g_signal_connect_swapped (menu->container, "add",
				G_CALLBACK (gimp_container_menu_add),
				menu);

      g_signal_connect_swapped (menu->container, "remove",
				G_CALLBACK (gimp_container_menu_remove),
				menu);

      g_signal_connect_swapped (menu->container, "reorder",
				G_CALLBACK (gimp_container_menu_reorder),
				menu);

      if (menu->context)
	{
	  GimpObject  *object;
	  const gchar *signal_name;

	  signal_name =
	    gimp_context_type_to_signal_name (menu->container->children_type);

	  g_signal_connect (menu->context, signal_name,
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
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail ( ! context || GIMP_IS_CONTEXT (context));

  if (context == menu->context)
    return;

  if (menu->context)
    {
      if (menu->container)
        {
          g_signal_handlers_disconnect_by_func (menu->context,
                                                gimp_container_menu_context_changed,
                                                menu);
        }

      g_object_unref (menu->context);
    }

  menu->context = context;

  if (menu->context)
    {
      g_object_ref (menu->context);

      if (menu->container)
        {
          GimpObject  *object;
          const gchar *signal_name;

          signal_name =
            gimp_context_type_to_signal_name (menu->container->children_type);

          g_signal_connect (menu->context, signal_name,
                            G_CALLBACK (gimp_container_menu_context_changed),
                            menu);

          object = gimp_context_get_by_type (menu->context,
                                             menu->container->children_type);

          gimp_container_menu_select_item (menu,
                                           object ? GIMP_VIEWABLE (object) : NULL);
        }
    }
}

void
gimp_container_menu_set_preview_size (GimpContainerMenu *menu,
				      gint               preview_size)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (preview_size  > 0 &&
                    preview_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE);

  if (menu->preview_size != preview_size)
    {
      menu->preview_size = preview_size;

      GIMP_CONTAINER_MENU_GET_CLASS (menu)->set_preview_size (menu);
    }
}

void
gimp_container_menu_set_name_func (GimpContainerMenu   *menu,
				   GimpItemGetNameFunc  get_name_func)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));

  menu->get_name_func = get_name_func;
}

void
gimp_container_menu_select_item (GimpContainerMenu *menu,
				 GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  if (menu->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (menu->hash_table, viewable);

      g_signal_emit (menu, menu_signals[SELECT_ITEM], 0,
                     viewable, insert_data);
    }
}

void
gimp_container_menu_activate_item (GimpContainerMenu *menu,
				   GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (menu->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (menu->hash_table, viewable);

      g_signal_emit (menu, menu_signals[ACTIVATE_ITEM], 0,
                     viewable, insert_data);
    }
}

void
gimp_container_menu_context_item (GimpContainerMenu *menu,
				  GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (menu->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (menu->hash_table, viewable);

      g_signal_emit (menu, menu_signals[CONTEXT_ITEM], 0,
                     viewable, insert_data);
    }
}

void
gimp_container_menu_item_selected (GimpContainerMenu *menu,
				   GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (menu->container && menu->context)
    {
      GimpContext *context;

      /*  ref and remember the context because menu->context may
       *  become NULL by calling gimp_context_set_by_type()
       */
      context = g_object_ref (menu->context);

      g_signal_handlers_block_by_func (context,
                                       gimp_container_menu_context_changed,
                                       menu);

      gimp_context_set_by_type (context,
				menu->container->children_type,
				GIMP_OBJECT (viewable));

      g_signal_handlers_unblock_by_func (context,
                                         gimp_container_menu_context_changed,
                                         menu);

      g_object_unref (context);
    }

  gimp_container_menu_select_item (menu, viewable);
}

void
gimp_container_menu_item_activated (GimpContainerMenu *menu,
				    GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_menu_activate_item (menu, viewable);
}

void
gimp_container_menu_item_context (GimpContainerMenu *menu,
				  GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_MENU (menu));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_menu_context_item (menu, viewable);
}

static void
gimp_container_menu_clear_items (GimpContainerMenu *menu)
{
  GIMP_CONTAINER_MENU_GET_CLASS (menu)->clear_items (menu);
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
  gpointer insert_data;

  insert_data = GIMP_CONTAINER_MENU_GET_CLASS (menu)->insert_item (menu,
								   viewable,
								   -1);

  g_hash_table_insert (menu->hash_table, viewable, insert_data);
}

static void
gimp_container_menu_add (GimpContainerMenu *menu,
			 GimpViewable      *viewable,
			 GimpContainer     *container)
{
  gpointer insert_data;
  gint     index;

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  insert_data = GIMP_CONTAINER_MENU_GET_CLASS (menu)->insert_item (menu,
								   viewable,
								   index);

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
      GIMP_CONTAINER_MENU_GET_CLASS (menu)->remove_item (menu,
							 viewable,
							 insert_data);

      g_hash_table_remove (menu->hash_table, viewable);
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
      GIMP_CONTAINER_MENU_GET_CLASS (menu)->reorder_item (menu,
							  viewable,
							  new_index,
							  insert_data);
    }
}

static void
gimp_container_menu_context_changed (GimpContext       *context,
				     GimpViewable      *viewable,
				     GimpContainerMenu *menu)
{
  gpointer insert_data;

  insert_data = g_hash_table_lookup (menu->hash_table, viewable);

  g_signal_emit (menu, menu_signals[SELECT_ITEM], 0, viewable, insert_data);
}
