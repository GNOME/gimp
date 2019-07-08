/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpcontainerview.h"
#include "gimpeditor.h"
#include "gimpimageview.h"
#include "gimpdnd.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


static void   gimp_image_view_activate_item (GimpContainerEditor *editor,
                                             GimpViewable        *viewable);


G_DEFINE_TYPE (GimpImageView, gimp_image_view, GIMP_TYPE_CONTAINER_EDITOR)

#define parent_class gimp_image_view_parent_class


static void
gimp_image_view_class_init (GimpImageViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_image_view_activate_item;
}

static void
gimp_image_view_init (GimpImageView *view)
{
  view->raise_button  = NULL;
  view->new_button    = NULL;
  view->delete_button = NULL;
}

GtkWidget *
gimp_image_view_new (GimpViewType     view_type,
                     GimpContainer   *container,
                     GimpContext     *context,
                     gint             view_size,
                     gint             view_border_width,
                     GimpMenuFactory *menu_factory)
{
  GimpImageView       *image_view;
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

  image_view = g_object_new (GIMP_TYPE_IMAGE_VIEW,
                             "view-type",         view_type,
                             "container",         container,
                             "context",           context,
                             "view-size",         view_size,
                             "view-border-width", view_border_width,
                             "menu-factory",      menu_factory,
                             "menu-identifier",   "<Images>",
                             "ui-path",           "/images-popup",
                             NULL);

  editor = GIMP_CONTAINER_EDITOR (image_view);

  image_view->raise_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "images",
                                   "images-raise-views", NULL);

  image_view->new_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "images",
                                   "images-new-view", NULL);

  image_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "images",
                                   "images-delete", NULL);

  if (view_type == GIMP_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = gimp_container_view_get_dnd_widget (editor->view);

      gimp_dnd_xds_source_add (dnd_widget,
                               (GimpDndDragViewableFunc) gimp_dnd_get_drag_data,
                               NULL);
    }

  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->raise_button),
                                  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->new_button),
                                  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->delete_button),
                                  GIMP_TYPE_IMAGE);

  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (image_view);
}

static void
gimp_image_view_activate_item (GimpContainerEditor *editor,
                               GimpViewable        *viewable)
{
  GimpImageView *view = GIMP_IMAGE_VIEW (editor);
  GimpContainer *container;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->raise_button));
    }
}
