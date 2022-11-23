/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatemplateview.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmatemplate.h"

#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmamenufactory.h"
#include "ligmatemplateview.h"
#include "ligmadnd.h"
#include "ligmahelp-ids.h"
#include "ligmaviewrenderer.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


static void   ligma_template_view_activate_item (LigmaContainerEditor *editor,
                                                LigmaViewable        *viewable);


G_DEFINE_TYPE (LigmaTemplateView, ligma_template_view,
               LIGMA_TYPE_CONTAINER_EDITOR);

#define parent_class ligma_template_view_parent_class


static void
ligma_template_view_class_init (LigmaTemplateViewClass *klass)
{
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = ligma_template_view_activate_item;
}

static void
ligma_template_view_init (LigmaTemplateView *view)
{
  view->create_button    = NULL;
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->edit_button      = NULL;
  view->delete_button    = NULL;
}

GtkWidget *
ligma_template_view_new (LigmaViewType     view_type,
                        LigmaContainer   *container,
                        LigmaContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        LigmaMenuFactory *menu_factory)
{
  LigmaTemplateView    *template_view;
  LigmaContainerEditor *editor;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  template_view = g_object_new (LIGMA_TYPE_TEMPLATE_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Templates>",
                                "ui-path",           "/templates-popup",
                                NULL);

  editor = LIGMA_CONTAINER_EDITOR (template_view);

  if (LIGMA_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      LigmaContainerTreeView *tree_view;

      tree_view = LIGMA_CONTAINER_TREE_VIEW (editor->view);

      ligma_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (ligma_container_tree_view_name_edited),
                                                    tree_view);
    }

  template_view->create_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "templates",
                                   "templates-create-image", NULL);

  template_view->new_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "templates",
                                   "templates-new", NULL);

  template_view->duplicate_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "templates",
                                   "templates-duplicate", NULL);

  template_view->edit_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "templates",
                                   "templates-edit", NULL);

  template_view->delete_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "templates",
                                   "templates-delete", NULL);

  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->create_button),
                                  LIGMA_TYPE_TEMPLATE);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->duplicate_button),
                                  LIGMA_TYPE_TEMPLATE);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->edit_button),
                                  LIGMA_TYPE_TEMPLATE);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->delete_button),
                                  LIGMA_TYPE_TEMPLATE);

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (template_view);
}

static void
ligma_template_view_activate_item (LigmaContainerEditor *editor,
                                  LigmaViewable        *viewable)
{
  LigmaTemplateView *view = LIGMA_TEMPLATE_VIEW (editor);
  LigmaContainer    *container;

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = ligma_container_view_get_container (editor->view);

  if (viewable && ligma_container_have (container, LIGMA_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->create_button));
    }
}
