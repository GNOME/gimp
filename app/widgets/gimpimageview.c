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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpcontainerview.h"
#include "gimpimageview.h"
#include "gimpdnd.h"

#include "gimp-intl.h"


static void   gimp_image_view_class_init           (GimpImageViewClass  *klass);
static void   gimp_image_view_init                 (GimpImageView       *view);

static void   gimp_image_view_raise_clicked        (GtkWidget           *widget,
                                                    GimpImageView       *view);
static void   gimp_image_view_new_clicked          (GtkWidget           *widget,
                                                    GimpImageView       *view);
static void   gimp_image_view_delete_clicked       (GtkWidget           *widget,
                                                    GimpImageView       *view);

static void   gimp_image_view_select_item          (GimpContainerEditor *editor,
                                                    GimpViewable        *viewable);
static void   gimp_image_view_activate_item        (GimpContainerEditor *editor,
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
  GimpContainerEditorClass *editor_class;

  editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_image_view_select_item;
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
                                         TRUE, /* reorderable */
                                         menu_factory, "<Images>"))
    {
      g_object_unref (image_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (image_view);

  image_view->raise_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_GOTO_TOP,
                            _("Raise this image's displays"),
                            NULL,
                            G_CALLBACK (gimp_image_view_raise_clicked),
                            NULL,
                            editor);

  image_view->new_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_NEW,
                            _("Create a new display for this image"),
                            NULL,
                            G_CALLBACK (gimp_image_view_new_clicked),
                            NULL,
                            editor);

  image_view->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_DELETE,
                            _("Delete this image"), NULL,
                            G_CALLBACK (gimp_image_view_delete_clicked),
                            NULL,
                            editor);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor, (GimpViewable *) gimp_context_get_image (context));

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->raise_button),
				  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->new_button),
				  GIMP_TYPE_IMAGE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (image_view->delete_button),
				  GIMP_TYPE_IMAGE);

  return GTK_WIDGET (image_view);
}

static void
gimp_image_view_raise_clicked (GtkWidget     *widget,
                               GimpImageView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpImage           *image;

  image = gimp_context_get_image (editor->view->context);

  if (image && gimp_container_have (editor->view->container,
                                    GIMP_OBJECT (image)))
    {
      if (view->raise_displays_func)
        view->raise_displays_func (image);
    }
}

static void
gimp_image_view_new_clicked (GtkWidget     *widget,
                             GimpImageView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpImage           *image;

  image = gimp_context_get_image (editor->view->context);

  if (image && gimp_container_have (editor->view->container,
                                    GIMP_OBJECT (image)))
    {
      gimp_create_display (image->gimp, image, 0x0101);
    }
}

static void
gimp_image_view_delete_clicked (GtkWidget     *widget,
                                GimpImageView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpImage           *image;

  image = gimp_context_get_image (editor->view->context);

  if (image && gimp_container_have (editor->view->container,
                                    GIMP_OBJECT (image)))
    {
      if (image->disp_count == 0)
        g_object_unref (image);
    }
}

static void
gimp_image_view_select_item (GimpContainerEditor *editor,
                             GimpViewable        *viewable)
{
  GimpImageView *view = GIMP_IMAGE_VIEW (editor);

  gboolean  raise_sensitive  = FALSE;
  gboolean  new_sensitive    = FALSE;
  gboolean  delete_sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      raise_sensitive = TRUE;
      new_sensitive   = TRUE;

      if (GIMP_IMAGE (viewable)->disp_count == 0)
        delete_sensitive = TRUE;
    }

  gtk_widget_set_sensitive (view->raise_button,  raise_sensitive);
  gtk_widget_set_sensitive (view->new_button,    new_sensitive);
  gtk_widget_set_sensitive (view->delete_button, delete_sensitive);
}

static void
gimp_image_view_activate_item (GimpContainerEditor *editor,
                               GimpViewable        *viewable)
{
  GimpImageView *view;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  view = GIMP_IMAGE_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->raise_button));
    }
}
