/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptemplateview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptemplate.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpmenufactory.h"
#include "gimptemplateview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


static void   gimp_template_view_activate_item (GimpContainerEditor *editor,
                                                GimpViewable        *viewable);


G_DEFINE_TYPE (GimpTemplateView, gimp_template_view,
               GIMP_TYPE_CONTAINER_EDITOR);

#define parent_class gimp_template_view_parent_class


static void
gimp_template_view_class_init (GimpTemplateViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_template_view_activate_item;
}

static void
gimp_template_view_init (GimpTemplateView *view)
{
  view->create_button    = NULL;
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->edit_button      = NULL;
  view->delete_button    = NULL;
}

GtkWidget *
gimp_template_view_new (GimpViewType     view_type,
                        GimpContainer   *container,
                        GimpContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        GimpMenuFactory *menu_factory)
{
  GimpTemplateView    *template_view;
  GimpContainerEditor *editor;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  template_view = g_object_new (GIMP_TYPE_TEMPLATE_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Templates>",
                                "ui-path",           "/templates-popup",
                                NULL);

  editor = GIMP_CONTAINER_EDITOR (template_view);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      gimp_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (gimp_container_tree_view_name_edited),
                                                    tree_view);
    }

  template_view->create_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-create-image", NULL);

  template_view->new_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-new", NULL);

  template_view->duplicate_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-duplicate", NULL);

  template_view->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-edit", NULL);

  template_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "templates",
                                   "templates-delete", NULL);

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->create_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->duplicate_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->edit_button),
                                  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->delete_button),
                                  GIMP_TYPE_TEMPLATE);

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (template_view);
}

static void
gimp_template_view_activate_item (GimpContainerEditor *editor,
                                  GimpViewable        *viewable)
{
  GimpTemplateView *view = GIMP_TEMPLATE_VIEW (editor);
  GimpContainer    *container;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->create_button));
    }
}
