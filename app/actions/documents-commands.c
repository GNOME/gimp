/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"

#include "file/file-open.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "documents-commands.h"
#include "file-commands.h"

#include "gimp-intl.h"


typedef struct
{
  const gchar *name;
  gboolean     found;
} RaiseClosure;


/*  local function prototypes  */

static void   documents_open_image    (GtkWidget     *editor,
                                       GimpContext   *context,
                                       GimpImagefile *imagefile);
static void   documents_raise_display (GimpDisplay   *display,
                                       RaiseClosure  *closure);



/*  public functions */

void
documents_open_cmd_callback (GtkAction *action,
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
      documents_open_image (GTK_WIDGET (editor), context, imagefile);
    }
  else
    {
      file_file_open_dialog (context->gimp, NULL, GTK_WIDGET (editor));
    }
}

void
documents_raise_or_open_cmd_callback (GtkAction *action,
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

      closure.name  = gimp_object_get_name (imagefile);
      closure.found = FALSE;

      gimp_container_foreach (context->gimp->displays,
                              (GFunc) documents_raise_display,
                              &closure);

      if (! closure.found)
        documents_open_image (GTK_WIDGET (editor), context, imagefile);
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
                             gimp_imagefile_get_file (imagefile),
                             GTK_WIDGET (editor));
    }
}

void
documents_copy_location_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  imagefile = gimp_context_get_imagefile (context);

  if (imagefile)
    gimp_clipboard_set_text (context->gimp,
                             gimp_object_get_name (imagefile));
}

void
documents_show_in_file_manager_cmd_callback (GtkAction *action,
                                             gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpImagefile       *imagefile;

  context   = gimp_container_view_get_context (editor->view);
  imagefile = gimp_context_get_imagefile (context);

  if (imagefile)
    {
      GFile  *file  = g_file_new_for_uri (gimp_object_get_name (imagefile));
      GError *error = NULL;

      if (! gimp_file_show_in_file_manager (file, &error))
        {
          gimp_message (context->gimp, G_OBJECT (editor),
                        GIMP_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

void
documents_remove_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpContainerEditor *editor  = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context = gimp_container_view_get_context (editor->view);
  GimpImagefile       *imagefile = gimp_context_get_imagefile (context);
  const gchar         *uri;

  uri = gimp_object_get_name (imagefile);

  gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri, NULL);

  gimp_container_view_remove_active (editor->view);
}

void
documents_clear_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpContainerEditor *editor  = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context = gimp_container_view_get_context (editor->view);
  Gimp                *gimp    = context->gimp;
  GtkWidget           *dialog;

  dialog = gimp_message_dialog_new (_("Clear Document History"),
                                    GIMP_ICON_SHRED,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Clear the Recent Documents list?"));

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the document history will "
                               "permanently remove all images from "
                               "the recent documents list."));

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      GtkRecentManager *manager = gtk_recent_manager_get_default ();
      GList            *items;
      GList            *list;

      items = gtk_recent_manager_get_items (manager);

      for (list = items; list; list = list->next)
        {
          GtkRecentInfo *info = list->data;

          if (gtk_recent_info_has_application (info,
                                               "GNU Image Manipulation Program"))
            {
              gtk_recent_manager_remove_item (manager,
                                              gtk_recent_info_get_uri (info),
                                              NULL);
            }

          gtk_recent_info_unref (info);
        }

      g_list_free (items);

      gimp_container_clear (gimp->documents);
    }

  gtk_widget_destroy (dialog);
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
      GError *error = NULL;

      if (! gimp_imagefile_create_thumbnail (imagefile,
                                             context, NULL,
                                             context->gimp->config->thumbnail_size,
                                             FALSE, &error))
        {
          gimp_message_literal (context->gimp,
                                NULL , GIMP_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
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
documents_remove_dangling_foreach (GimpImagefile *imagefile,
                                   GimpContainer *container)
{
  GimpThumbnail *thumbnail = gimp_imagefile_get_thumbnail (imagefile);

  if (gimp_thumbnail_peek_image (thumbnail) == GIMP_THUMB_STATE_NOT_FOUND)
    {
      const gchar *uri = gimp_object_get_name (imagefile);

      gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri,
                                      NULL);

      gimp_container_remove (container, GIMP_OBJECT (imagefile));
    }
}

void
documents_remove_dangling_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;

  container = gimp_container_view_get_container (editor->view);

  gimp_container_foreach (container,
                          (GFunc) documents_remove_dangling_foreach,
                          container);
}


/*  private functions  */

static void
documents_open_image (GtkWidget     *editor,
                      GimpContext   *context,
                      GimpImagefile *imagefile)
{
  GFile              *file;
  GimpImage          *image;
  GimpPDBStatusType   status;
  GError             *error = NULL;

  file = gimp_imagefile_get_file (imagefile);

  image = file_open_with_display (context->gimp, context, NULL, file, FALSE,
                                  G_OBJECT (gimp_widget_get_monitor (editor)),
                                  &status, &error);

  if (! image && status != GIMP_PDB_CANCEL)
    {
      gimp_message (context->gimp, G_OBJECT (editor), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }
}

static void
documents_raise_display (GimpDisplay  *display,
                         RaiseClosure *closure)
{
  const gchar *uri = gimp_object_get_name (gimp_display_get_image (display));

  if (! g_strcmp0 (closure->name, uri))
    {
      closure->found = TRUE;
      gimp_display_shell_present (gimp_display_get_shell (display));
    }
}
