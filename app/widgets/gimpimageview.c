/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdocumentview.c
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

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpcontainerview.h"
#include "gimpimageview.h"
#include "gimpdnd.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


static void   gimp_image_view_class_init    (GimpImageViewClass  *klass);
static void   gimp_image_view_init          (GimpImageView       *view);

static void   gimp_image_view_activate_item (GimpContainerEditor *editor,
                                             GimpViewable        *viewable);


static GimpContainerEditorClass *parent_class = NULL;


GType
gimp_image_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpImageViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImageView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpImageView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_image_view_class_init (GimpImageViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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
                     gint             preview_size,
                     gint             preview_border_width,
                     GimpMenuFactory *menu_factory)
{
  GimpImageView       *image_view;
  GimpContainerEditor *editor;

  image_view = g_object_new (GIMP_TYPE_IMAGE_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (image_view),
                                         view_type,
                                         container, context,
                                         preview_size, preview_border_width,
                                         menu_factory, "<Images>",
                                         "/images-popup"))
    {
      g_object_unref (image_view);
      return NULL;
    }

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

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->raise_button),
				  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->new_button),
				  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->delete_button),
				  GIMP_TYPE_IMAGE);

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager,
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
