/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimppreviewrenderer.h"


enum
{
  PROP_0,
  PROP_CONTAINER,
  PROP_CONTEXT
};

enum
{
  SELECT_ITEM,
  ACTIVATE_ITEM,
  CONTEXT_ITEM,
  LAST_SIGNAL
};


static void   gimp_container_view_class_init        (GimpContainerViewClass *klass);
static void   gimp_container_view_init              (GimpContainerView      *view,
                                                     GimpContainerViewClass *klass);

static void   gimp_container_view_set_property      (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void   gimp_container_view_get_property      (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static void   gimp_container_view_destroy           (GtkObject        *object);

static void   gimp_container_view_real_set_container (GimpContainerView *view,
						      GimpContainer     *container);

static void   gimp_container_view_clear_items (GimpContainerView      *view);
static void   gimp_container_view_real_clear_items (GimpContainerView *view);

static void   gimp_container_view_add_foreach (GimpViewable           *viewable,
					       GimpContainerView      *view);
static void   gimp_container_view_add         (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_view_remove      (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       GimpContainer          *container);
static void   gimp_container_view_reorder     (GimpContainerView      *view,
					       GimpViewable           *viewable,
					       gint                    new_index,
					       GimpContainer          *container);

static void   gimp_container_view_freeze      (GimpContainerView      *view,
                                               GimpContainer          *container);
static void   gimp_container_view_thaw        (GimpContainerView      *view,
                                               GimpContainer          *container);
static void   gimp_container_view_name_changed (GimpViewable          *viewable,
                                                GimpContainerView     *view);

static void   gimp_container_view_context_changed  (GimpContext        *context,
						    GimpViewable       *viewable,
						    GimpContainerView  *view);
static void   gimp_container_view_viewable_dropped (GtkWidget          *widget,
						    GimpViewable       *viewable,
						    gpointer            data);
static void  gimp_container_view_button_viewable_dropped (GtkWidget    *widget,
							  GimpViewable *viewable,
							  gpointer      data);


static guint  view_signals[LAST_SIGNAL] = { 0 };

static GimpEditorClass *parent_class = NULL;


GType
gimp_container_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_view_init,
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpContainerView",
                                     &view_info, 0);
    }

  return type;
}

static void
gimp_container_view_class_init (GimpContainerViewClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  view_signals[SELECT_ITEM] =
    g_signal_new ("select_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, select_item),
		  NULL, NULL,
		  gimp_marshal_BOOLEAN__OBJECT_POINTER,
		  G_TYPE_BOOLEAN, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, activate_item),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  view_signals[CONTEXT_ITEM] =
    g_signal_new ("context_item",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContainerViewClass, context_item),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_POINTER,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_OBJECT,
		  G_TYPE_POINTER);

  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  gtk_object_class->destroy  = gimp_container_view_destroy;

  klass->select_item         = NULL;
  klass->activate_item       = NULL;
  klass->context_item        = NULL;

  klass->set_container       = gimp_container_view_real_set_container;
  klass->insert_item         = NULL;
  klass->remove_item         = NULL;
  klass->reorder_item        = NULL;
  klass->rename_item         = NULL;
  klass->clear_items         = gimp_container_view_real_clear_items;
  klass->set_preview_size    = NULL;

  klass->insert_data_free = NULL;

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container", NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        G_PARAM_READWRITE));

}

static void
gimp_container_view_init (GimpContainerView      *view,
                          GimpContainerViewClass *klass)
{
}

static void
gimp_container_view_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      gimp_container_view_set_container (view, g_value_get_object (value));
      break;
    case PROP_CONTEXT:
      gimp_container_view_set_context (view, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_container_view_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      g_value_set_object (value, gimp_container_view_get_container (view));
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, gimp_container_view_get_context (view));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_container_view_destroy (GtkObject *object)
{
  GimpContainerView        *view    = GIMP_CONTAINER_VIEW (object);
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container)
    gimp_container_view_set_container (view, NULL);

  if (private->context)
    gimp_container_view_set_context (view, NULL);

  if (private->hash_table)
    {
      g_hash_table_destroy (private->hash_table);
      private->hash_table = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpContainerViewPrivate *
gimp_container_view_get_private (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  static GQuark view_private_key = 0;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  if (! view_private_key)
    view_private_key = g_quark_from_static_string ("gimp-container-view-private");

  private = g_object_get_qdata ((GObject *) (GObject *) view, view_private_key);

  if (! private)
    {
      GimpContainerViewClass *klass = GIMP_CONTAINER_VIEW_GET_CLASS (view);

      private = g_new0 (GimpContainerViewPrivate, 1);

      private->container            = NULL;
      private->context              = NULL;
      private->hash_table           = g_hash_table_new_full (g_direct_hash,
                                                             g_direct_equal,
                                                             NULL,
                                                             klass->insert_data_free);
      private->preview_size         = 0;
      private->preview_border_width = 1;
      private->reorderable          = FALSE;

      g_object_set_qdata ((GObject *) view, view_private_key, private);
    }

  return private;
}

void
gimp_container_view_construct (GimpContainerView *view,
                               GimpContainer     *container,
                               GimpContext       *context,
                               gint               preview_size,
                               gint               preview_border_width,
                               gboolean           reorderable)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (container == NULL || GIMP_IS_CONTAINER (container));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));
  g_return_if_fail (preview_size >  0 &&
                    preview_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (preview_border_width >= 0 &&
                    preview_border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  private->reorderable = reorderable ? TRUE : FALSE;

  gimp_container_view_set_preview_size (view, preview_size,
                                        preview_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);
}

GimpContainer *
gimp_container_view_get_container (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->container;
}

void
gimp_container_view_set_container (GimpContainerView *view,
				   GimpContainer     *container)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (container == NULL || GIMP_IS_CONTAINER (container));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (container != private->container)
    {
      GIMP_CONTAINER_VIEW_GET_CLASS (view)->set_container (view, container);

      g_object_notify (G_OBJECT (view), "container");
    }
}

static void
gimp_container_view_real_set_container (GimpContainerView *view,
					GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container)
    {
      GimpContainerViewClass *view_class;

      gimp_container_view_select_item (view, NULL);
      gimp_container_view_clear_items (view);

      gimp_container_remove_handler (private->container,
                                     private->name_changed_handler_id);

      g_signal_handlers_disconnect_by_func (private->container,
					    gimp_container_view_add,
					    view);
      g_signal_handlers_disconnect_by_func (private->container,
					    gimp_container_view_remove,
					    view);
      g_signal_handlers_disconnect_by_func (private->container,
					    gimp_container_view_reorder,
					    view);
      g_signal_handlers_disconnect_by_func (private->container,
					    gimp_container_view_freeze,
					    view);
      g_signal_handlers_disconnect_by_func (private->container,
					    gimp_container_view_thaw,
					    view);

      g_hash_table_destroy (private->hash_table);

      view_class = GIMP_CONTAINER_VIEW_GET_CLASS (view);

      private->hash_table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL,
                                                   view_class->insert_data_free);

      if (private->context)
	{
	  g_signal_handlers_disconnect_by_func (private->context,
						gimp_container_view_context_changed,
						view);

	  if (private->dnd_widget)
	    {
	      gtk_drag_dest_unset (private->dnd_widget);
	      gimp_dnd_viewable_dest_remove (private->dnd_widget,
                                             private->container->children_type);
	    }
	}
    }

  private->container = container;

  if (private->container)
    {
      GimpViewableClass *viewable_class;

      viewable_class = g_type_class_ref (container->children_type);

      gimp_container_foreach (private->container,
			      (GFunc) gimp_container_view_add_foreach,
			      view);

      private->name_changed_handler_id =
        gimp_container_add_handler (private->container,
                                    viewable_class->name_changed_signal,
                                    G_CALLBACK (gimp_container_view_name_changed),
                                    view);

      g_type_class_unref (viewable_class);

      g_signal_connect_object (private->container, "add",
			       G_CALLBACK (gimp_container_view_add),
			       view,
			       G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "remove",
			       G_CALLBACK (gimp_container_view_remove),
			       view,
			       G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "reorder",
			       G_CALLBACK (gimp_container_view_reorder),
			       view,
			       G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "freeze",
			       G_CALLBACK (gimp_container_view_freeze),
			       view,
			       G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "thaw",
			       G_CALLBACK (gimp_container_view_thaw),
			       view,
			       G_CONNECT_SWAPPED);

      if (private->context)
	{
	  GimpObject  *object;
	  const gchar *signal_name;

	  signal_name =
	    gimp_context_type_to_signal_name (private->container->children_type);

	  g_signal_connect_object (private->context, signal_name,
				   G_CALLBACK (gimp_container_view_context_changed),
				   view,
				   0);

	  object = gimp_context_get_by_type (private->context,
					     private->container->children_type);

	  gimp_container_view_select_item (view, (GimpViewable *) object);

	  if (private->dnd_widget)
            gimp_dnd_viewable_dest_add (private->dnd_widget,
                                        private->container->children_type,
                                        gimp_container_view_viewable_dropped,
                                        view);
	}
    }
}

GimpContext *
gimp_container_view_get_context (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->context;
}

void
gimp_container_view_set_context (GimpContainerView *view,
				 GimpContext       *context)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (context == private->context)
    return;

  if (private->context)
    {
      if (private->container)
        {
          g_signal_handlers_disconnect_by_func (private->context,
                                                gimp_container_view_context_changed,
                                                view);

          if (private->dnd_widget)
            {
              gtk_drag_dest_unset (private->dnd_widget);
              gimp_dnd_viewable_dest_remove (private->dnd_widget,
                                             private->container->children_type);
            }
        }

      g_object_unref (private->context);
    }

  private->context = context;

  if (private->context)
    {
      g_object_ref (private->context);

      if (private->container)
        {
          GimpObject  *object;
          const gchar *signal_name;

          signal_name =
            gimp_context_type_to_signal_name (private->container->children_type);

          g_signal_connect_object (private->context, signal_name,
                                   G_CALLBACK (gimp_container_view_context_changed),
                                   view,
                                   0);

          object = gimp_context_get_by_type (private->context,
                                             private->container->children_type);

          gimp_container_view_select_item (view, (GimpViewable *) object);

          if (private->dnd_widget)
            gimp_dnd_viewable_dest_add (private->dnd_widget,
                                        private->container->children_type,
                                        gimp_container_view_viewable_dropped,
                                        view);
        }
    }

  g_object_notify (G_OBJECT (view), "context");
}

gint
gimp_container_view_get_preview_size (GimpContainerView *view,
                                      gint              *preview_border_width)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), 0);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (preview_border_width)
    *preview_border_width = private->preview_border_width;

  return private->preview_size;
}

void
gimp_container_view_set_preview_size (GimpContainerView *view,
				      gint               preview_size,
                                      gint               preview_border_width)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (preview_size >  0 &&
                    preview_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (preview_border_width >= 0 &&
                    preview_border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->preview_size         != preview_size ||
      private->preview_border_width != preview_border_width)
    {
      private->preview_size         = preview_size;
      private->preview_border_width = preview_border_width;

      GIMP_CONTAINER_VIEW_GET_CLASS (view)->set_preview_size (view);
    }
}

void
gimp_container_view_enable_dnd (GimpContainerView *view,
				GtkButton         *button,
				GType              children_type)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GTK_IS_BUTTON (button));

  gimp_dnd_viewable_dest_add (GTK_WIDGET (button),
			      children_type,
			      gimp_container_view_button_viewable_dropped,
			      view);
}

gboolean
gimp_container_view_select_item (GimpContainerView *view,
				 GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;
  gboolean                  success = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (private->hash_table, viewable);

      g_signal_emit (view, view_signals[SELECT_ITEM], 0,
                     viewable, insert_data, &success);
    }

  return success;
}

void
gimp_container_view_activate_item (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (private->hash_table, viewable);

      g_signal_emit (view, view_signals[ACTIVATE_ITEM], 0,
                     viewable, insert_data);
    }
}

void
gimp_container_view_context_item (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->hash_table)
    {
      gpointer insert_data;

      insert_data = g_hash_table_lookup (private->hash_table, viewable);

      g_signal_emit (view, view_signals[CONTEXT_ITEM], 0,
                     viewable, insert_data);
    }
}

gboolean
gimp_container_view_item_selected (GimpContainerView *view,
				   GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;
  gboolean                  success;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  success = gimp_container_view_select_item (view, viewable);

  if (success && private->container && private->context)
    {
      GimpContext *context;

      /*  ref and remember the context because private->context may
       *  become NULL by calling gimp_context_set_by_type()
       */
      context = g_object_ref (private->context);

      g_signal_handlers_block_by_func (context,
                                       gimp_container_view_context_changed,
                                       view);

      gimp_context_set_by_type (context,
                                private->container->children_type,
                                GIMP_OBJECT (viewable));

      g_signal_handlers_unblock_by_func (context,
                                         gimp_container_view_context_changed,
                                         view);

      g_object_unref (context);
    }

  return success;
}

void
gimp_container_view_item_activated (GimpContainerView *view,
				    GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_activate_item (view, viewable);
}

void
gimp_container_view_item_context (GimpContainerView *view,
				  GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gimp_container_view_context_item (view, viewable);
}

static void
gimp_container_view_clear_items (GimpContainerView *view)
{
  GIMP_CONTAINER_VIEW_GET_CLASS (view)->clear_items (view);
}

static void
gimp_container_view_real_clear_items (GimpContainerView *view)
{
  GimpContainerViewClass   *view_class = GIMP_CONTAINER_VIEW_GET_CLASS (view);
  GimpContainerViewPrivate *private    = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_hash_table_destroy (private->hash_table);

  private->hash_table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                               NULL,
                                               view_class->insert_data_free);
}

static void
gimp_container_view_add_foreach (GimpViewable      *viewable,
				 GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = GIMP_CONTAINER_VIEW_GET_CLASS (view)->insert_item (view,
								   viewable,
								   -1);

  g_hash_table_insert (private->hash_table, viewable, insert_data);
}

static void
gimp_container_view_add (GimpContainerView *view,
			 GimpViewable      *viewable,
			 GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;
  gint                      index;

  if (gimp_container_frozen (container))
    return;

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  insert_data = GIMP_CONTAINER_VIEW_GET_CLASS (view)->insert_item (view,
								   viewable,
								   index);

  g_hash_table_insert (private->hash_table, viewable, insert_data);
}

static void
gimp_container_view_remove (GimpContainerView *view,
			    GimpViewable      *viewable,
			    GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  if (gimp_container_frozen (container))
    return;

  insert_data = g_hash_table_lookup (private->hash_table, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_CLASS (view)->remove_item (view,
							 viewable,
							 insert_data);

      g_hash_table_remove (private->hash_table, viewable);
    }
}

static void
gimp_container_view_reorder (GimpContainerView *view,
			     GimpViewable      *viewable,
			     gint               new_index,
			     GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  if (gimp_container_frozen (container))
    return;

  insert_data = g_hash_table_lookup (private->hash_table, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_CLASS (view)->reorder_item (view,
							  viewable,
							  new_index,
							  insert_data);
    }
}

static void
gimp_container_view_freeze (GimpContainerView *view,
                            GimpContainer     *container)
{
  gimp_container_view_clear_items (view);
}

static void
gimp_container_view_thaw (GimpContainerView *view,
                          GimpContainer     *container)
{
  GimpContext *context = gimp_container_view_get_context (view);

  gimp_container_foreach (container,
                          (GFunc) gimp_container_view_add_foreach,
                          view);

  if (context)
    {
      GimpObject *object;

      object = gimp_context_get_by_type (context,
                                         container->children_type);

      gimp_container_view_select_item (view, (GimpViewable *) object);
    }
}

static void
gimp_container_view_name_changed (GimpViewable      *viewable,
                                  GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->hash_table, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_CLASS (view)->rename_item (view,
                                                         viewable,
							 insert_data);
    }
}

static void
gimp_container_view_context_changed (GimpContext       *context,
				     GimpViewable      *viewable,
				     GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;
  gboolean                  success = FALSE;

  insert_data = g_hash_table_lookup (private->hash_table, viewable);

  g_signal_emit (view, view_signals[SELECT_ITEM], 0,
                 viewable, insert_data, &success);

  if (! success)
    g_warning ("gimp_container_view_context_changed(): select_item() failed "
               "(should not happen)");
}

static void
gimp_container_view_viewable_dropped (GtkWidget    *widget,
				      GimpViewable *viewable,
				      gpointer      data)
{
  GimpContainerView *view      = GIMP_CONTAINER_VIEW (data);
  GimpContainer     *container = gimp_container_view_get_container (view);
  GimpContext       *context   = gimp_container_view_get_context (view);

  gimp_context_set_by_type (context, container->children_type,
                            GIMP_OBJECT (viewable));
}

static void
gimp_container_view_button_viewable_dropped (GtkWidget    *widget,
					     GimpViewable *viewable,
					     gpointer      data)
{
  GimpContainerView *view      = GIMP_CONTAINER_VIEW (data);
  GimpContainer     *container = gimp_container_view_get_container (view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gimp_container_view_item_selected (view, viewable);

      gtk_button_clicked (GTK_BUTTON (widget));
    }
}
