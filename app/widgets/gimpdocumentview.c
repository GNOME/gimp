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

#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "display/gimpdisplay.h"

#include "gimpcontainerview.h"
#include "gimpdocumentview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_document_view_class_init (GimpDocumentViewClass *klass);
static void   gimp_document_view_init       (GimpDocumentView      *view);

static void   gimp_document_view_open_clicked          (GtkWidget        *widget,
                                                        GimpDocumentView *view);
static void   gimp_document_view_open_extended_clicked (GtkWidget        *widget,
                                                        guint             state,
                                                        GimpDocumentView *view);
static void   gimp_document_view_remove_clicked        (GtkWidget        *widget,
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
static void   gimp_document_view_open_image      (GimpDocumentView    *view,
                                                  GimpImagefile       *imagefile);
static GList * gimp_document_view_drag_file      (GtkWidget           *widget,
                                                  gpointer             data);


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
  view->remove_button  = NULL;
  view->refresh_button = NULL;
}

GtkWidget *
gimp_document_view_new (GimpViewType            view_type,
                        GimpContainer          *container,
                        GimpContext            *context,
                        gint                    preview_size,
                        gint                    preview_border_width,
                        GimpFileOpenDialogFunc  file_open_dialog_func,
                        GimpMenuFactory        *menu_factory)
{
  GimpDocumentView    *document_view;
  GimpContainerEditor *editor;
  gchar               *str;

  g_return_val_if_fail (file_open_dialog_func != NULL, NULL);

  document_view = g_object_new (GIMP_TYPE_DOCUMENT_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (document_view),
                                         view_type,
                                         container, context,
                                         preview_size, preview_border_width,
                                         TRUE, /* reorderable */
                                         menu_factory, "<Documents>"))
    {
      g_object_unref (document_view);
      return NULL;
    }

  document_view->file_open_dialog_func = file_open_dialog_func;

  editor = GIMP_CONTAINER_EDITOR (document_view);

  str = g_strdup_printf (_("Open the selected entry\n"
                           "%s  Raise window if already open\n"
                           "%s  Open image dialog"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control ());

  document_view->open_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_OPEN, str,
                            GIMP_HELP_DOCUMENT_OPEN,
                            G_CALLBACK (gimp_document_view_open_clicked),
                            G_CALLBACK (gimp_document_view_open_extended_clicked),
                            editor);

  g_free (str);

  document_view->remove_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_REMOVE, _("Remove selected entry"),
                            GIMP_HELP_DOCUMENT_REMOVE,
                            G_CALLBACK (gimp_document_view_remove_clicked),
                            NULL,
                            editor);

  str = g_strdup_printf (_("Recreate preview\n"
                           "%s  Reload all previews\n"
                           "%s  Remove Dangling Entries"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control ());

  document_view->refresh_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_REFRESH, str,
                            GIMP_HELP_DOCUMENT_REFRESH,
                            G_CALLBACK (gimp_document_view_refresh_clicked),
                            G_CALLBACK (gimp_document_view_refresh_extended_clicked),
                            editor);

  g_free (str);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor, (GimpViewable *) gimp_context_get_imagefile (context));

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (document_view->open_button),
				  GIMP_TYPE_IMAGEFILE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (document_view->remove_button),
				  GIMP_TYPE_IMAGEFILE);

  if (view_type == GIMP_VIEW_TYPE_LIST)
    {
      static const GtkTargetEntry document_view_target_entries[] =
      {
        GIMP_TARGET_IMAGEFILE,
        GIMP_TARGET_URI_LIST
      };

      gtk_drag_source_set (editor->view->dnd_widget,
                           GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                           document_view_target_entries,
                           G_N_ELEMENTS (document_view_target_entries),
                           GDK_ACTION_COPY | GDK_ACTION_MOVE);
      gimp_dnd_file_source_add (editor->view->dnd_widget,
                                gimp_document_view_drag_file,
                                editor);
    }

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
      gimp_document_view_open_image (view, imagefile);
    }
  else
    {
      view->file_open_dialog_func (editor->view->context->gimp, NULL,
                                   GTK_WIDGET (view));
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
      gtk_window_present (GTK_WINDOW (gdisp->shell));
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
          view->file_open_dialog_func (editor->view->context->gimp,
                                       gimp_object_get_name (GIMP_OBJECT (imagefile)),
                                       GTK_WIDGET (view));
        }
      else if (state & GDK_SHIFT_MASK)
        {
          RaiseClosure closure;

          closure.name  = gimp_object_get_name (GIMP_OBJECT (imagefile));
          closure.found = FALSE;

          gimp_container_foreach (editor->view->context->gimp->displays,
                                  gimp_document_view_raise_display,
                                  &closure);

          if (! closure.found)
            gimp_document_view_open_image (view, imagefile);
        }
    }
  else
    {
      view->file_open_dialog_func (editor->view->context->gimp, NULL,
                                   GTK_WIDGET (view));
    }
}

static void
gimp_document_view_remove_clicked (GtkWidget        *widget,
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
  if (gimp_thumbnail_peek_image (imagefile->thumbnail) == GIMP_THUMB_STATE_NOT_FOUND)
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
                              editor->view);
    }
  else if (state & GDK_SHIFT_MASK)
    {
      gimp_container_foreach (editor->view->container,
                              (GFunc) gimp_imagefile_update,
                              editor->view);
    }
}

static void
gimp_document_view_select_item (GimpContainerEditor *editor,
                                GimpViewable        *viewable)
{
  GimpDocumentView *view;
  gboolean          remove_sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_DOCUMENT_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      remove_sensitive = TRUE;
    }

  gtk_widget_set_sensitive (view->remove_button, remove_sensitive);
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

static void
gimp_document_view_open_image (GimpDocumentView *view,
                               GimpImagefile    *imagefile)
{
  Gimp              *gimp;
  const gchar       *uri;
  GimpImage         *gimage;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  gimp = GIMP_CONTAINER_EDITOR (view)->view->context->gimp;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  gimage = file_open_with_display (gimp, uri, &status, &error);

  if (! gimage && status != GIMP_PDB_CANCEL)
    {
      gchar *filename;

      filename = file_utils_uri_to_utf8_filename (uri);

      g_message (_("Opening '%s' failed:\n\n%s"),
                 filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }
}

static GList *
gimp_document_view_drag_file (GtkWidget *widget,
                              gpointer   data)
{
  GimpViewable *viewable;

  viewable = gimp_dnd_get_drag_data (widget);

  if (viewable)
    {
      GList *list = NULL;

      return g_list_append (list, g_strdup (gimp_object_get_name (GIMP_OBJECT (viewable))));
    }

  return NULL;
}
