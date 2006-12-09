/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "gimpcontainereditor.h"
#include "gimpcontainergridview.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"


static void  gimp_container_editor_docked_iface_init (GimpDockedInterface *iface);

static gboolean gimp_container_editor_select_item    (GtkWidget           *widget,
                                                      GimpViewable        *viewable,
                                                      gpointer             insert_data,
                                                      GimpContainerEditor *editor);
static void   gimp_container_editor_activate_item    (GtkWidget           *widget,
                                                      GimpViewable        *viewable,
                                                      gpointer             insert_data,
                                                      GimpContainerEditor *editor);
static void   gimp_container_editor_context_item     (GtkWidget           *widget,
                                                      GimpViewable        *viewable,
                                                      gpointer             insert_data,
                                                      GimpContainerEditor *editor);
static void   gimp_container_editor_real_context_item(GimpContainerEditor *editor,
                                                      GimpViewable        *viewable);

static GtkWidget * gimp_container_editor_get_preview (GimpDocked       *docked,
                                                      GimpContext      *context,
                                                      GtkIconSize       size);
static void        gimp_container_editor_set_context (GimpDocked       *docked,
                                                      GimpContext      *context);
static GimpUIManager * gimp_container_editor_get_menu(GimpDocked       *docked,
                                                      const gchar     **ui_path,
                                                      gpointer         *popup_data);

static gboolean  gimp_container_editor_has_button_bar      (GimpDocked *docked);
static void      gimp_container_editor_set_show_button_bar (GimpDocked *docked,
                                                            gboolean    show);
static gboolean  gimp_container_editor_get_show_button_bar (GimpDocked *docked);


G_DEFINE_TYPE_WITH_CODE (GimpContainerEditor, gimp_container_editor,
                         GTK_TYPE_VBOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_container_editor_docked_iface_init))

#define parent_class gimp_container_editor_parent_class


static void
gimp_container_editor_class_init (GimpContainerEditorClass *klass)
{
  klass->select_item     = NULL;
  klass->activate_item   = NULL;
  klass->context_item    = gimp_container_editor_real_context_item;
}

static void
gimp_container_editor_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_preview         = gimp_container_editor_get_preview;
  iface->set_context         = gimp_container_editor_set_context;
  iface->get_menu            = gimp_container_editor_get_menu;
  iface->has_button_bar      = gimp_container_editor_has_button_bar;
  iface->set_show_button_bar = gimp_container_editor_set_show_button_bar;
  iface->get_show_button_bar = gimp_container_editor_get_show_button_bar;
}

static void
gimp_container_editor_init (GimpContainerEditor *view)
{
  view->view = NULL;
}

gboolean
gimp_container_editor_construct (GimpContainerEditor *editor,
                                 GimpViewType         view_type,
                                 GimpContainer       *container,
                                 GimpContext         *context,
                                 gint                 view_size,
                                 gint                 view_border_width,
                                 GimpMenuFactory     *menu_factory,
                                 const gchar         *menu_identifier,
                                 const gchar         *ui_identifier)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_EDITOR (editor), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), FALSE);

  switch (view_type)
    {
    case GIMP_VIEW_TYPE_GRID:
      editor->view =
        GIMP_CONTAINER_VIEW (gimp_container_grid_view_new (container,
                                                           context,
                                                           view_size,
                                                           view_border_width));
      break;

    case GIMP_VIEW_TYPE_LIST:
      editor->view =
        GIMP_CONTAINER_VIEW (gimp_container_tree_view_new (container,
                                                           context,
                                                           view_size,
                                                           view_border_width));
      break;

    default:
      g_warning ("%s: unknown GimpViewType passed", G_STRFUNC);
      return FALSE;
    }

  if (GIMP_IS_LIST (container))
    gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                         ! GIMP_LIST (container)->sort_func);

  if (menu_factory && menu_identifier && ui_identifier)
    gimp_editor_create_menu (GIMP_EDITOR (editor->view),
                             menu_factory, menu_identifier, ui_identifier,
                             editor);

  gtk_container_add (GTK_CONTAINER (editor), GTK_WIDGET (editor->view));
  gtk_widget_show (GTK_WIDGET (editor->view));

  g_signal_connect_object (editor->view, "select-item",
                           G_CALLBACK (gimp_container_editor_select_item),
                           editor, 0);
  g_signal_connect_object (editor->view, "activate-item",
                           G_CALLBACK (gimp_container_editor_activate_item),
                           editor, 0);
  g_signal_connect_object (editor->view, "context-item",
                           G_CALLBACK (gimp_container_editor_context_item),
                           editor, 0);

  {
    GimpObject *object = gimp_context_get_by_type (context,
                                                   container->children_type);

    gimp_container_editor_select_item (GTK_WIDGET (editor->view),
                                       (GimpViewable *) object, NULL,
                                       editor);
  }

  return TRUE;
}


/*  private functions  */

static gboolean
gimp_container_editor_select_item (GtkWidget           *widget,
                                   GimpViewable        *viewable,
                                   gpointer             insert_data,
                                   GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->select_item)
    klass->select_item (editor, viewable);

  if (GIMP_EDITOR (editor->view)->ui_manager)
    gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager,
                            GIMP_EDITOR (editor->view)->popup_data);

  return TRUE;
}

static void
gimp_container_editor_activate_item (GtkWidget           *widget,
                                     GimpViewable        *viewable,
                                     gpointer             insert_data,
                                     GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->activate_item)
    klass->activate_item (editor, viewable);
}

static void
gimp_container_editor_context_item (GtkWidget           *widget,
                                    GimpViewable        *viewable,
                                    gpointer             insert_data,
                                    GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->context_item)
    klass->context_item (editor, viewable);
}

static void
gimp_container_editor_real_context_item (GimpContainerEditor *editor,
                                         GimpViewable        *viewable)
{
  GimpContainer *container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gimp_editor_popup_menu (GIMP_EDITOR (editor->view), NULL, NULL);
    }
}

static GtkWidget *
gimp_container_editor_get_preview (GimpDocked   *docked,
                                   GimpContext  *context,
                                   GtkIconSize   size)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_preview (GIMP_DOCKED (editor->view),
                                  context, size);
}

static void
gimp_container_editor_set_context (GimpDocked  *docked,
                                   GimpContext *context)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  gimp_docked_set_context (GIMP_DOCKED (editor->view), context);
}

static GimpUIManager *
gimp_container_editor_get_menu (GimpDocked   *docked,
                                const gchar **ui_path,
                                gpointer     *popup_data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_menu (GIMP_DOCKED (editor->view), ui_path, popup_data);
}

static gboolean
gimp_container_editor_has_button_bar (GimpDocked *docked)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_has_button_bar (GIMP_DOCKED (editor->view));
}

static void
gimp_container_editor_set_show_button_bar (GimpDocked *docked,
                                           gboolean    show)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (editor->view), show);
}

static gboolean
gimp_container_editor_get_show_button_bar (GimpDocked *docked)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_show_button_bar (GIMP_DOCKED (editor->view));
}
