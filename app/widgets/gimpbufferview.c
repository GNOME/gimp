/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbufferview.c
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

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpcontainerview.h"
#include "gimpbufferview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"

#include "gimp-intl.h"


static void   gimp_buffer_view_class_init (GimpBufferViewClass *klass);
static void   gimp_buffer_view_init       (GimpBufferView      *view);

static void   gimp_buffer_view_paste_clicked        (GtkWidget      *widget,
						     GimpBufferView *view);
static void   gimp_buffer_view_paste_into_clicked   (GtkWidget      *widget,
						     GimpBufferView *view);
static void   gimp_buffer_view_paste_as_new_clicked (GtkWidget      *widget,
						     GimpBufferView *view);
static void   gimp_buffer_view_delete_clicked       (GtkWidget      *widget,
						     GimpBufferView *view);

static void   gimp_buffer_view_select_item          (GimpContainerEditor *editor,
						     GimpViewable        *viewable);
static void   gimp_buffer_view_activate_item        (GimpContainerEditor *editor,
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
  GimpContainerEditorClass *editor_class;

  editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_buffer_view_select_item;
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
                                         TRUE, /* reorderable */
                                         menu_factory, "<Buffers>"))
    {
      g_object_unref (buffer_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (buffer_view);

  buffer_view->paste_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_PASTE, _("Paste"),
                            GIMP_HELP_BUFFER_PASTE,
                            G_CALLBACK (gimp_buffer_view_paste_clicked),
                            NULL,
                            editor);

  buffer_view->paste_into_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_PASTE_INTO, _("Paste Into"),
                            GIMP_HELP_BUFFER_PASTE_INTO,
                            G_CALLBACK (gimp_buffer_view_paste_into_clicked),
                            NULL,
                            editor);

  buffer_view->paste_as_new_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_PASTE_AS_NEW, _("Paste as New"),
                            GIMP_HELP_BUFFER_PASTE_AS_NEW,
                            G_CALLBACK (gimp_buffer_view_paste_as_new_clicked),
                            NULL,
                            editor);

  buffer_view->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_DELETE, _("Delete"),
                            GIMP_HELP_BUFFER_DELETE,
                            G_CALLBACK (gimp_buffer_view_delete_clicked),
                            NULL,
                            editor);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor, (GimpViewable *) gimp_context_get_buffer (context));

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

  return GTK_WIDGET (buffer_view);
}

static void
gimp_buffer_view_paste_clicked (GtkWidget      *widget,
				GimpBufferView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpBuffer          *buffer;

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpDisplay *gdisp  = gimp_context_get_display (editor->view->context);
      GimpImage   *gimage = NULL;
      gint         x      = -1;
      gint         y      = -1;
      gint         width  = -1;
      gint         height = -1;;

      if (gdisp)
	{
          GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);

          gimp_display_shell_untransform_viewport (shell,
                                                   &x, &y, &width, &height);

          gimage = gdisp->gimage;
        }
      else
        {
          gimage = gimp_context_get_image (editor->view->context);
        }

      if (gimage)
        {
	  gimp_edit_paste (gimage, gimp_image_active_drawable (gimage),
			   buffer, FALSE, x, y, width, height);

	  gimp_image_flush (gimage);
	}
    }
}

static void
gimp_buffer_view_paste_into_clicked (GtkWidget      *widget,
				     GimpBufferView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpBuffer          *buffer;

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpDisplay *gdisp  = gimp_context_get_display (editor->view->context);
      GimpImage   *gimage = NULL;
      gint         x      = -1;
      gint         y      = -1;
      gint         width  = -1;
      gint         height = -1;;

      if (gdisp)
	{
          GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);

          gimp_display_shell_untransform_viewport (shell,
                                                   &x, &y, &width, &height);

          gimage = gdisp->gimage;
        }
      else
        {
          gimage = gimp_context_get_image (editor->view->context);
        }

      if (gimage)
        {
	  gimp_edit_paste (gimage, gimp_image_active_drawable (gimage),
			   buffer, TRUE, x, y, width, height);

	  gimp_image_flush (gimage);
	}
    }
}

static void
gimp_buffer_view_paste_as_new_clicked (GtkWidget      *widget,
				       GimpBufferView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpBuffer          *buffer;

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpImage *gimage = gimp_context_get_image (editor->view->context);

      if (gimage)
        gimp_edit_paste_as_new (gimage->gimp, gimage, buffer);
    }
}

static void
gimp_buffer_view_delete_clicked (GtkWidget      *widget,
				 GimpBufferView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpBuffer          *buffer;

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      gimp_container_remove (editor->view->container,
			     GIMP_OBJECT (buffer));
    }
}

static void
gimp_buffer_view_select_item (GimpContainerEditor *editor,
			      GimpViewable        *viewable)
{
  GimpBufferView *view;

  gboolean  paste_sensitive        = FALSE;
  gboolean  paste_into_sensitive   = FALSE;
  gboolean  paste_as_new_sensitive = FALSE;
  gboolean  delete_sensitive       = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_BUFFER_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      paste_sensitive        = TRUE;
      paste_into_sensitive   = TRUE;
      paste_as_new_sensitive = TRUE;
      delete_sensitive       = TRUE;
    }

  gtk_widget_set_sensitive (view->paste_button,        paste_sensitive);
  gtk_widget_set_sensitive (view->paste_into_button,   paste_into_sensitive);
  gtk_widget_set_sensitive (view->paste_as_new_button, paste_as_new_sensitive);
  gtk_widget_set_sensitive (view->delete_button,       delete_sensitive);
}

static void
gimp_buffer_view_activate_item (GimpContainerEditor *editor,
				GimpViewable        *viewable)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (editor);

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      gimp_buffer_view_paste_clicked (NULL, view);
    }
}
