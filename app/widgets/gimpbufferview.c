/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbufferview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpcontainerview.h"
#include "gimpbufferview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


static void   gimp_buffer_view_class_init    (GimpBufferViewClass *klass);
static void   gimp_buffer_view_init          (GimpBufferView      *view);

static void   gimp_buffer_view_activate_item (GimpContainerEditor *editor,
                                              GimpViewable        *viewable);


static GimpContainerEditorClass *parent_class = NULL;


GType
gimp_buffer_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpBufferViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_buffer_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpBufferView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_buffer_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpBufferView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_buffer_view_class_init (GimpBufferViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->activate_item = gimp_buffer_view_activate_item;
}

static void
gimp_buffer_view_init (GimpBufferView *view)
{
  view->paste_button        = NULL;
  view->paste_into_button   = NULL;
  view->paste_as_new_button = NULL;
  view->delete_button       = NULL;
}

GtkWidget *
gimp_buffer_view_new (GimpViewType     view_type,
		      GimpContainer   *container,
		      GimpContext     *context,
		      gint             preview_size,
                      gint             preview_border_width,
		      GimpMenuFactory *menu_factory)
{
  GimpBufferView      *buffer_view;
  GimpContainerEditor *editor;

  buffer_view = g_object_new (GIMP_TYPE_BUFFER_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (buffer_view),
                                         view_type,
                                         container,context,
                                         preview_size, preview_border_width,
                                         menu_factory, "<Buffers>",
                                         "/buffers-popup"))
    {
      g_object_unref (buffer_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (buffer_view);

  buffer_view->paste_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste", NULL);

  buffer_view->paste_into_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste-into", NULL);

  buffer_view->paste_as_new_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new", NULL);

  buffer_view->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "buffers",
                                   "buffers-delete", NULL);

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (buffer_view->paste_button),
				  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (buffer_view->paste_into_button),
				  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (buffer_view->paste_as_new_button),
				  GIMP_TYPE_BUFFER);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (buffer_view->delete_button),
				  GIMP_TYPE_BUFFER);

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return GTK_WIDGET (buffer_view);
}

static void
gimp_buffer_view_activate_item (GimpContainerEditor *editor,
				GimpViewable        *viewable)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (editor);
  GimpContainer  *container;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->paste_button));
    }
}
