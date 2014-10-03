/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.c
 * Copyright (C) 2001-2010 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimptreehandler.h"
#include "core/gimpviewable.h"

#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"
#include "gimpcontainertreeview.h"


enum
{
  SELECT_ITEM,
  ACTIVATE_ITEM,
  CONTEXT_ITEM,
  LAST_SIGNAL
};


#define GIMP_CONTAINER_VIEW_GET_PRIVATE(obj) (gimp_container_view_get_private ((GimpContainerView *) (obj)))


typedef struct _GimpContainerViewPrivate GimpContainerViewPrivate;

struct _GimpContainerViewPrivate
{
  GimpContainer   *container;
  GimpContext     *context;

  GHashTable      *item_hash;

  gint             view_size;
  gint             view_border_width;
  gboolean         reorderable;
  GtkSelectionMode selection_mode;

  /*  initialized by subclass  */
  GtkWidget       *dnd_widget;

  GimpTreeHandler *name_changed_handler;
};


static void   gimp_container_view_iface_base_init   (GimpContainerViewInterface *view_iface);

static GimpContainerViewPrivate *
              gimp_container_view_get_private        (GimpContainerView *view);

static void   gimp_container_view_real_set_container (GimpContainerView *view,
                                                      GimpContainer     *container);
static void   gimp_container_view_real_set_context   (GimpContainerView *view,
                                                      GimpContext       *context);
static void   gimp_container_view_real_set_selection_mode (GimpContainerView *view,
                                                           GtkSelectionMode   mode);

static void   gimp_container_view_clear_items      (GimpContainerView  *view);
static void   gimp_container_view_real_clear_items (GimpContainerView  *view);

static void   gimp_container_view_add_container    (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_add_foreach      (GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_add              (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    GimpContainer      *container);

static void   gimp_container_view_remove_container (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_remove_foreach   (GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_remove           (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    GimpContainer      *container);

static void   gimp_container_view_reorder          (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    gint                new_index,
                                                    GimpContainer      *container);

static void   gimp_container_view_freeze           (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_thaw             (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_name_changed     (GimpViewable       *viewable,
                                                    GimpContainerView  *view);

static void   gimp_container_view_connect_context    (GimpContainerView *view);
static void   gimp_container_view_disconnect_context (GimpContainerView *view);

static void   gimp_container_view_context_changed  (GimpContext        *context,
                                                    GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_viewable_dropped (GtkWidget          *widget,
                                                    gint                x,
                                                    gint                y,
                                                    GimpViewable       *viewable,
                                                    gpointer            data);
static void  gimp_container_view_button_viewable_dropped (GtkWidget    *widget,
                                                          gint          x,
                                                          gint          y,
                                                          GimpViewable *viewable,
                                                          gpointer      data);
static gint  gimp_container_view_real_get_selected (GimpContainerView    *view,
                                                    GList               **list);


static guint view_signals[LAST_SIGNAL] = { 0 };


GType
gimp_container_view_interface_get_type (void)
{
  static GType iface_type = 0;

  if (! iface_type)
    {
      const GTypeInfo iface_info =
      {
        sizeof (GimpContainerViewInterface),
        (GBaseInitFunc)     gimp_container_view_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpContainerViewInterface",
                                           &iface_info,
                                           0);

      g_type_interface_add_prerequisite (iface_type, GTK_TYPE_WIDGET);
    }

  return iface_type;
}

static void
gimp_container_view_iface_base_init (GimpContainerViewInterface *view_iface)
{
  if (view_iface->set_container)
    return;

  view_signals[SELECT_ITEM] =
    g_signal_new ("select-item",
                  G_TYPE_FROM_INTERFACE (view_iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpContainerViewInterface, select_item),
                  NULL, NULL,
                  gimp_marshal_BOOLEAN__OBJECT_POINTER,
                  G_TYPE_BOOLEAN, 2,
                  GIMP_TYPE_OBJECT,
                  G_TYPE_POINTER);

  view_signals[ACTIVATE_ITEM] =
    g_signal_new ("activate-item",
                  G_TYPE_FROM_INTERFACE (view_iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerViewInterface, activate_item),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT_POINTER,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_OBJECT,
                  G_TYPE_POINTER);

  view_signals[CONTEXT_ITEM] =
    g_signal_new ("context-item",
                  G_TYPE_FROM_INTERFACE (view_iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerViewInterface, context_item),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT_POINTER,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_OBJECT,
                  G_TYPE_POINTER);

  view_iface->select_item        = NULL;
  view_iface->activate_item      = NULL;
  view_iface->context_item       = NULL;

  view_iface->set_container      = gimp_container_view_real_set_container;
  view_iface->set_context        = gimp_container_view_real_set_context;
  view_iface->set_selection_mode = gimp_container_view_real_set_selection_mode;
  view_iface->insert_item        = NULL;
  view_iface->insert_item_after  = NULL;
  view_iface->remove_item        = NULL;
  view_iface->reorder_item       = NULL;
  view_iface->rename_item        = NULL;
  view_iface->clear_items        = gimp_container_view_real_clear_items;
  view_iface->set_view_size      = NULL;
  view_iface->get_selected       = gimp_container_view_real_get_selected;

  view_iface->insert_data_free   = NULL;
  view_iface->model_is_tree      = FALSE;

  g_object_interface_install_property (view_iface,
                                       g_param_spec_object ("container",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTAINER,
                                                            GIMP_PARAM_READWRITE));

  g_object_interface_install_property (view_iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTEXT,
                                                            GIMP_PARAM_READWRITE));

  g_object_interface_install_property (view_iface,
                                       g_param_spec_enum ("selection-mode",
                                                          NULL, NULL,
                                                          GTK_TYPE_SELECTION_MODE,
                                                          GTK_SELECTION_SINGLE,
                                                          GIMP_PARAM_READWRITE));

  g_object_interface_install_property (view_iface,
                                       g_param_spec_boolean ("reorderable",
                                                             NULL, NULL,
                                                             FALSE,
                                                             GIMP_PARAM_READWRITE));

  g_object_interface_install_property (view_iface,
                                       g_param_spec_int ("view-size",
                                                         NULL, NULL,
                                                         1, GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                                                         GIMP_VIEW_SIZE_MEDIUM,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_interface_install_property (view_iface,
                                       g_param_spec_int ("view-border-width",
                                                         NULL, NULL,
                                                         0,
                                                         GIMP_VIEW_MAX_BORDER_WIDTH,
                                                         1,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_container_view_private_dispose (GimpContainerView        *view,
                                     GimpContainerViewPrivate *private)
{
  if (private->container)
    gimp_container_view_set_container (view, NULL);

  if (private->context)
    gimp_container_view_set_context (view, NULL);
}

static void
gimp_container_view_private_finalize (GimpContainerViewPrivate *private)
{
  if (private->item_hash)
    {
      g_hash_table_destroy (private->item_hash);
      private->item_hash = NULL;
    }

  g_slice_free (GimpContainerViewPrivate, private);
}

static GimpContainerViewPrivate *
gimp_container_view_get_private (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-container-view-private");

  private = g_object_get_qdata ((GObject *) view, private_key);

  if (! private)
    {
      GimpContainerViewInterface *view_iface;

      view_iface = GIMP_CONTAINER_VIEW_GET_INTERFACE (view);

      private = g_slice_new0 (GimpContainerViewPrivate);

      private->view_border_width = 1;

      private->item_hash = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  view_iface->insert_data_free);

      g_object_set_qdata_full ((GObject *) view, private_key, private,
                               (GDestroyNotify) gimp_container_view_private_finalize);

      g_signal_connect (view, "destroy",
                        G_CALLBACK (gimp_container_view_private_dispose),
                        private);
    }

  return private;
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
gimp_container_view_install_properties (GObjectClass *klass)
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
  if (container)
    g_return_if_fail (g_type_is_a (gimp_container_get_children_type (container),
                                   GIMP_TYPE_VIEWABLE));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (container != private->container)
    {
      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->set_container (view, container);

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
      if (private->context)
        gimp_container_view_disconnect_context (view);

      gimp_container_view_select_item (view, NULL);

      /* freeze/thaw is only supported for the toplevel container */
      g_signal_handlers_disconnect_by_func (private->container,
                                            gimp_container_view_freeze,
                                            view);
      g_signal_handlers_disconnect_by_func (private->container,
                                            gimp_container_view_thaw,
                                            view);

      if (! gimp_container_frozen (private->container))
        gimp_container_view_remove_container (view, private->container);
    }

  private->container = container;

  if (private->container)
    {
      if (! gimp_container_frozen (private->container))
        gimp_container_view_add_container (view, private->container);

      /* freeze/thaw is only supported for the toplevel container */
      g_signal_connect_object (private->container, "freeze",
                               G_CALLBACK (gimp_container_view_freeze),
                               view,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "thaw",
                               G_CALLBACK (gimp_container_view_thaw),
                               view,
                               G_CONNECT_SWAPPED);

      if (private->context)
        gimp_container_view_connect_context (view);
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
      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->set_context (view, context);

      g_object_notify (G_OBJECT (view), "context");
    }
}

static void
gimp_container_view_real_set_context (GimpContainerView *view,
                                      GimpContext       *context)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->context)
    {
      if (private->container)
        gimp_container_view_disconnect_context (view);

      g_object_unref (private->context);
    }

  private->context = context;

  if (private->context)
    {
      g_object_ref (private->context);

      if (private->container)
        gimp_container_view_connect_context (view);
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

  GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->set_selection_mode (view, mode);
}

static void
gimp_container_view_real_set_selection_mode (GimpContainerView *view,
                                             GtkSelectionMode   mode)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  private->selection_mode = mode;
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

      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->set_view_size (view);

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
  gpointer                  insert_data;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  g_signal_emit (view, view_signals[SELECT_ITEM], 0,
                 viewable, insert_data, &success);

  return success;
}

void
gimp_container_view_activate_item (GimpContainerView *view,
                                   GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;
  gpointer                  insert_data;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  g_signal_emit (view, view_signals[ACTIVATE_ITEM], 0,
                 viewable, insert_data);
}

void
gimp_container_view_context_item (GimpContainerView *view,
                                  GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;
  gpointer                  insert_data;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  g_signal_emit (view, view_signals[CONTEXT_ITEM], 0,
                 viewable, insert_data);
}

gpointer
gimp_container_view_lookup (GimpContainerView *view,
                            GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);
  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), NULL);

  /*  we handle the NULL viewable here as a workaround for bug #149906 */
  if (! viewable)
    return NULL;

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return g_hash_table_lookup (private->item_hash, viewable);
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

  /* HACK */
  if (private->container && private->context)
    {
      GType        children_type;
      const gchar *signal_name;

      children_type = gimp_container_get_children_type (private->container);
      signal_name   = gimp_context_type_to_signal_name (children_type);

      if (signal_name)
        {
          gimp_context_set_by_type (private->context, children_type,
                                    GIMP_OBJECT (viewable));
          return TRUE;
        }
    }

  success = gimp_container_view_select_item (view, viewable);

#if 0
  if (success && private->container && private->context)
    {
      GimpContext *context;
      GType        children_type;

      /*  ref and remember the context because private->context may
       *  become NULL by calling gimp_context_set_by_type()
       */
      context       = g_object_ref (private->context);
      children_type = gimp_container_get_children_type (private->container);

      g_signal_handlers_block_by_func (context,
                                       gimp_container_view_context_changed,
                                       view);

      gimp_context_set_by_type (context, children_type, GIMP_OBJECT (viewable));

      g_signal_handlers_unblock_by_func (context,
                                         gimp_container_view_context_changed,
                                         view);

      g_object_unref (context);
    }
#endif

  return success;
}

gboolean
gimp_container_view_multi_selected (GimpContainerView *view,
                                    GList             *items)
{
  guint                     selected_count;
  gboolean                  success = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);

  selected_count = g_list_length (items);

  if (selected_count == 0)
    {
      /* do nothing */
    }
  else if (selected_count == 1)
    {
      success = gimp_container_view_item_selected (view, items->data);
    }
  else
    {
      success = FALSE;
      g_signal_emit (view, view_signals[SELECT_ITEM], 0,
                     NULL, items, &success);
    }

  return success;
}

gint
gimp_container_view_get_selected (GimpContainerView  *view,
                                  GList             **list)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), 0);

  return GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->get_selected (view, list);
}

static gint
gimp_container_view_real_get_selected (GimpContainerView    *view,
                                       GList               **list)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  GimpObject               *object;

  if (list)
    *list = NULL;

  if (! private->container || ! private->context)
    return 0;

  children_type = gimp_container_get_children_type (private->container);
  object = gimp_context_get_by_type (private->context,
                                     children_type);

  if (list && object)
    *list = g_list_append (*list, object);

  return object ? 1 : 0;
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

void
gimp_container_view_set_property (GObject      *object,
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
        gint size, border;

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
gimp_container_view_get_property (GObject    *object,
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
        gint size, border;

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

static void
gimp_container_view_clear_items (GimpContainerView *view)
{
  GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->clear_items (view);
}

static void
gimp_container_view_real_clear_items (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_hash_table_remove_all (private->item_hash);
}

static void
gimp_container_view_add_container (GimpContainerView *view,
                                   GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  gimp_container_foreach (container,
                          (GFunc) gimp_container_view_add_foreach,
                          view);

  if (container == private->container)
    {
      GType              children_type;
      GimpViewableClass *viewable_class;

      children_type  = gimp_container_get_children_type (container);
      viewable_class = g_type_class_ref (children_type);

      private->name_changed_handler =
        gimp_tree_handler_connect (container,
                                   viewable_class->name_changed_signal,
                                   G_CALLBACK (gimp_container_view_name_changed),
                                   view);

      g_type_class_unref (viewable_class);
    }

  g_signal_connect_object (container, "add",
                           G_CALLBACK (gimp_container_view_add),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_container_view_remove),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "reorder",
                           G_CALLBACK (gimp_container_view_reorder),
                           view,
                           G_CONNECT_SWAPPED);
}

static void
gimp_container_view_add_foreach (GimpViewable      *viewable,
                                 GimpContainerView *view)
{
  GimpContainerViewInterface *view_iface;
  GimpContainerViewPrivate   *private;
  GimpViewable               *parent;
  GimpContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;

  view_iface = GIMP_CONTAINER_VIEW_GET_INTERFACE (view);
  private    = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  parent = gimp_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, -1);

  g_hash_table_insert (private->item_hash, viewable, insert_data);

  if (view_iface->insert_item_after)
    view_iface->insert_item_after (view, viewable, insert_data);

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_add_container (view, children);
}

static void
gimp_container_view_add (GimpContainerView *view,
                         GimpViewable      *viewable,
                         GimpContainer     *container)
{
  GimpContainerViewInterface *view_iface;
  GimpContainerViewPrivate   *private;
  GimpViewable               *parent;
  GimpContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;
  gint                        index;

  view_iface = GIMP_CONTAINER_VIEW_GET_INTERFACE (view);
  private    = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  parent = gimp_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  g_hash_table_insert (private->item_hash, viewable, insert_data);

  if (view_iface->insert_item_after)
    view_iface->insert_item_after (view, viewable, insert_data);

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_add_container (view, children);
}

static void
gimp_container_view_remove_container (GimpContainerView *view,
                                      GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_object_ref (container);

  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_add,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_remove,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_reorder,
                                        view);

  if (container == private->container)
    {
      gimp_tree_handler_disconnect (private->name_changed_handler);
      private->name_changed_handler = NULL;

      /* optimization: when the toplevel container gets removed, call
       * clear_items() which will get rid of all view widget stuff
       * *and* empty private->item_hash, so below call to
       * remove_foreach() will only disconnect all containers but not
       * remove all items individually (because they are gone from
       * item_hash).
       */
      gimp_container_view_clear_items (view);
    }

  gimp_container_foreach (container,
                          (GFunc) gimp_container_view_remove_foreach,
                          view);

  g_object_unref (container);
}

static void
gimp_container_view_remove_foreach (GimpViewable      *viewable,
                                    GimpContainerView *view)
{
  gimp_container_view_remove (view, viewable, NULL);
}

static void
gimp_container_view_remove (GimpContainerView *view,
                            GimpViewable      *viewable,
                            GimpContainer     *unused)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GimpContainer            *children;
  gpointer                  insert_data;

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_remove_container (view, children);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->remove_item (view,
                                                             viewable,
                                                             insert_data);

      g_hash_table_remove (private->item_hash, viewable);
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

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->reorder_item (view,
                                                              viewable,
                                                              new_index,
                                                              insert_data);
    }
}

static void
gimp_container_view_freeze (GimpContainerView *view,
                            GimpContainer     *container)
{
  gimp_container_view_remove_container (view, container);
}

static void
gimp_container_view_thaw (GimpContainerView *view,
                          GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  gimp_container_view_add_container (view, container);

  if (private->context)
    {
      GType        children_type;
      const gchar *signal_name;

      children_type = gimp_container_get_children_type (private->container);
      signal_name   = gimp_context_type_to_signal_name (children_type);

      if (signal_name)
        {
          GimpObject *object;

          object = gimp_context_get_by_type (private->context, children_type);

          gimp_container_view_select_item (view, GIMP_VIEWABLE (object));
        }
    }
}

static void
gimp_container_view_name_changed (GimpViewable      *viewable,
                                  GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_INTERFACE (view)->rename_item (view,
                                                             viewable,
                                                             insert_data);
    }
}

static void
gimp_container_view_connect_context (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  const gchar              *signal_name;

  children_type = gimp_container_get_children_type (private->container);
  signal_name   = gimp_context_type_to_signal_name (children_type);

  if (signal_name)
    {
      g_signal_connect_object (private->context, signal_name,
                               G_CALLBACK (gimp_container_view_context_changed),
                               view,
                               0);

      if (private->dnd_widget)
        gimp_dnd_viewable_dest_add (private->dnd_widget,
                                    children_type,
                                    gimp_container_view_viewable_dropped,
                                    view);

      if (! gimp_container_frozen (private->container))
        {
          GimpObject *object = gimp_context_get_by_type (private->context,
                                                         children_type);

          gimp_container_view_select_item (view, GIMP_VIEWABLE (object));
        }
    }
}

static void
gimp_container_view_disconnect_context (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     children_type;
  const gchar              *signal_name;

  children_type = gimp_container_get_children_type (private->container);
  signal_name   = gimp_context_type_to_signal_name (children_type);

  if (signal_name)
    {
      g_signal_handlers_disconnect_by_func (private->context,
                                            gimp_container_view_context_changed,
                                            view);

      if (private->dnd_widget)
        {
          gtk_drag_dest_unset (private->dnd_widget);
          gimp_dnd_viewable_dest_remove (private->dnd_widget,
                                         children_type);
        }
    }
}

static void
gimp_container_view_context_changed (GimpContext       *context,
                                     GimpViewable      *viewable,
                                     GimpContainerView *view)
{
  if (! gimp_container_view_select_item (view, viewable))
    g_warning ("%s: select_item() failed (should not happen)", G_STRFUNC);
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
      gimp_container_view_item_selected (view, viewable);
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

  if (viewable && gimp_container_view_lookup (view, viewable))
    {
      gimp_container_view_item_selected (view, viewable);

      gtk_button_clicked (GTK_BUTTON (widget));
    }
}

