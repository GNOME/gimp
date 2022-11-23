/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmathumb/ligmathumb.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimagefile.h"

#include "file/file-open.h"

#include "widgets/ligmaclipboard.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmacontainerview-utils.h"
#include "widgets/ligmadocumentview.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"
#include "display/ligmadisplayshell.h"

#include "documents-commands.h"
#include "file-commands.h"

#include "ligma-intl.h"


typedef struct
{
  const gchar *name;
  gboolean     found;
} RaiseClosure;


/*  local function prototypes  */

static void   documents_open_image    (GtkWidget     *editor,
                                       LigmaContext   *context,
                                       LigmaImagefile *imagefile);
static void   documents_raise_display (LigmaDisplay   *display,
                                       RaiseClosure  *closure);



/*  public functions */

void
documents_open_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaContainer       *container;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  container = ligma_container_view_get_container (editor->view);

  imagefile = ligma_context_get_imagefile (context);

  if (imagefile && ligma_container_have (container, LIGMA_OBJECT (imagefile)))
    {
      documents_open_image (GTK_WIDGET (editor), context, imagefile);
    }
  else
    {
      file_file_open_dialog (context->ligma, NULL, GTK_WIDGET (editor));
    }
}

void
documents_raise_or_open_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaContainer       *container;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  container = ligma_container_view_get_container (editor->view);

  imagefile = ligma_context_get_imagefile (context);

  if (imagefile && ligma_container_have (container, LIGMA_OBJECT (imagefile)))
    {
      RaiseClosure closure;

      closure.name  = ligma_object_get_name (imagefile);
      closure.found = FALSE;

      ligma_container_foreach (context->ligma->displays,
                              (GFunc) documents_raise_display,
                              &closure);

      if (! closure.found)
        documents_open_image (GTK_WIDGET (editor), context, imagefile);
    }
}

void
documents_file_open_dialog_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaContainer       *container;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  container = ligma_container_view_get_container (editor->view);

  imagefile = ligma_context_get_imagefile (context);

  if (imagefile && ligma_container_have (container, LIGMA_OBJECT (imagefile)))
    {
      file_file_open_dialog (context->ligma,
                             ligma_imagefile_get_file (imagefile),
                             GTK_WIDGET (editor));
    }
}

void
documents_copy_location_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  imagefile = ligma_context_get_imagefile (context);

  if (imagefile)
    ligma_clipboard_set_text (context->ligma,
                             ligma_object_get_name (imagefile));
}

void
documents_show_in_file_manager_cmd_callback (LigmaAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  imagefile = ligma_context_get_imagefile (context);

  if (imagefile)
    {
      GFile  *file  = g_file_new_for_uri (ligma_object_get_name (imagefile));
      GError *error = NULL;

      if (! ligma_file_show_in_file_manager (file, &error))
        {
          ligma_message (context->ligma, G_OBJECT (editor),
                        LIGMA_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

void
documents_remove_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaContainerEditor *editor  = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context = ligma_container_view_get_context (editor->view);
  LigmaImagefile       *imagefile = ligma_context_get_imagefile (context);
  const gchar         *uri;

  uri = ligma_object_get_name (imagefile);

  gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri, NULL);

  ligma_container_view_remove_active (editor->view);
}

void
documents_clear_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaContainerEditor *editor  = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context = ligma_container_view_get_context (editor->view);
  Ligma                *ligma    = context->ligma;
  GtkWidget           *dialog;

  dialog = ligma_message_dialog_new (_("Clear Document History"),
                                    LIGMA_ICON_SHRED,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    ligma_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Clear the Recent Documents list?"));

  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the document history will "
                               "permanently remove all images from "
                               "the recent documents list."));

  if (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK)
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

      ligma_container_clear (ligma->documents);
    }

  gtk_widget_destroy (dialog);
}

void
documents_recreate_preview_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaContainer       *container;
  LigmaImagefile       *imagefile;

  context   = ligma_container_view_get_context (editor->view);
  container = ligma_container_view_get_container (editor->view);

  imagefile = ligma_context_get_imagefile (context);

  if (imagefile && ligma_container_have (container, LIGMA_OBJECT (imagefile)))
    {
      GError *error = NULL;

      if (! ligma_imagefile_create_thumbnail (imagefile,
                                             context, NULL,
                                             context->ligma->config->thumbnail_size,
                                             FALSE, &error))
        {
          ligma_message_literal (context->ligma,
                                NULL , LIGMA_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}

void
documents_reload_previews_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;

  container = ligma_container_view_get_container (editor->view);

  ligma_container_foreach (container,
                          (GFunc) ligma_imagefile_update,
                          editor->view);
}

static void
documents_remove_dangling_foreach (LigmaImagefile *imagefile,
                                   LigmaContainer *container)
{
  LigmaThumbnail *thumbnail = ligma_imagefile_get_thumbnail (imagefile);

  if (ligma_thumbnail_peek_image (thumbnail) == LIGMA_THUMB_STATE_NOT_FOUND)
    {
      const gchar *uri = ligma_object_get_name (imagefile);

      gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri,
                                      NULL);

      ligma_container_remove (container, LIGMA_OBJECT (imagefile));
    }
}

void
documents_remove_dangling_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;

  container = ligma_container_view_get_container (editor->view);

  ligma_container_foreach (container,
                          (GFunc) documents_remove_dangling_foreach,
                          container);
}


/*  private functions  */

static void
documents_open_image (GtkWidget     *editor,
                      LigmaContext   *context,
                      LigmaImagefile *imagefile)
{
  GFile              *file;
  LigmaImage          *image;
  LigmaPDBStatusType   status;
  GError             *error = NULL;

  file = ligma_imagefile_get_file (imagefile);

  image = file_open_with_display (context->ligma, context, NULL, file, FALSE,
                                  G_OBJECT (ligma_widget_get_monitor (editor)),
                                  &status, &error);

  if (! image && status != LIGMA_PDB_CANCEL)
    {
      ligma_message (context->ligma, G_OBJECT (editor), LIGMA_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }
}

static void
documents_raise_display (LigmaDisplay  *display,
                         RaiseClosure *closure)
{
  const gchar *uri = ligma_object_get_name (ligma_display_get_image (display));

  if (! g_strcmp0 (closure->name, uri))
    {
      closure->found = TRUE;
      ligma_display_shell_present (ligma_display_get_shell (display));
    }
}
