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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "file/file-open.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gui/file-open-dialog.h"

#include "gimpcontainerview.h"
#include "gimpdocumentview.h"
#include "gimpdnd.h"

#include "libgimp/gimpintl.h"


static void   gimp_document_view_class_init (GimpDocumentViewClass *klass);
static void   gimp_document_view_init       (GimpDocumentView      *view);

static void   gimp_document_view_open_clicked          (GtkWidget        *widget,
                                                        GimpDocumentView *view);
static void   gimp_document_view_open_extended_clicked (GtkWidget        *widget,
                                                        guint             state,
                                                        GimpDocumentView *view);
static void   gimp_document_view_delete_clicked        (GtkWidget        *widget,
                                                        GimpDocumentView *view);
static void   gimp_document_view_refresh_clicked       (GtkWidget        *widget,
                                                        GimpDocumentView *view);
static void   gimp_document_view_refresh_extended_clicked (GtkWidget     *widget,
                                                           guint          state,
                                                        GimpDocumentView *view);

static void   gimp_document_view_select_item     (GimpContainerEditor *editor,
                                                  GimpViewable        *viewable);
static void   gimp_document_view_activate_item   (GimpContainerEditor *editor,
                                                  GimpViewable        *viewable);


static GimpContainerEditorClass *parent_class = NULL;


GType
gimp_document_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpDocumentViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_document_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDocumentView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_document_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpDocumentView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_document_view_class_init (GimpDocumentViewClass *klass)
{
  GimpContainerEditorClass *editor_class;

  editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_document_view_select_item;
  editor_class->activate_item = gimp_document_view_activate_item;
}

static void
gimp_document_view_init (GimpDocumentView *view)
{
  view->open_button    = NULL;
  view->delete_button  = NULL;
  view->refresh_button = NULL;
}

GtkWidget *
gimp_document_view_new (GimpViewType     view_type,
                        GimpContainer   *container,
                        GimpContext     *context,
                        gint             preview_size,
                        gint             min_items_x,
                        gint             min_items_y,
                        GimpItemFactory *item_factory)
{
  GimpDocumentView    *document_view;
  GimpContainerEditor *editor;

  document_view = g_object_new (GIMP_TYPE_DOCUMENT_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (document_view),
                                         view_type,
                                         container,
                                         context,
                                         preview_size,
                                         TRUE, /* reorderable */
                                         min_items_x,
                                         min_items_y,
                                         item_factory))
    {
      g_object_unref (G_OBJECT (document_view));
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (document_view);

  document_view->open_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_OPEN,
                            _("Open the selected entry\n"
                              "<Shift> Raise window if already open\n"
                              "<Ctrl> Open image dialog"),
                            NULL,
                            G_CALLBACK (gimp_document_view_open_clicked),
                            G_CALLBACK (gimp_document_view_open_extended_clicked),
                            editor);

  document_view->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_DELETE,
                            _("Remove selected entry"), NULL,
                            G_CALLBACK (gimp_document_view_delete_clicked),
                            NULL,
                            editor);

  document_view->refresh_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_REFRESH,
                            _("Recreate preview\n"
                              "<Shift> Reload all previews\n"
                              "<Ctrl> Remove Dangling Entries"),
                            NULL,
                            G_CALLBACK (gimp_document_view_refresh_clicked),
                            G_CALLBACK (gimp_document_view_refresh_extended_clicked),
                            editor);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor, (GimpViewable *) gimp_context_get_imagefile (context));

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (document_view->open_button),
				  GIMP_TYPE_IMAGEFILE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (document_view->delete_button),
				  GIMP_TYPE_IMAGEFILE);

  return GTK_WIDGET (document_view);
}

static void
gimp_document_view_open_clicked (GtkWidget        *widget,
                                 GimpDocumentView *view)
{
  GimpContainerEditor *editor;
  GimpImagefile       *imagefile;

  editor = GIMP_CONTAINER_EDITOR (view);

  imagefile = gimp_context_get_imagefile (editor->view->context);

  if (imagefile && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (imagefile)))
    {
      GimpPDBStatusType dummy;

      file_open_with_display (editor->view->context->gimp,
                              gimp_object_get_name (GIMP_OBJECT (imagefile)),
                              &dummy, NULL);
    }
  else
    {
      file_open_dialog_show (editor->view->context->gimp);
    }
}


typedef struct _RaiseClosure RaiseClosure;

struct _RaiseClosure
{
  const gchar *name;
  gboolean     found;
};

static void
gimp_document_view_raise_display (gpointer data,
                                  gpointer user_data)
{
  GimpDisplay  *gdisp;
  RaiseClosure *closure;
  const gchar  *uri;

  gdisp   = (GimpDisplay *) data;
  closure = (RaiseClosure *) user_data;

  uri = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

  if (uri && ! strcmp (closure->name, uri))
    {
      closure->found = TRUE;
      gdk_window_raise (gdisp->shell->window);
    }
}

static void
gimp_document_view_open_extended_clicked (GtkWidget        *widget,
                                          guint             state,
                                          GimpDocumentView *view)
{
  GimpContainerEditor *editor;
  GimpImagefile       *imagefile;

  editor = GIMP_CONTAINER_EDITOR (view);

  imagefile = gimp_context_get_imagefile (editor->view->context);

  if (imagefile && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (imagefile)))
    {
      if (state & GDK_CONTROL_MASK)
        {
          file_open_dialog_show (editor->view->context->gimp);
        }
      else if (state & GDK_SHIFT_MASK)
        {
          RaiseClosure closure;

          closure.name  = gimp_object_get_name (GIMP_OBJECT (imagefile));
          closure.found = FALSE;

          gdisplays_foreach (gimp_document_view_raise_display, &closure);

          if (! closure.found)
            {
              GimpPDBStatusType dummy;

              file_open_with_display (editor->view->context->gimp,
                                      closure.name,
                                      &dummy, NULL);
            }
        }
    }
  else
    {
      file_open_dialog_show (editor->view->context->gimp);
    }
}

static void
gimp_document_view_delete_clicked (GtkWidget        *widget,
                                   GimpDocumentView *view)
{
  GimpContainerEditor *editor;
  GimpImagefile       *imagefile;

  editor = GIMP_CONTAINER_EDITOR (view);

  imagefile = gimp_context_get_imagefile (editor->view->context);

  if (imagefile && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (imagefile)))
    {
      gimp_container_remove (editor->view->container,
			     GIMP_OBJECT (imagefile));
    }
}

static void
gimp_document_view_refresh_clicked (GtkWidget        *widget,
                                    GimpDocumentView *view)
{
  GimpContainerEditor *editor;
  GimpImagefile       *imagefile;

  editor = GIMP_CONTAINER_EDITOR (view);

  imagefile = gimp_context_get_imagefile (editor->view->context);

  if (imagefile && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (imagefile)))
    {
      gimp_imagefile_create_thumbnail (imagefile, editor->view->preview_size);
    }
}

static void
gimp_document_view_delete_dangling_foreach (GimpImagefile     *imagefile,
                                            GimpContainerView *container_view)
{
  gimp_imagefile_update (imagefile, container_view->preview_size);

  if (imagefile->state == GIMP_IMAGEFILE_STATE_NOT_FOUND)
    {
      gimp_container_remove (container_view->container, 
                             GIMP_OBJECT (imagefile));
    }
}

static void
gimp_document_view_refresh_extended_clicked (GtkWidget        *widget,
                                             guint             state,
                                             GimpDocumentView *view)
{
  GimpContainerEditor *editor;

  editor = GIMP_CONTAINER_EDITOR (view);

  if (state & GDK_CONTROL_MASK)
    {
      gimp_container_foreach (editor->view->container,
                              (GFunc) gimp_document_view_delete_dangling_foreach,
                              editor->view->container);
    }
  else if (state & GDK_SHIFT_MASK)
    {
      gimp_container_foreach (editor->view->container,
                              (GFunc) gimp_imagefile_update,
                              (gpointer) editor->view->preview_size);
    }
}

static void
gimp_document_view_select_item (GimpContainerEditor *editor,
                                GimpViewable        *viewable)
{
  GimpDocumentView *view;

  gboolean  delete_sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_DOCUMENT_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      delete_sensitive = TRUE;
    }

  gtk_widget_set_sensitive (view->delete_button, delete_sensitive);
}

static void
gimp_document_view_activate_item (GimpContainerEditor *editor,
                                  GimpViewable        *viewable)
{
  GimpDocumentView *view;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  view = GIMP_DOCUMENT_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      gimp_document_view_open_clicked (NULL, view);
    }
}
