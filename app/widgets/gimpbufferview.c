/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbufferview.c
 * Copyright (C) 2001 Michael Natterer
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

#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"

#include "gimpcontainerview.h"
#include "gimpbufferview.h"
#include "gimpdnd.h"

#include "gdisplay.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/paste.xpm"
#include "pixmaps/paste-into.xpm"
#include "pixmaps/paste-as-new.xpm"
#include "pixmaps/delete.xpm"


static void   gimp_buffer_view_class_init (GimpBufferViewClass *klass);
static void   gimp_buffer_view_init       (GimpBufferView      *view);
static void   gimp_buffer_view_destroy    (GtkObject           *object);

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


GtkType
gimp_buffer_view_get_type (void)
{
  static guint view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpBufferView",
	sizeof (GimpBufferView),
	sizeof (GimpBufferViewClass),
	(GtkClassInitFunc) gimp_buffer_view_class_init,
	(GtkObjectInitFunc) gimp_buffer_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GIMP_TYPE_CONTAINER_EDITOR, &view_info);
    }

  return view_type;
}

static void
gimp_buffer_view_class_init (GimpBufferViewClass *klass)
{
  GtkObjectClass           *object_class;
  GimpContainerEditorClass *editor_class;

  object_class = (GtkObjectClass *) klass;
  editor_class = (GimpContainerEditorClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER_EDITOR);

  object_class->destroy       = gimp_buffer_view_destroy;

  editor_class->select_item   = gimp_buffer_view_select_item;
  editor_class->activate_item = gimp_buffer_view_activate_item;
}

static void
gimp_buffer_view_init (GimpBufferView *view)
{
  GimpContainerEditor *editor;

  editor = GIMP_CONTAINER_EDITOR (view);

  view->paste_button =
    gimp_container_editor_add_button (editor,
				      paste_xpm,
				      _("Paste"), NULL,
				      gimp_buffer_view_paste_clicked);

  view->paste_into_button =
    gimp_container_editor_add_button (editor,
				      paste_into_xpm,
				      _("Paste Into"), NULL,
				      gimp_buffer_view_paste_into_clicked);

  view->paste_as_new_button =
    gimp_container_editor_add_button (editor,
				      paste_as_new_xpm,
				      _("Paste as New"), NULL,
				      gimp_buffer_view_paste_as_new_clicked);

  view->delete_button =
    gimp_container_editor_add_button (editor,
				      delete_xpm,
				      _("Delete"), NULL,
				      gimp_buffer_view_delete_clicked);

  gtk_widget_set_sensitive (view->paste_button, FALSE);
  gtk_widget_set_sensitive (view->paste_into_button, FALSE);
  gtk_widget_set_sensitive (view->paste_as_new_button, FALSE);
  gtk_widget_set_sensitive (view->delete_button, FALSE);
}

static void
gimp_buffer_view_destroy (GtkObject *object)
{
  GimpBufferView *view;

  view = GIMP_BUFFER_VIEW (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_buffer_view_new (GimpViewType   view_type,
		      GimpContainer *container,
		      GimpContext   *context,
		      gint           preview_size,
		      gint           min_items_x,
		      gint           min_items_y)
{
  GimpBufferView      *buffer_view;
  GimpContainerEditor *editor;

  buffer_view = gtk_type_new (GIMP_TYPE_BUFFER_VIEW);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (buffer_view),
					 view_type,
					 container,
					 context,
					 preview_size,
					 min_items_x,
					 min_items_y))
    {
      gtk_object_unref (GTK_OBJECT (buffer_view));
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (buffer_view);

  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (buffer_view->paste_button));
  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (buffer_view->paste_into_button));
  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (buffer_view->paste_as_new_button));
  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (buffer_view->delete_button));

  return GTK_WIDGET (buffer_view);
}

static void
gimp_buffer_view_paste_clicked (GtkWidget      *widget,
				GimpBufferView *view)
{
  GimpContainerEditor *editor;
  GimpBuffer          *buffer;

  editor = GIMP_CONTAINER_EDITOR (view);

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpImage *gimage;

      gimage = gimp_context_get_image (editor->view->context);

      if (gimage)
	{
	  gimp_edit_paste (gimage,
			   gimp_image_active_drawable (gimage),
			   buffer->tiles,
			   FALSE);

	  gdisplays_flush ();
	}
    }
}

static void
gimp_buffer_view_paste_into_clicked (GtkWidget      *widget,
				     GimpBufferView *view)
{
  GimpContainerEditor *editor;
  GimpBuffer          *buffer;

  editor = GIMP_CONTAINER_EDITOR (view);

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpImage *gimage;

      gimage = gimp_context_get_image (editor->view->context);

      if (gimage)
	{
	  gimp_edit_paste (gimage,
			   gimp_image_active_drawable (gimage),
			   buffer->tiles,
			   TRUE);

	  gdisplays_flush ();
	}
    }
}

static void
gimp_buffer_view_paste_as_new_clicked (GtkWidget      *widget,
				       GimpBufferView *view)
{
  GimpContainerEditor *editor;
  GimpBuffer          *buffer;

  editor = GIMP_CONTAINER_EDITOR (view);

  buffer = gimp_context_get_buffer (editor->view->context);

  if (buffer && gimp_container_have (editor->view->container,
				     GIMP_OBJECT (buffer)))
    {
      GimpImage *gimage;

      gimage = gimp_context_get_image (editor->view->context);

      if (gimage)
	{
	  gimp_edit_paste_as_new (gimage->gimp, gimage, buffer->tiles);
	}
    }
}

static void
gimp_buffer_view_delete_clicked (GtkWidget      *widget,
				 GimpBufferView *view)
{
  GimpContainerEditor *editor;
  GimpBuffer          *buffer;

  editor = GIMP_CONTAINER_EDITOR (view);

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

  if (viewable &&
      gimp_container_have (editor->view->container,
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
  GimpBufferView *view;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  view = GIMP_BUFFER_VIEW (editor);

  if (viewable &&
      gimp_container_have (editor->view->container,
			   GIMP_OBJECT (viewable)))
    {
      gimp_buffer_view_paste_clicked (NULL, view);
    }
}
