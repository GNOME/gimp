/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerview.c
 * Copyright (C) 2001-2010 Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmamarshal.h"
#include "core/ligmatreehandler.h"
#include "core/ligmaviewable.h"

#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmaviewrenderer.h"
#include "ligmauimanager.h"
#include "ligmacontainertreeview.h"


enum
{
  SELECT_ITEMS,
  ACTIVATE_ITEM,
  LAST_SIGNAL
};


#define LIGMA_CONTAINER_VIEW_GET_PRIVATE(obj) (ligma_container_view_get_private ((LigmaContainerView *) (obj)))


typedef struct _LigmaContainerViewPrivate LigmaContainerViewPrivate;

struct _LigmaContainerViewPrivate
{
  LigmaContainer   *container;
  LigmaContext     *context;

  GHashTable      *item_hash;

  gint             view_size;
  gint             view_border_width;
  gboolean         reorderable;
  GtkSelectionMode selection_mode;

  /*  initialized by subclass  */
  GtkWidget       *dnd_widget;

  LigmaTreeHandler *name_changed_handler;
  LigmaTreeHandler *expanded_changed_handler;
};


/*  local function prototypes  */

static LigmaContainerViewPrivate *
              ligma_container_view_get_private        (LigmaContainerView *view);

static void   ligma_container_view_real_set_container (LigmaContainerView *view,
                                                      LigmaContainer     *container);
static void   ligma_container_view_real_set_context   (LigmaContainerView *view,
                                                      LigmaContext       *context);
static void   ligma_container_view_real_set_selection_mode (LigmaContainerView *view,
                                                           GtkSelectionMode   mode);

static void   ligma_container_view_clear_items      (LigmaContainerView  *view);
static void   ligma_container_view_real_clear_items (LigmaContainerView  *view);

static gint  ligma_container_view_real_get_selected (LigmaContainerView  *view,
                                                    GList             **list,
                                                    GList             **paths);

static void   ligma_container_view_add_container    (LigmaContainerView  *view,
                                                    LigmaContainer      *container);
static void   ligma_container_view_add_foreach      (LigmaViewable       *viewable,
                                                    LigmaContainerView  *view);
static void   ligma_container_view_add              (LigmaContainerView  *view,
                                                    LigmaViewable       *viewable,
                                                    LigmaContainer      *container);

static void   ligma_container_view_remove_container (LigmaContainerView  *view,
                                                    LigmaContainer      *container);
static void   ligma_container_view_remove_foreach   (LigmaViewable       *viewable,
                                                    LigmaContainerView  *view);
static void   ligma_container_view_remove           (LigmaContainerView  *view,
                                                    LigmaViewable       *viewable,
                                                    LigmaContainer      *container);

static void   ligma_container_view_reorder          (LigmaContainerView  *view,
                                                    LigmaViewable       *viewable,
                                                    gint                new_index,
                                                    LigmaContainer      *container);

static void   ligma_container_view_freeze           (LigmaContainerView  *view,
                                                    LigmaContainer      *container);
static void   ligma_container_view_thaw             (LigmaContainerView  *view,
                                                    LigmaContainer      *container);
static void   ligma_container_view_name_changed     (LigmaViewable       *viewable,
                                                    LigmaContainerView  *view);
static void   ligma_container_view_expanded_changed (LigmaViewable       *viewable,
                                                    LigmaContainerView  *view);

static void   ligma_container_view_connect_context    (LigmaContainerView *view);
static void   ligma_container_view_disconnect_context (LigmaContainerView *view);

static void   ligma_container_view_context_changed  (LigmaContext        *context,
                                                    LigmaViewable       *viewable,
                                                    LigmaContainerView  *view);
static void   ligma_container_view_viewable_dropped (GtkWidget          *widget,
                                                    gint                x,
                                                    gint                y,
                                                    LigmaViewable       *viewable,
                                                    gpointer            data);
static void ligma_container_view_button_viewables_dropped (GtkWidget *widget,
                                                          gint        x,
                                                          gint        y,
                                                          GList      *viewables,
                                                          gpointer    data);
static void  ligma_container_view_button_viewable_dropped (GtkWidget    *widget,
                                                          gint          x,
                                                          gint          y,
                                                          LigmaViewable *viewable,
                                                          gpointer      data);


G_DEFINE_INTERFACE (LigmaContainerView, ligma_container_view, GTK_TYPE_WIDGET)


static guint view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_container_view_default_init (LigmaContainerViewInterface *iface)
{
  view_signals[SELECT_ITEMS] =
    g_signal_new ("select-items",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaContainerViewInterface, select_items),
                  NULL, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 2,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER);

  view_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate-item",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContainerViewInterface, activate_item),
                  NULL, NULL,
                  ligma_marshal_VOID__OBJECT_POINTER,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_OBJECT,
                  G_TYPE_POINTER);

  iface->select_items       = NULL;
  iface->activate_item      = NULL;

  iface->set_container      = ligma_container_view_real_set_container;
  iface->set_context        = ligma_container_view_real_set_context;
  iface->set_selection_mode = ligma_container_view_real_set_selection_mode;
  iface->insert_item        = NULL;
  iface->insert_items_after = NULL;
  iface->remove_item        = NULL;
  iface->reorder_item       = NULL;
  iface->rename_item        = NULL;
  iface->expand_item        = NULL;
  iface->clear_items        = ligma_container_view_real_clear_items;
  iface->set_view_size      = NULL;
  iface->get_selected       = ligma_container_view_real_get_selected;

  iface->insert_data_free   = NULL;
  iface->model_is_tree      = FALSE;

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("container",
                                                            NULL, NULL,
                                                            LIGMA_TYPE_CONTAINER,
                                                            LIGMA_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            LIGMA_TYPE_CONTEXT,
                                                            LIGMA_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("selection-mode",
                                                          NULL, NULL,
                                                          GTK_TYPE_SELECTION_MODE,
                                                          GTK_SELECTION_SINGLE,
                                                          LIGMA_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("reorderable",
                                                             NULL, NULL,
                                                             FALSE,
                                                             LIGMA_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-size",
                                                         NULL, NULL,
                                                         1, LIGMA_VIEWABLE_MAX_PREVIEW_SIZE,
                                                         LIGMA_VIEW_SIZE_MEDIUM,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-border-width",
                                                         NULL, NULL,
                                                         0,
                                                         LIGMA_VIEW_MAX_BORDER_WIDTH,
                                                         1,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_container_view_private_dispose (LigmaContainerView        *view,
                                     LigmaContainerViewPrivate *private)
{
  if (private->container)
    ligma_container_view_set_container (view, NULL);

  if (private->context)
    ligma_container_view_set_context (view, NULL);
}

static void
ligma_container_view_private_finalize (LigmaContainerViewPrivate *private)
{
  if (private->item_hash)
    {
      g_hash_table_destroy (private->item_hash);
      private->item_hash = NULL;
    }
  g_clear_pointer (&private->name_changed_handler,
                   ligma_tree_handler_disconnect);
  g_clear_pointer (&private->expanded_changed_handler,
                   ligma_tree_handler_disconnect);

  g_slice_free (LigmaContainerViewPrivate, private);
}

/**
 * ligma_container_view_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #LigmaContainerView. A #LigmaContainerViewProp property is installed
 * for each property, using the values from the #LigmaContainerViewProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %LIGMA_CONTAINER_VIEW_PROP_LAST is good for).
 **/
void
ligma_container_view_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_CONTAINER,
                                    "container");
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_CONTEXT,
                                    "context");
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_SELECTION_MODE,
                                    "selection-mode");
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_REORDERABLE,
                                    "reorderable");
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE,
                                    "view-size");
  g_object_class_override_property (klass,
                                    LIGMA_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
                                    "view-border-width");
}

LigmaContainer *
ligma_container_view_get_container (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), NULL);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->container;
}

void
ligma_container_view_set_container (LigmaContainerView *view,
                                   LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (container == NULL || LIGMA_IS_CONTAINER (container));
  if (container)
    g_return_if_fail (g_type_is_a (ligma_container_get_children_type (container),
                                   LIGMA_TYPE_VIEWABLE));

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (container != private->container)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->set_container (view, container);

      g_object_notify (G_OBJECT (view), "container");
    }
}

LigmaContext *
ligma_container_view_get_context (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), NULL);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->context;
}

void
ligma_container_view_set_context (LigmaContainerView *view,
                                 LigmaContext       *context)
{
  LigmaContainerViewPrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (context != private->context)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->set_context (view, context);

      g_object_notify (G_OBJECT (view), "context");
    }
}

GtkSelectionMode
ligma_container_view_get_selection_mode (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->selection_mode;
}

void
ligma_container_view_set_selection_mode (LigmaContainerView *view,
                                        GtkSelectionMode   mode)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (mode == GTK_SELECTION_SINGLE ||
                    mode == GTK_SELECTION_MULTIPLE);

  LIGMA_CONTAINER_VIEW_GET_IFACE (view)->set_selection_mode (view, mode);
}

gint
ligma_container_view_get_view_size (LigmaContainerView *view,
                                   gint              *view_border_width)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), 0);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (view_border_width)
    *view_border_width = private->view_border_width;

  return private->view_size;
}

void
ligma_container_view_set_view_size (LigmaContainerView *view,
                                   gint               view_size,
                                   gint               view_border_width)
{
  LigmaContainerViewPrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (view_size >  0 &&
                    view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->view_size         != view_size ||
      private->view_border_width != view_border_width)
    {
      private->view_size         = view_size;
      private->view_border_width = view_border_width;

      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->set_view_size (view);

      g_object_freeze_notify (G_OBJECT (view));
      g_object_notify (G_OBJECT (view), "view-size");
      g_object_notify (G_OBJECT (view), "view-border-width");
      g_object_thaw_notify (G_OBJECT (view));
    }
}

gboolean
ligma_container_view_get_reorderable (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->reorderable;
}

void
ligma_container_view_set_reorderable (LigmaContainerView *view,
                                     gboolean           reorderable)
{
  LigmaContainerViewPrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  private->reorderable = reorderable ? TRUE : FALSE;
  g_object_notify (G_OBJECT (view), "reorderable");
}

GtkWidget *
ligma_container_view_get_dnd_widget (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), NULL);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->dnd_widget;
}

void
ligma_container_view_set_dnd_widget (LigmaContainerView *view,
                                    GtkWidget         *dnd_widget)
{
  LigmaContainerViewPrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (dnd_widget == NULL || GTK_IS_WIDGET (dnd_widget));

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  private->dnd_widget = dnd_widget;
}

void
ligma_container_view_enable_dnd (LigmaContainerView *view,
                                GtkButton         *button,
                                GType              children_type)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GTK_IS_BUTTON (button));

  ligma_dnd_viewable_list_dest_add (GTK_WIDGET (button),
                                   children_type,
                                   ligma_container_view_button_viewables_dropped,
                                   view);
  ligma_dnd_viewable_dest_add (GTK_WIDGET (button),
                              children_type,
                              ligma_container_view_button_viewable_dropped,
                              view);
}

gboolean
ligma_container_view_select_items (LigmaContainerView *view,
                                  GList             *viewables)
{
  LigmaContainerViewPrivate *private;
  gboolean                  success = FALSE;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (ligma_container_frozen (private->container))
    return TRUE;

  g_signal_emit (view, view_signals[SELECT_ITEMS], 0,
                 viewables, NULL, &success);

  return success;
}

/* Mostly a convenience function calling the more generic
 * ligma_container_view_select_items().
 * This is to be used when you want to select one viewable only (or
 * because the container this is called from only handles single
 * selection anyway).
 */
gboolean
ligma_container_view_select_item (LigmaContainerView *view,
                                 LigmaViewable      *viewable)
{
  GList    *viewables = NULL;
  gboolean  success;

  if (viewable)
    viewables = g_list_prepend (viewables, viewable);

  success = ligma_container_view_select_items (view, viewables);

  g_list_free (viewables);

  return success;
}

void
ligma_container_view_activate_item (LigmaContainerView *view,
                                   LigmaViewable      *viewable)
{
  LigmaContainerViewPrivate *private;
  gpointer                  insert_data;

  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (LIGMA_IS_VIEWABLE (viewable));

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (ligma_container_frozen (private->container))
    return;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  g_signal_emit (view, view_signals[ACTIVATE_ITEM], 0,
                 viewable, insert_data);
}

gpointer
ligma_container_view_lookup (LigmaContainerView *view,
                            LigmaViewable      *viewable)
{
  LigmaContainerViewPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), NULL);
  g_return_val_if_fail (viewable == NULL || LIGMA_IS_VIEWABLE (viewable), NULL);

  /*  we handle the NULL viewable here as a workaround for bug #149906 */
  if (! viewable)
    return NULL;

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  return g_hash_table_lookup (private->item_hash, viewable);
}

gboolean
ligma_container_view_contains (LigmaContainerView *view,
                              GList             *viewables)
{
  LigmaContainerViewPrivate *private;
  GList                    *iter;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (viewables, FALSE);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  for (iter = viewables; iter; iter = iter->next)
    {
      if (! g_hash_table_contains (private->item_hash, iter->data))
        return FALSE;
    }

  return TRUE;
}

gboolean
ligma_container_view_item_selected (LigmaContainerView *view,
                                   LigmaViewable      *viewable)
{
  LigmaContainerViewPrivate *private;
  gboolean                  success;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (LIGMA_IS_VIEWABLE (viewable), FALSE);

  private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  /* HACK */
  if (private->container && private->context)
    {
      GType        children_type;
      const gchar *signal_name;

      children_type = ligma_container_get_children_type (private->container);
      signal_name   = ligma_context_type_to_signal_name (children_type);

      if (signal_name)
        {
          ligma_context_set_by_type (private->context, children_type,
                                    LIGMA_OBJECT (viewable));
          return TRUE;
        }
    }

  success = ligma_container_view_select_item (view, viewable);

#if 0
  if (success && private->container && private->context)
    {
      LigmaContext *context;
      GType        children_type;

      /*  ref and remember the context because private->context may
       *  become NULL by calling ligma_context_set_by_type()
       */
      context       = g_object_ref (private->context);
      children_type = ligma_container_get_children_type (private->container);

      g_signal_handlers_block_by_func (context,
                                       ligma_container_view_context_changed,
                                       view);

      ligma_context_set_by_type (context, children_type, LIGMA_OBJECT (viewable));

      g_signal_handlers_unblock_by_func (context,
                                         ligma_container_view_context_changed,
                                         view);

      g_object_unref (context);
    }
#endif

  return success;
}

gboolean
ligma_container_view_multi_selected (LigmaContainerView *view,
                                    GList             *items,
                                    GList             *items_data)
{
  gboolean success = FALSE;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);

  g_signal_emit (view, view_signals[SELECT_ITEMS], 0,
                 items, items_data, &success);

  return success;
}

/**
 * ligma_container_view_get_selected:
 * @view:
 * @items:
 * @items_data:
 *
 * Get the selected items in @view.
 *
 * If @items is not %NULL, fills it with a newly allocated #GList of the
 * selected items.
 * If @items_data is not %NULL and if the implementing class associates
 * data to its contents, it will be filled with a newly allocated #GList
 * of the same size as @items, or will be %NULL otherwise. It is up to
 * the class to decide what type of data is passed along.
 *
 * Note that by default, the interface only implements some basic single
 * selection. Override select_items() signal to get more complete
 * selection support.
 *
 * Returns: the number of selected items.
 */
gint
ligma_container_view_get_selected (LigmaContainerView  *view,
                                  GList             **items,
                                  GList             **items_data)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), 0);

  return LIGMA_CONTAINER_VIEW_GET_IFACE (view)->get_selected (view, items, items_data);
}

gboolean
ligma_container_view_is_item_selected (LigmaContainerView *view,
                                      LigmaViewable      *viewable)
{
  GList    *items;
  gboolean  found;

  ligma_container_view_get_selected (view, &items, NULL);
  found = (g_list_find (items, viewable) != NULL);

  g_list_free (items);

  return found;
}

void
ligma_container_view_item_activated (LigmaContainerView *view,
                                    LigmaViewable      *viewable)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_VIEW (view));
  g_return_if_fail (LIGMA_IS_VIEWABLE (viewable));

  ligma_container_view_activate_item (view, viewable);
}

void
ligma_container_view_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case LIGMA_CONTAINER_VIEW_PROP_CONTAINER:
      ligma_container_view_set_container (view, g_value_get_object (value));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_CONTEXT:
      ligma_container_view_set_context (view, g_value_get_object (value));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_SELECTION_MODE:
      ligma_container_view_set_selection_mode (view, g_value_get_enum (value));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_REORDERABLE:
      ligma_container_view_set_reorderable (view, g_value_get_boolean (value));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case LIGMA_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size, border;

        size = ligma_container_view_get_view_size (view, &border);

        if (property_id == LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE)
          size = g_value_get_int (value);
        else
          border = g_value_get_int (value);

        ligma_container_view_set_view_size (view, size, border);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
ligma_container_view_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case LIGMA_CONTAINER_VIEW_PROP_CONTAINER:
      g_value_set_object (value, ligma_container_view_get_container (view));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_CONTEXT:
      g_value_set_object (value, ligma_container_view_get_context (view));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_SELECTION_MODE:
      g_value_set_enum (value, ligma_container_view_get_selection_mode (view));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_REORDERABLE:
      g_value_set_boolean (value, ligma_container_view_get_reorderable (view));
      break;
    case LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case LIGMA_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size, border;

        size = ligma_container_view_get_view_size (view, &border);

        if (property_id == LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE)
          g_value_set_int (value, size);
        else
          g_value_set_int (value, border);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  Private functions  */


static LigmaContainerViewPrivate *
ligma_container_view_get_private (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("ligma-container-view-private");

  private = g_object_get_qdata ((GObject *) view, private_key);

  if (! private)
    {
      LigmaContainerViewInterface *view_iface;

      view_iface = LIGMA_CONTAINER_VIEW_GET_IFACE (view);

      private = g_slice_new0 (LigmaContainerViewPrivate);

      private->view_border_width = 1;

      private->item_hash = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  view_iface->insert_data_free);

      g_object_set_qdata_full ((GObject *) view, private_key, private,
                               (GDestroyNotify) ligma_container_view_private_finalize);

      g_signal_connect (view, "destroy",
                        G_CALLBACK (ligma_container_view_private_dispose),
                        private);
    }

  return private;
}

static void
ligma_container_view_real_set_container (LigmaContainerView *view,
                                        LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container)
    {
      if (private->context)
        ligma_container_view_disconnect_context (view);

      ligma_container_view_select_items (view, NULL);

      /* freeze/thaw is only supported for the toplevel container */
      g_signal_handlers_disconnect_by_func (private->container,
                                            ligma_container_view_freeze,
                                            view);
      g_signal_handlers_disconnect_by_func (private->container,
                                            ligma_container_view_thaw,
                                            view);

      if (! ligma_container_frozen (private->container))
        ligma_container_view_remove_container (view, private->container);
    }

  private->container = container;

  if (private->container)
    {
      if (! ligma_container_frozen (private->container))
        ligma_container_view_add_container (view, private->container);

      /* freeze/thaw is only supported for the toplevel container */
      g_signal_connect_object (private->container, "freeze",
                               G_CALLBACK (ligma_container_view_freeze),
                               view,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "thaw",
                               G_CALLBACK (ligma_container_view_thaw),
                               view,
                               G_CONNECT_SWAPPED);

      if (private->context)
        ligma_container_view_connect_context (view);
    }
}

static void
ligma_container_view_real_set_context (LigmaContainerView *view,
                                      LigmaContext       *context)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->context &&
      private->container)
    {
      ligma_container_view_disconnect_context (view);
    }

  g_set_object (&private->context, context);

  if (private->context &&
      private->container)
    {
      ligma_container_view_connect_context (view);
    }
}

static void
ligma_container_view_real_set_selection_mode (LigmaContainerView *view,
                                             GtkSelectionMode   mode)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  private->selection_mode = mode;
}

static void
ligma_container_view_clear_items (LigmaContainerView *view)
{
  LIGMA_CONTAINER_VIEW_GET_IFACE (view)->clear_items (view);
}

static void
ligma_container_view_real_clear_items (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  g_hash_table_remove_all (private->item_hash);
}

static gint
ligma_container_view_real_get_selected (LigmaContainerView  *view,
                                       GList             **items,
                                       GList             **items_data)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  LigmaObject               *object;

  if (items)
    *items = NULL;

  /* In base interface, @items_data just stays NULL. We don't have a
   * concept for it. Classes implementing this interface may want to
   * store and pass data for their items, but they will have to
   * implement themselves which data, and pass through with this
   * parameter.
   */
  if (items_data)
    *items_data = NULL;

  if (! private->container || ! private->context)
    return 0;

  children_type = ligma_container_get_children_type (private->container);
  if (ligma_context_type_to_property (children_type) == -1)
    {
      /* If you experience this warning, it means you should implement
       * your own definition for get_selected() because the default one
       * won't work for you (only made for context properties).
       */
      g_warning ("%s: TODO: implement LigmaContainerViewInterface's get_selected() for type '%s'.\n",
                 G_STRFUNC, g_type_name (G_OBJECT_TYPE (view)));
      return 0;
    }

  object = ligma_context_get_by_type (private->context,
                                     children_type);

  /* Base interface provides the API for multi-selection but only
   * implements single selection. Classes must implement their own
   * multi-selection.
   */
  if (items && object)
    *items = g_list_append (*items, object);

  return object ? 1 : 0;
}

static void
ligma_container_view_add_container (LigmaContainerView *view,
                                   LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  ligma_container_foreach (container,
                          (GFunc) ligma_container_view_add_foreach,
                          view);

  if (container == private->container)
    {
      GType              children_type;
      LigmaViewableClass *viewable_class;

      children_type  = ligma_container_get_children_type (container);
      viewable_class = g_type_class_ref (children_type);

      private->name_changed_handler =
        ligma_tree_handler_connect (container,
                                   viewable_class->name_changed_signal,
                                   G_CALLBACK (ligma_container_view_name_changed),
                                   view);

      if (LIGMA_CONTAINER_VIEW_GET_IFACE (view)->expand_item)
        {
          private->expanded_changed_handler =
            ligma_tree_handler_connect (container,
                                       "expanded-changed",
                                       G_CALLBACK (ligma_container_view_expanded_changed),
                                       view);
        }

      g_type_class_unref (viewable_class);
    }

  g_signal_connect_object (container, "add",
                           G_CALLBACK (ligma_container_view_add),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_container_view_remove),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "reorder",
                           G_CALLBACK (ligma_container_view_reorder),
                           view,
                           G_CONNECT_SWAPPED);
}

static void
ligma_container_view_add_foreach (LigmaViewable      *viewable,
                                 LigmaContainerView *view)
{
  LigmaContainerViewInterface *view_iface;
  LigmaContainerViewPrivate   *private;
  LigmaViewable               *parent;
  LigmaContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;

  view_iface = LIGMA_CONTAINER_VIEW_GET_IFACE (view);
  private    = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  parent = ligma_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, -1);

  g_hash_table_insert (private->item_hash, viewable, insert_data);

  children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_container_view_add_container (view, children);
}

static void
ligma_container_view_add (LigmaContainerView *view,
                         LigmaViewable      *viewable,
                         LigmaContainer     *container)
{
  LigmaContainerViewInterface *view_iface;
  LigmaContainerViewPrivate   *private;
  LigmaViewable               *parent;
  LigmaContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;
  gint                        index;

  view_iface = LIGMA_CONTAINER_VIEW_GET_IFACE (view);
  private    = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  index = ligma_container_get_child_index (container,
                                          LIGMA_OBJECT (viewable));

  parent = ligma_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  g_hash_table_insert (private->item_hash, viewable, insert_data);

  if (view_iface->insert_items_after)
    view_iface->insert_items_after (view);

  children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_container_view_add_container (view, children);
}

static void
ligma_container_view_remove_container (LigmaContainerView *view,
                                      LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  g_object_ref (container);

  g_signal_handlers_disconnect_by_func (container,
                                        ligma_container_view_add,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        ligma_container_view_remove,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        ligma_container_view_reorder,
                                        view);

  if (container == private->container)
    {
      g_clear_pointer (&private->name_changed_handler,
                       ligma_tree_handler_disconnect);
      g_clear_pointer (&private->expanded_changed_handler,
                       ligma_tree_handler_disconnect);

      /* optimization: when the toplevel container gets removed, call
       * clear_items() which will get rid of all view widget stuff
       * *and* empty private->item_hash, so below call to
       * remove_foreach() will only disconnect all containers but not
       * remove all items individually (because they are gone from
       * item_hash).
       */
      ligma_container_view_clear_items (view);
    }

  ligma_container_foreach (container,
                          (GFunc) ligma_container_view_remove_foreach,
                          view);

  g_object_unref (container);
}

static void
ligma_container_view_remove_foreach (LigmaViewable      *viewable,
                                    LigmaContainerView *view)
{
  ligma_container_view_remove (view, viewable, NULL);
}

static void
ligma_container_view_remove (LigmaContainerView *view,
                            LigmaViewable      *viewable,
                            LigmaContainer     *unused)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  LigmaContainer            *children;
  gpointer                  insert_data;

  children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_container_view_remove_container (view, children);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->remove_item (view,
                                                             viewable,
                                                             insert_data);

      g_hash_table_remove (private->item_hash, viewable);
    }
}

static void
ligma_container_view_reorder (LigmaContainerView *view,
                             LigmaViewable      *viewable,
                             gint               new_index,
                             LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->reorder_item (view,
                                                              viewable,
                                                              new_index,
                                                              insert_data);
    }
}

static void
ligma_container_view_freeze (LigmaContainerView *view,
                            LigmaContainer     *container)
{
  ligma_container_view_remove_container (view, container);
}

static void
ligma_container_view_thaw (LigmaContainerView *view,
                          LigmaContainer     *container)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  ligma_container_view_add_container (view, container);

  if (private->context)
    {
      GType        children_type;
      const gchar *signal_name;

      children_type = ligma_container_get_children_type (private->container);
      signal_name   = ligma_context_type_to_signal_name (children_type);

      if (signal_name)
        {
          LigmaObject *object;

          object = ligma_context_get_by_type (private->context, children_type);

          ligma_container_view_select_item (view, LIGMA_VIEWABLE (object));
        }
    }
}

static void
ligma_container_view_name_changed (LigmaViewable      *viewable,
                                  LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->rename_item (view,
                                                             viewable,
                                                             insert_data);
    }
}

static void
ligma_container_view_expanded_changed (LigmaViewable      *viewable,
                                      LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      LIGMA_CONTAINER_VIEW_GET_IFACE (view)->expand_item (view,
                                                             viewable,
                                                             insert_data);
    }
}

static void
ligma_container_view_connect_context (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  const gchar              *signal_name;

  children_type = ligma_container_get_children_type (private->container);
  signal_name   = ligma_context_type_to_signal_name (children_type);

  if (signal_name)
    {
      g_signal_connect_object (private->context, signal_name,
                               G_CALLBACK (ligma_container_view_context_changed),
                               view,
                               0);

      if (private->dnd_widget)
        ligma_dnd_viewable_dest_add (private->dnd_widget,
                                    children_type,
                                    ligma_container_view_viewable_dropped,
                                    view);

      if (! ligma_container_frozen (private->container))
        {
          LigmaObject *object = ligma_context_get_by_type (private->context,
                                                         children_type);

          ligma_container_view_select_item (view, LIGMA_VIEWABLE (object));
        }
    }
}

static void
ligma_container_view_disconnect_context (LigmaContainerView *view)
{
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  const gchar              *signal_name;

  children_type = ligma_container_get_children_type (private->container);
  signal_name   = ligma_context_type_to_signal_name (children_type);

  if (signal_name)
    {
      g_signal_handlers_disconnect_by_func (private->context,
                                            ligma_container_view_context_changed,
                                            view);

      if (private->dnd_widget)
        {
          gtk_drag_dest_unset (private->dnd_widget);
          ligma_dnd_viewable_dest_remove (private->dnd_widget,
                                         children_type);
        }
    }
}

static void
ligma_container_view_context_changed (LigmaContext       *context,
                                     LigmaViewable      *viewable,
                                     LigmaContainerView *view)
{
  GList *viewables = NULL;

  if (viewable)
    viewables = g_list_prepend (viewables, viewable);

  g_signal_handlers_block_by_func (context,
                                   ligma_container_view_context_changed,
                                   view);

  if (! ligma_container_view_select_items (view, viewables))
    g_warning ("%s: select_items() failed (should not happen)", G_STRFUNC);

  g_signal_handlers_unblock_by_func (context,
                                     ligma_container_view_context_changed,
                                     view);

  g_list_free (viewables);
}

static void
ligma_container_view_viewable_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      LigmaViewable *viewable,
                                      gpointer      data)
{
  LigmaContainerView        *view    = LIGMA_CONTAINER_VIEW (data);
  LigmaContainerViewPrivate *private = LIGMA_CONTAINER_VIEW_GET_PRIVATE (view);

  if (viewable && private->container &&
      ligma_container_have (private->container, LIGMA_OBJECT (viewable)))
    {
      ligma_container_view_item_selected (view, viewable);
    }
}

static void
ligma_container_view_button_viewables_dropped (GtkWidget *widget,
                                              gint        x,
                                              gint        y,
                                              GList      *viewables,
                                              gpointer    data)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (data);

  if (viewables)
    {
      ligma_container_view_multi_selected (view, viewables, NULL);

      gtk_button_clicked (GTK_BUTTON (widget));
    }
}

static void
ligma_container_view_button_viewable_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             LigmaViewable *viewable,
                                             gpointer      data)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (data);

  if (viewable && ligma_container_view_lookup (view, viewable))
    {
      ligma_container_view_item_selected (view, viewable);

      gtk_button_clicked (GTK_BUTTON (widget));
    }
}
