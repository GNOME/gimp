/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdocumentview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "documents-commands.h"
#include "file-commands.h"

#include "gimp-intl.h"


typedef struct _RaiseClosure RaiseClosure;

struct _RaiseClosure
{
  const gchar *name;
  gboolean     found;
};


/*  local function prototypes  */

static void   documents_open_image    (GimpContext   *context,
                                       GimpImagefile *imagefile);
static void   documents_raise_display (gpointer       data,
                                       gpointer       user_data);


/*  public functions */

void
documents_open_document_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpContainer       *container;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  container = gimp_container_view_get_container (editor->view);

  imagefile = gimp_context_get_imagefile (context);

  if (imagefile && gimp_container_have (container, GIMP_OBJECT (imagefile)))
    {
      documents_open_image (context, imagefile);
    }
  else
    {
      file_file_open_dialog (context->gimp, NULL, GTK_WIDGET (editor));
    }
}

void
documents_raise_or_open_document_cmd_callback (GtkAction *action,
                                               gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpContainer       *container;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  container = gimp_container_view_get_container (editor->view);

  imagefile = gimp_context_get_imagefile (context);

  if (imagefile && gimp_container_have (container, GIMP_OBJECT (imagefile)))
    {
      RaiseClosure closure;

      closure.name  = gimp_object_get_name (GIMP_OBJECT (imagefile));
      closure.found = FALSE;

      gimp_container_foreach (context->gimp->displays,
                              documents_raise_display,
                              &closure);

      if (! closure.found)
        documents_open_image (context, imagefile);
    }
}

void
documents_file_open_dialog_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpContainer       *container;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  container = gimp_container_view_get_container (editor->view);

  imagefile = gimp_context_get_imagefile (context);

  if (imagefile && gimp_container_have (container, GIMP_OBJECT (imagefile)))
    {
      file_file_open_dialog (context->gimp,
                             gimp_object_get_name (GIMP_OBJECT (imagefile)),
                             GTK_WIDGET (editor));
    }
}

void
documents_remove_document_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpContainer       *container;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  container = gimp_container_view_get_container (editor->view);

  imagefile = gimp_context_get_imagefile (context);

  if (imagefile && gimp_container_have (container, GIMP_OBJECT (imagefile)))
    {
      gimp_container_remove (container, GIMP_OBJECT (imagefile));
    }
}

void
documents_recreate_preview_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpContainer       *container;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  container = gimp_container_view_get_container (editor->view);

  imagefile = gimp_context_get_imagefile (context);

  if (imagefile && gimp_container_have (container, GIMP_OBJECT (imagefile)))
    {
      gimp_imagefile_create_thumbnail (imagefile,
                                       context, NULL,
                                       imagefile->gimp->config->thumbnail_size,
                                       FALSE);
    }
}

void
documents_reload_previews_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;

  container = gimp_container_view_get_container (editor->view);

  gimp_container_foreach (container,
                          (GFunc) gimp_imagefile_update,
                          editor->view);
}

static void
documents_delete_dangling_foreach (GimpImagefile *imagefile,
                                   GimpContainer *container)
{
  if (gimp_thumbnail_peek_image (imagefile->thumbnail) ==
      GIMP_THUMB_STATE_NOT_FOUND)
    {
      gimp_container_remove (container, GIMP_OBJECT (imagefile));
    }
}

void
documents_delete_dangling_documents_cmd_callback (GtkAction *action,
                                                  gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;

  container = gimp_container_view_get_container (editor->view);

  gimp_container_foreach (container,
                          (GFunc) documents_delete_dangling_foreach,
                          container);
}


/*  private functions  */

static void
documents_open_image (GimpContext   *context,
                      GimpImagefile *imagefile)
{
  const gchar        *uri;
  GimpImage          *gimage;
  GimpPDBStatusType   status;
  GError             *error = NULL;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  gimage = file_open_with_display (context->gimp, context, NULL,
                                   uri, &status, &error);

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

static void
documents_raise_display (gpointer data,
                         gpointer user_data)
{
  GimpDisplay  *gdisp   = data;
  RaiseClosure *closure = user_data;
  const gchar  *uri;

  uri = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

  if (uri && ! strcmp (closure->name, uri))
    {
      closure->found = TRUE;
      gtk_window_present (GTK_WINDOW (gdisp->shell));
    }
}
