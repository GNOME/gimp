/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.c
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimptreehandler.h"
#include "core/gimpviewable.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-cruft.h"
#include "gimpcontainerview-private.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"


enum
{
  SELECTION_CHANGED,
  ITEM_ACTIVATED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_container_view_real_selection_changed
                                                     (GimpContainerView *view);
static void   gimp_container_view_real_item_activated
                                                     (GimpContainerView *view,
                                                      GimpViewable      *item);
static void   gimp_container_view_real_set_container (GimpContainerView *view,
                                                      GimpContainer     *container);
static void   gimp_container_view_real_set_context   (GimpContainerView *view,
                                                      GimpContext       *context);
static void   gimp_container_view_real_set_selection_mode
                                                     (GimpContainerView *view,
                                                      GtkSelectionMode   mode);
static gboolean
              gimp_container_view_real_set_selected  (GimpContainerView  *view,
                                                      GList              *list);
static gint   gimp_container_view_real_get_selected  (GimpContainerView  *view,
                                                      GList             **list);

static void   gimp_container_view_container_freeze   (GimpContainerView  *view,
                                                      GimpContainer      *container);
static void   gimp_container_view_container_thaw     (GimpContainerView  *view,
                                                      GimpContainer      *container);

static void   gimp_container_view_connect_context    (GimpContainerView *view);
static void   gimp_container_view_disconnect_context (GimpContainerView *view);
static void   gimp_container_view_context_changed    (GimpContext        *context,
                                                      GimpViewable       *viewable,
                                                      GimpContainerView  *view);

static void   gimp_container_view_viewable_dropped   (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      GimpViewable       *viewable,
                                                      gpointer            data);
static void   gimp_container_view_button_viewables_dropped
                                                     (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      GList              *viewables,
                                                      gpointer            data);
static void   gimp_container_view_button_viewable_dropped
                                                     (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      GimpViewable       *viewable,
                                                      gpointer            data);


G_DEFINE_INTERFACE (GimpContainerView, gimp_container_view, GTK_TYPE_WIDGET)


static guint view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_container_view_default_init (GimpContainerViewInterface *iface)
{
  view_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerViewInterface, selection_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  view_signals[ITEM_ACTIVATED] =
    g_signal_new ("item-activated",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerViewInterface, item_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  iface->selection_changed  = gimp_container_view_real_selection_changed;
  iface->item_activated     = gimp_container_view_real_item_activated;

  iface->set_container      = gimp_container_view_real_set_container;
  iface->set_context        = gimp_container_view_real_set_context;
  iface->set_selection_mode = gimp_container_view_real_set_selection_mode;
  iface->set_view_size      = NULL;
  iface->set_selected       = gimp_container_view_real_set_selected;
  iface->get_selected       = gimp_container_view_real_get_selected;

  iface->insert_item        = NULL;
  iface->insert_items_after = NULL;
  iface->remove_item        = NULL;
  iface->reorder_item       = NULL;
  iface->rename_item        = NULL;
  iface->expand_item        = NULL;
  iface->clear_items        = _gimp_container_view_real_clear_items;

  iface->insert_data_free   = NULL;
  iface->model_is_tree      = FALSE;
  iface->use_list_model     = FALSE;

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("container",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTAINER,
                                                            GIMP_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTEXT,
                                                            GIMP_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("selection-mode",
                                                          NULL, NULL,
                                                          GTK_TYPE_SELECTION_MODE,
                                                          GTK_SELECTION_SINGLE,
                                                          GIMP_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("reorderable",
                                                             NULL, NULL,
                                                             FALSE,
                                                             GIMP_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-size",
                                                         NULL, NULL,
                                                         1, GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                                                         GIMP_VIEW_SIZE_MEDIUM,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-border-width",
                                                         NULL, NULL,
                                                         0,
                                                         GIMP_VIEW_MAX_BORDER_WIDTH,
                                                         1,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}


/*  public functions  */

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
  g_return_if_fail (container == NULL ||
                    g_type_is_a (gimp_container_get_child_type (container),
                                 GIMP_TYPE_VIEWABLE));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (container != private->container)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->set_container (view, container);

      g_object_notify (G_OBJECT (view), "container");
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
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (context != private->context)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->set_context (view, context);

      g_object_notify (G_OBJECT (view), "context");
    }
}

GtkSelectionMode
gimp_container_view_get_selection_mode (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->selection_mode;
}

void
gimp_container_view_set_selection_mode (GimpContainerView *view,
                                        GtkSelectionMode   mode)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (mode == GTK_SELECTION_SINGLE ||
                    mode == GTK_SELECTION_MULTIPLE);

  GIMP_CONTAINER_VIEW_GET_IFACE (view)->set_selection_mode (view, mode);
}

gint
gimp_container_view_get_view_size (GimpContainerView *view,
                                   gint              *view_border_width)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), 0);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (view_border_width)
    *view_border_width = private->view_border_width;

  return private->view_size;
}

void
gimp_container_view_set_view_size (GimpContainerView *view,
                                   gint               view_size,
                                   gint               view_border_width)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->view_size         != view_size ||
      private->view_border_width != view_border_width)
    {
      private->view_size         = view_size;
      private->view_border_width = view_border_width;

      GIMP_CONTAINER_VIEW_GET_IFACE (view)->set_view_size (view);

      g_object_freeze_notify (G_OBJECT (view));
      g_object_notify (G_OBJECT (view), "view-size");
      g_object_notify (G_OBJECT (view), "view-border-width");
      g_object_thaw_notify (G_OBJECT (view));
    }
}

gboolean
gimp_container_view_get_reorderable (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->reorderable;
}

void
gimp_container_view_set_reorderable (GimpContainerView *view,
                                     gboolean           reorderable)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  private->reorderable = reorderable ? TRUE : FALSE;
  g_object_notify (G_OBJECT (view), "reorderable");
}

GtkWidget *
gimp_container_view_get_dnd_widget (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->dnd_widget;
}

void
gimp_container_view_set_dnd_widget (GimpContainerView *view,
                                    GtkWidget         *dnd_widget)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (dnd_widget == NULL || GTK_IS_WIDGET (dnd_widget));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  private->dnd_widget = dnd_widget;
}

void
gimp_container_view_enable_dnd (GimpContainerView *view,
                                GtkButton         *button,
                                GType              child_type)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GTK_IS_BUTTON (button));

  gimp_dnd_viewable_list_dest_add (GTK_WIDGET (button),
                                   child_type,
                                   gimp_container_view_button_viewables_dropped,
                                   view);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (button),
                              child_type,
                              gimp_container_view_button_viewable_dropped,
                              view);
}

gboolean
gimp_container_view_set_selected (GimpContainerView *view,
                                  GList             *viewables)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (gimp_container_frozen (private->container))
    return FALSE;

  return GIMP_CONTAINER_VIEW_GET_IFACE (view)->set_selected (view, viewables);
}

gboolean
gimp_container_view_set_1_selected (GimpContainerView *view,
                                    GimpViewable      *viewable)
{
  GList    *viewables = NULL;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);

  if (viewable)
    viewables = g_list_prepend (viewables, viewable);

  success = gimp_container_view_set_selected (view, viewables);

  g_list_free (viewables);

  return success;
}

/**
 * gimp_container_view_get_selected:
 * @view:
 * @items:
 *
 * Get the selected items in @view.
 *
 * If @items is not %NULL, fills it with a newly allocated #GList of the
 * selected items.
 *
 * Note that by default, the interface only implements some basic single
 * selection. Override select_items() signal to get more complete
 * selection support.
 *
 * Returns: the number of selected items.
 */
gint
gimp_container_view_get_selected (GimpContainerView  *view,
                                  GList             **items)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), 0);

  if (items)
    *items = NULL;

  return GIMP_CONTAINER_VIEW_GET_IFACE (view)->get_selected (view, items);
}

GimpViewable *
gimp_container_view_get_1_selected (GimpContainerView *view)
{
  GimpViewable *item = NULL;
  GList        *items;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  if (gimp_container_view_get_selected (view, &items) == 1)
    item = items->data;

  g_list_free (items);

  return item;
}

gboolean
gimp_container_view_is_item_selected (GimpContainerView *view,
                                      GimpViewable      *viewable)
{
  GList    *items;
  gboolean  found;

  gimp_container_view_get_selected (view, &items);
  found = (g_list_find (items, viewable) != NULL);

  g_list_free (items);

  return found;
}


/*  protected functions, only to be used by implementors  */

void
_gimp_container_view_selection_changed (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (! gimp_container_frozen (private->container))
    g_signal_emit (view, view_signals[SELECTION_CHANGED], 0);
}

void
_gimp_container_view_item_activated (GimpContainerView *view,
                                     GimpViewable      *viewable)
{
  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (view, view_signals[ITEM_ACTIVATED], 0,
                 viewable);
}

/**
 * gimp_container_view_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpContainerView. A #GimpContainerViewProp property is installed
 * for each property, using the values from the #GimpContainerViewProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %GIMP_CONTAINER_VIEW_PROP_LAST is good for).
 **/
void
_gimp_container_view_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_CONTAINER,
                                    "container");
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_CONTEXT,
                                    "context");
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_SELECTION_MODE,
                                    "selection-mode");
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_REORDERABLE,
                                    "reorderable");
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE,
                                    "view-size");
  g_object_class_override_property (klass,
                                    GIMP_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
                                    "view-border-width");
}

void
_gimp_container_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case GIMP_CONTAINER_VIEW_PROP_CONTAINER:
      gimp_container_view_set_container (view, g_value_get_object (value));
      break;
    case GIMP_CONTAINER_VIEW_PROP_CONTEXT:
      gimp_container_view_set_context (view, g_value_get_object (value));
      break;
    case GIMP_CONTAINER_VIEW_PROP_SELECTION_MODE:
      gimp_container_view_set_selection_mode (view, g_value_get_enum (value));
      break;
    case GIMP_CONTAINER_VIEW_PROP_REORDERABLE:
      gimp_container_view_set_reorderable (view, g_value_get_boolean (value));
      break;
    case GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case GIMP_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = gimp_container_view_get_view_size (view, &border);

        if (property_id == GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE)
          size = g_value_get_int (value);
        else
          border = g_value_get_int (value);

        gimp_container_view_set_view_size (view, size, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
_gimp_container_view_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case GIMP_CONTAINER_VIEW_PROP_CONTAINER:
      g_value_set_object (value, gimp_container_view_get_container (view));
      break;
    case GIMP_CONTAINER_VIEW_PROP_CONTEXT:
      g_value_set_object (value, gimp_container_view_get_context (view));
      break;
    case GIMP_CONTAINER_VIEW_PROP_SELECTION_MODE:
      g_value_set_enum (value, gimp_container_view_get_selection_mode (view));
      break;
    case GIMP_CONTAINER_VIEW_PROP_REORDERABLE:
      g_value_set_boolean (value, gimp_container_view_get_reorderable (view));
      break;
    case GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case GIMP_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = gimp_container_view_get_view_size (view, &border);

        if (property_id == GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE)
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

static void
gimp_container_view_real_selection_changed (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container && private->context)
    {
      GType child_type = gimp_container_get_child_type (private->container);

      if (gimp_context_type_to_signal_name (child_type))
        {
          GList *items = NULL;
          gint   n_items;

          n_items = gimp_container_view_get_selected (view, &items);

          if (n_items == 1 &&
              gimp_context_get_by_type (private->context, child_type) !=
              GIMP_OBJECT (items->data))
            {
              GimpContext *context;

              /*  ref and remember the context because
               *  private->context may become NULL by calling
               *  gimp_context_set_by_type()
               */
              context = g_object_ref (private->context);

              g_signal_handlers_block_by_func (context,
                                               gimp_container_view_context_changed,
                                               view);

              gimp_context_set_by_type (context, child_type, items->data);

              g_signal_handlers_unblock_by_func (context,
                                                 gimp_container_view_context_changed,
                                                 view);

              g_object_unref (context);
            }

          g_list_free (items);
        }
    }
}

static void
gimp_container_view_real_item_activated (GimpContainerView *view,
                                         GimpViewable      *item)
{
}

static void
gimp_container_view_real_set_container (GimpContainerView *view,
                                        GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container)
    {
      if (private->context)
        gimp_container_view_disconnect_context (view);

      g_signal_handlers_disconnect_by_func (private->container,
                                            gimp_container_view_container_freeze,
                                            view);
      g_signal_handlers_disconnect_by_func (private->container,
                                            gimp_container_view_container_thaw,
                                            view);

      if (! GIMP_CONTAINER_VIEW_GET_IFACE (view)->use_list_model)
        _gimp_container_view_disconnect_cruft (view);
    }

  private->container = container;

  if (private->container)
    {
      if (! GIMP_CONTAINER_VIEW_GET_IFACE (view)->use_list_model)
        _gimp_container_view_connect_cruft (view);

      g_signal_connect_object (private->container, "freeze",
                               G_CALLBACK (gimp_container_view_container_freeze),
                               view,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "thaw",
                               G_CALLBACK (gimp_container_view_container_thaw),
                               view,
                               G_CONNECT_SWAPPED);

      if (private->context)
        gimp_container_view_connect_context (view);
    }
}

static void
gimp_container_view_real_set_context (GimpContainerView *view,
                                      GimpContext       *context)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->context && private->container)
    {
      gimp_container_view_disconnect_context (view);
    }

  g_set_object (&private->context, context);

  if (private->context && private->container)
    {
      gimp_container_view_connect_context (view);
    }
}

static void
gimp_container_view_real_set_selection_mode (GimpContainerView *view,
                                             GtkSelectionMode   mode)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  private->selection_mode = mode;
}

static gboolean
gimp_container_view_real_set_selected (GimpContainerView *view,
                                       GList             *items)
{
  return FALSE;
}

static gint
gimp_container_view_real_get_selected (GimpContainerView  *view,
                                       GList             **items)
{
  if (*items)
    *items = NULL;

  return 0;
}

static void
gimp_container_view_container_freeze (GimpContainerView *view,
                                      GimpContainer     *container)
{
}

static void
gimp_container_view_container_thaw (GimpContainerView *view,
                                    GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->context)
    {
      GType        child_type;
      const gchar *signal_name;

      child_type  = gimp_container_get_child_type (private->container);
      signal_name = gimp_context_type_to_signal_name (child_type);

      if (signal_name)
        {
          GimpObject *object;

          object = gimp_context_get_by_type (private->context, child_type);

          gimp_container_view_set_1_selected (view, GIMP_VIEWABLE (object));
        }
    }
}

static void
gimp_container_view_connect_context (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     child_type;
  const gchar              *signal_name;

  child_type  = gimp_container_get_child_type (private->container);
  signal_name = gimp_context_type_to_signal_name (child_type);

  if (signal_name)
    {
      g_signal_connect_object (private->context, signal_name,
                               G_CALLBACK (gimp_container_view_context_changed),
                               view,
                               0);

      if (private->dnd_widget)
        gimp_dnd_viewable_dest_add (private->dnd_widget,
                                    child_type,
                                    gimp_container_view_viewable_dropped,
                                    view);

      if (! gimp_container_frozen (private->container))
        {
          GimpObject *object = gimp_context_get_by_type (private->context,
                                                         child_type);

          gimp_container_view_set_1_selected (view, GIMP_VIEWABLE (object));
        }
    }
}

static void
gimp_container_view_disconnect_context (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     child_type;
  const gchar              *signal_name;

  child_type  = gimp_container_get_child_type (private->container);
  signal_name = gimp_context_type_to_signal_name (child_type);

  if (signal_name)
    {
      g_signal_handlers_disconnect_by_func (private->context,
                                            gimp_container_view_context_changed,
                                            view);

      if (private->dnd_widget)
        {
          gtk_drag_dest_unset (private->dnd_widget);
          gimp_dnd_viewable_dest_remove (private->dnd_widget,
                                         child_type);
        }
    }
}

static void
gimp_container_view_context_changed (GimpContext       *context,
                                     GimpViewable      *viewable,
                                     GimpContainerView *view)
{
  gimp_container_view_set_1_selected (view, viewable);
}

static void
gimp_container_view_viewable_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      GimpViewable *viewable,
                                      gpointer      data)
{
  GimpContainerView        *view    = GIMP_CONTAINER_VIEW (data);
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (viewable && private->container &&
      gimp_container_have (private->container, GIMP_OBJECT (viewable)))
    {
      gimp_container_view_set_1_selected (view, viewable);
    }
}

static void
gimp_container_view_button_viewables_dropped (GtkWidget *widget,
                                              gint        x,
                                              gint        y,
                                              GList      *viewables,
                                              gpointer    data)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (data);

  if (viewables && gimp_container_view_set_selected (view, viewables))
    {
      gtk_button_clicked (GTK_BUTTON (widget));
    }
}

static void
gimp_container_view_button_viewable_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             GimpViewable *viewable,
                                             gpointer      data)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (data);

  if (viewable && gimp_container_view_set_1_selected (view, viewable))
    {
      gtk_button_clicked (GTK_BUTTON (widget));
    }
}
