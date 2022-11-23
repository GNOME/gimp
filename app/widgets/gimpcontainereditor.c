/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainereditor.c
 * Copyright (C) 2001-2011 Michael Natterer <mitch@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmaasyncset.h"
#include "core/ligmacontext.h"
#include "core/ligmalist.h"
#include "core/ligmaviewable.h"

#include "ligmacontainereditor.h"
#include "ligmacontainericonview.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmaviewrenderer.h"
#include "ligmauimanager.h"


enum
{
  PROP_0,
  PROP_VIEW_TYPE,
  PROP_CONTAINER,
  PROP_CONTEXT,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH,
  PROP_MENU_FACTORY,
  PROP_MENU_IDENTIFIER,
  PROP_UI_PATH
};


struct _LigmaContainerEditorPrivate
{
  LigmaViewType     view_type;
  LigmaContainer   *container;
  LigmaContext     *context;
  gint             view_size;
  gint             view_border_width;
  LigmaMenuFactory *menu_factory;
  gchar           *menu_identifier;
  gchar           *ui_path;
  GtkWidget       *busy_box;
  GBinding        *async_set_binding;
};


static void  ligma_container_editor_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_container_editor_constructed      (GObject             *object);
static void   ligma_container_editor_dispose          (GObject             *object);
static void   ligma_container_editor_set_property     (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void   ligma_container_editor_get_property     (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static gboolean ligma_container_editor_select_items   (LigmaContainerView   *view,
                                                      GList               *items,
                                                      GList               *paths,
                                                      LigmaContainerEditor *editor);
static void   ligma_container_editor_activate_item    (GtkWidget           *widget,
                                                      LigmaViewable        *viewable,
                                                      gpointer             insert_data,
                                                      LigmaContainerEditor *editor);

static GtkWidget * ligma_container_editor_get_preview (LigmaDocked       *docked,
                                                      LigmaContext      *context,
                                                      GtkIconSize       size);
static void        ligma_container_editor_set_context (LigmaDocked       *docked,
                                                      LigmaContext      *context);
static LigmaUIManager * ligma_container_editor_get_menu(LigmaDocked       *docked,
                                                      const gchar     **ui_path,
                                                      gpointer         *popup_data);

static gboolean  ligma_container_editor_has_button_bar      (LigmaDocked *docked);
static void      ligma_container_editor_set_show_button_bar (LigmaDocked *docked,
                                                            gboolean    show);
static gboolean  ligma_container_editor_get_show_button_bar (LigmaDocked *docked);


G_DEFINE_TYPE_WITH_CODE (LigmaContainerEditor, ligma_container_editor,
                         GTK_TYPE_BOX,
                         G_ADD_PRIVATE (LigmaContainerEditor)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_container_editor_docked_iface_init))

#define parent_class ligma_container_editor_parent_class


static void
ligma_container_editor_class_init (LigmaContainerEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed   = ligma_container_editor_constructed;
  object_class->dispose       = ligma_container_editor_dispose;
  object_class->set_property  = ligma_container_editor_set_property;
  object_class->get_property  = ligma_container_editor_get_property;

  klass->select_item     = NULL;
  klass->activate_item   = NULL;

  g_object_class_install_property (object_class, PROP_VIEW_TYPE,
                                   g_param_spec_enum ("view-type",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_VIEW_TYPE,
                                                      LIGMA_VIEW_TYPE_LIST,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_int ("view-size",
                                                     NULL, NULL,
                                                     1, LIGMA_VIEWABLE_MAX_PREVIEW_SIZE,
                                                     LIGMA_VIEW_SIZE_MEDIUM,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_VIEW_BORDER_WIDTH,
                                   g_param_spec_int ("view-border-width",
                                                     NULL, NULL,
                                                     0,
                                                     LIGMA_VIEW_MAX_BORDER_WIDTH,
                                                     1,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_MENU_FACTORY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_IDENTIFIER,
                                   g_param_spec_string ("menu-identifier",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_PATH,
                                   g_param_spec_string ("ui-path",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_container_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  iface->get_preview         = ligma_container_editor_get_preview;
  iface->set_context         = ligma_container_editor_set_context;
  iface->get_menu            = ligma_container_editor_get_menu;
  iface->has_button_bar      = ligma_container_editor_has_button_bar;
  iface->set_show_button_bar = ligma_container_editor_set_show_button_bar;
  iface->get_show_button_bar = ligma_container_editor_get_show_button_bar;
}

static void
ligma_container_editor_init (LigmaContainerEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  editor->priv = ligma_container_editor_get_instance_private (editor);
}

static void
ligma_container_editor_constructed (GObject *object)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_CONTAINER (editor->priv->container));
  ligma_assert (LIGMA_IS_CONTEXT (editor->priv->context));

  switch (editor->priv->view_type)
    {
    case LIGMA_VIEW_TYPE_GRID:
      editor->view =
        LIGMA_CONTAINER_VIEW (ligma_container_icon_view_new (editor->priv->container,
                                                           editor->priv->context,
                                                           editor->priv->view_size,
                                                           editor->priv->view_border_width));
      break;

    case LIGMA_VIEW_TYPE_LIST:
      editor->view =
        LIGMA_CONTAINER_VIEW (ligma_container_tree_view_new (editor->priv->container,
                                                           editor->priv->context,
                                                           editor->priv->view_size,
                                                           editor->priv->view_border_width));
      break;

    default:
      ligma_assert_not_reached ();
    }

  if (LIGMA_IS_LIST (editor->priv->container))
    ligma_container_view_set_reorderable (LIGMA_CONTAINER_VIEW (editor->view),
                                         ! ligma_list_get_sort_func (LIGMA_LIST (editor->priv->container)));

  if (editor->priv->menu_factory    &&
      editor->priv->menu_identifier &&
      editor->priv->ui_path)
    {
      ligma_editor_create_menu (LIGMA_EDITOR (editor->view),
                               editor->priv->menu_factory,
                               editor->priv->menu_identifier,
                               editor->priv->ui_path,
                               editor);
    }

  gtk_box_pack_start (GTK_BOX (editor), GTK_WIDGET (editor->view),
                      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->view));

  editor->priv->busy_box = ligma_busy_box_new (NULL);
  gtk_box_pack_start (GTK_BOX (editor), editor->priv->busy_box, TRUE, TRUE, 0);

  g_object_bind_property (editor->priv->busy_box, "visible",
                          editor->view,           "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  /*  Connect "select-items" with G_CONNECT_AFTER because it's a
   *  RUN_LAST signal and the default handler selecting the row must
   *  run before signal connections. See bug #784176.
   */
  g_signal_connect_object (editor->view, "select-items",
                           G_CALLBACK (ligma_container_editor_select_items),
                           editor, G_CONNECT_AFTER);

  g_signal_connect_object (editor->view, "activate-item",
                           G_CALLBACK (ligma_container_editor_activate_item),
                           editor, 0);
  /* g_signal_connect_object (editor->view, "context-item", XXX maybe listen to popup-menu? */
  /*                          G_CALLBACK (ligma_container_editor_context_item), */
  /*                          editor, 0); */

  {
    GList      *objects = NULL;
    LigmaObject *object  = ligma_context_get_by_type (editor->priv->context,
                                                    ligma_container_get_children_type (editor->priv->container));

    if (object)
      objects = g_list_prepend (objects, object);

    ligma_container_editor_select_items (editor->view, objects, NULL, editor);

    g_list_free (objects);
  }
}

static void
ligma_container_editor_dispose (GObject *object)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (object);

  ligma_container_editor_bind_to_async_set (editor, NULL, NULL);

  g_clear_object (&editor->priv->container);
  g_clear_object (&editor->priv->context);
  g_clear_object (&editor->priv->menu_factory);

  g_clear_pointer (&editor->priv->menu_identifier, g_free);
  g_clear_pointer (&editor->priv->ui_path,         g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_container_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      editor->priv->view_type = g_value_get_enum (value);
      break;

    case PROP_CONTAINER:
      editor->priv->container = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      editor->priv->context = g_value_dup_object (value);
      break;

    case PROP_VIEW_SIZE:
      editor->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      editor->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_MENU_FACTORY:
      editor->priv->menu_factory = g_value_dup_object (value);
      break;

    case PROP_MENU_IDENTIFIER:
      editor->priv->menu_identifier = g_value_dup_string (value);
      break;

    case PROP_UI_PATH:
      editor->priv->ui_path = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_container_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      g_value_set_enum (value, editor->priv->view_type);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, editor->priv->container);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, editor->priv->context);
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, editor->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, editor->priv->view_border_width);
      break;

    case PROP_MENU_FACTORY:
      g_value_set_object (value, editor->priv->menu_factory);
      break;

    case PROP_MENU_IDENTIFIER:
      g_value_set_string (value, editor->priv->menu_identifier);
      break;

    case PROP_UI_PATH:
      g_value_set_string (value, editor->priv->ui_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkSelectionMode
ligma_container_editor_get_selection_mode (LigmaContainerEditor *editor)
{
  return ligma_container_view_get_selection_mode (LIGMA_CONTAINER_VIEW (editor->view));
}

void
ligma_container_editor_set_selection_mode (LigmaContainerEditor *editor,
                                          GtkSelectionMode     mode)
{
  ligma_container_view_set_selection_mode (LIGMA_CONTAINER_VIEW (editor->view),
                                          mode);
}

/*  private functions  */

static gboolean
ligma_container_editor_select_items (LigmaContainerView   *view,
                                    GList               *items,
                                    GList               *paths,
                                    LigmaContainerEditor *editor)
{
  LigmaContainerEditorClass *klass    = LIGMA_CONTAINER_EDITOR_GET_CLASS (editor);
  LigmaViewable             *viewable = NULL;

  /* XXX Right now a LigmaContainerEditor only supports 1 item selected
   * at once. Let's see later if we want to allow more.
   */
  /*g_return_val_if_fail (g_list_length (items) < 2, FALSE);*/

  if (items)
    viewable = items->data;

  if (klass->select_item)
    klass->select_item (editor, viewable);

  if (editor->priv->container)
    {
      const gchar *signal_name;
      GType        children_type;

      children_type = ligma_container_get_children_type (editor->priv->container);
      signal_name   = ligma_context_type_to_signal_name (children_type);

      if (signal_name)
        ligma_context_set_by_type (editor->priv->context, children_type,
                                  LIGMA_OBJECT (viewable));
    }

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor->view)));

  return TRUE;
}

static void
ligma_container_editor_activate_item (GtkWidget           *widget,
                                     LigmaViewable        *viewable,
                                     gpointer             insert_data,
                                     LigmaContainerEditor *editor)
{
  LigmaContainerEditorClass *klass = LIGMA_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->activate_item)
    klass->activate_item (editor, viewable);
}

static GtkWidget *
ligma_container_editor_get_preview (LigmaDocked   *docked,
                                   LigmaContext  *context,
                                   GtkIconSize   size)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  return ligma_docked_get_preview (LIGMA_DOCKED (editor->view),
                                  context, size);
}

static void
ligma_container_editor_set_context (LigmaDocked  *docked,
                                   LigmaContext *context)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  ligma_docked_set_context (LIGMA_DOCKED (editor->view), context);
}

static LigmaUIManager *
ligma_container_editor_get_menu (LigmaDocked   *docked,
                                const gchar **ui_path,
                                gpointer     *popup_data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  return ligma_docked_get_menu (LIGMA_DOCKED (editor->view), ui_path, popup_data);
}

static gboolean
ligma_container_editor_has_button_bar (LigmaDocked *docked)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  return ligma_docked_has_button_bar (LIGMA_DOCKED (editor->view));
}

static void
ligma_container_editor_set_show_button_bar (LigmaDocked *docked,
                                           gboolean    show)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  ligma_docked_set_show_button_bar (LIGMA_DOCKED (editor->view), show);
}

static gboolean
ligma_container_editor_get_show_button_bar (LigmaDocked *docked)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (docked);

  return ligma_docked_get_show_button_bar (LIGMA_DOCKED (editor->view));
}

void
ligma_container_editor_bind_to_async_set (LigmaContainerEditor *editor,
                                         LigmaAsyncSet        *async_set,
                                         const gchar         *message)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_EDITOR (editor));
  g_return_if_fail (async_set == NULL || LIGMA_IS_ASYNC_SET (async_set));
  g_return_if_fail (async_set == NULL || message != NULL);

  if (! async_set && ! editor->priv->async_set_binding)
    return;

  g_clear_object (&editor->priv->async_set_binding);

  if (async_set)
    {
      ligma_busy_box_set_message (LIGMA_BUSY_BOX (editor->priv->busy_box),
                                 message);

      editor->priv->async_set_binding = g_object_bind_property (
        async_set,              "empty",
        editor->priv->busy_box, "visible",
        G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
    }
  else
    {
      gtk_widget_hide (editor->priv->busy_box);
    }
}
