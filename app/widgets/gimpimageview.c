/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "ligmacontainerview.h"
#include "ligmaeditor.h"
#include "ligmaimageview.h"
#include "ligmadnd.h"
#include "ligmamenufactory.h"
#include "ligmauimanager.h"
#include "ligmaviewrenderer.h"

#include "ligma-intl.h"


static void   ligma_image_view_activate_item (LigmaContainerEditor *editor,
                                             LigmaViewable        *viewable);


G_DEFINE_TYPE (LigmaImageView, ligma_image_view, LIGMA_TYPE_CONTAINER_EDITOR)

#define parent_class ligma_image_view_parent_class


static void
ligma_image_view_class_init (LigmaImageViewClass *klass)
{
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = ligma_image_view_activate_item;
}

static void
ligma_image_view_init (LigmaImageView *view)
{
  view->raise_button  = NULL;
  view->new_button    = NULL;
  view->delete_button = NULL;
}

GtkWidget *
ligma_image_view_new (LigmaViewType     view_type,
                     LigmaContainer   *container,
                     LigmaContext     *context,
                     gint             view_size,
                     gint             view_border_width,
                     LigmaMenuFactory *menu_factory)
{
  LigmaImageView       *image_view;
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

  image_view = g_object_new (LIGMA_TYPE_IMAGE_VIEW,
                             "view-type",         view_type,
                             "container",         container,
                             "context",           context,
                             "view-size",         view_size,
                             "view-border-width", view_border_width,
                             "menu-factory",      menu_factory,
                             "menu-identifier",   "<Images>",
                             "ui-path",           "/images-popup",
                             NULL);

  editor = LIGMA_CONTAINER_EDITOR (image_view);

  image_view->raise_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "images",
                                   "images-raise-views", NULL);

  image_view->new_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "images",
                                   "images-new-view", NULL);

  image_view->delete_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor->view), "images",
                                   "images-delete", NULL);

  if (view_type == LIGMA_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = ligma_container_view_get_dnd_widget (editor->view);

      ligma_dnd_xds_source_add (dnd_widget,
                               (LigmaDndDragViewableFunc) ligma_dnd_get_drag_viewable,
                               NULL);
    }

  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->raise_button),
                                  LIGMA_TYPE_IMAGE);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->new_button),
                                  LIGMA_TYPE_IMAGE);
  ligma_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->delete_button),
                                  LIGMA_TYPE_IMAGE);

  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (image_view);
}

static void
ligma_image_view_activate_item (LigmaContainerEditor *editor,
                               LigmaViewable        *viewable)
{
  LigmaImageView *view = LIGMA_IMAGE_VIEW (editor);
  LigmaContainer *container;

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = ligma_container_view_get_container (editor->view);

  if (viewable && ligma_container_have (container, LIGMA_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->raise_button));
    }
}
