/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpdnd-xds.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
 *
 * Saving Files via Drag-and-Drop:
 * The Direct Save Protocol for the X Window System
 *
 *   http://www.newplanetsoftware.com/xds/
 *   http://rox.sourceforge.net/xds.html
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpimage.h"

#include "plug-in/gimppluginmanager-file.h"

#include "file/file-save.h"

#include "gimpdnd-xds.h"
#include "gimpfiledialog.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"

#include "gimp-log.h"
#include "gimp-intl.h"


#define MAX_URI_LEN 4096


/*  local function prototypes  */

static gboolean   gimp_file_overwrite_dialog (GtkWidget *parent,
                                              GFile     *file);


/*  public functions  */

void
gimp_dnd_xds_source_set (GdkDragContext *context,
                         GimpImage      *image)
{
  GdkAtom  property;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  GIMP_LOG (DND, NULL);

  property = gdk_atom_intern_static_string ("XdndDirectSave0");

  if (image)
    {
      GdkAtom  type = gdk_atom_intern_static_string ("text/plain");
      GFile   *untitled;
      GFile   *file;
      gchar   *basename;

      basename = g_strconcat (_("Untitled"), ".xcf", NULL);
      untitled = g_file_new_for_path (basename);
      g_free (basename);

      file = gimp_image_get_any_file (image);

      if (file)
        {
          GFile *xcf_file = gimp_file_with_new_extension (file, untitled);
          basename = g_file_get_basename (xcf_file);
          g_object_unref (xcf_file);
        }
      else
        {
          basename = g_file_get_path (untitled);
        }

      g_object_unref (untitled);

      gdk_property_change (gdk_drag_context_get_source_window (context),
                           property, type, 8, GDK_PROP_MODE_REPLACE,
                           (const guchar *) basename,
                           basename ? strlen (basename) : 0);

      g_free (basename);
    }
  else
    {
      gdk_property_delete (gdk_drag_context_get_source_window (context),
                           property);
    }
}

void
gimp_dnd_xds_save_image (GdkDragContext   *context,
                         GimpImage        *image,
                         GtkSelectionData *selection)
{
  GimpPlugInProcedure *proc;
  GdkAtom              property;
  GdkAtom              type;
  gint                 length;
  guchar              *data;
  gchar               *uri;
  GFile               *file;
  gboolean             export = FALSE;
  GError              *error  = NULL;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_LOG (DND, NULL);

  property = gdk_atom_intern_static_string ("XdndDirectSave0");
  type     = gdk_atom_intern_static_string ("text/plain");

  if (! gdk_property_get (gdk_drag_context_get_source_window (context),
                          property, type,
                          0, MAX_URI_LEN, FALSE,
                          NULL, NULL, &length, &data))
    return;


  uri = g_strndup ((const gchar *) data, length);
  g_free (data);

  file = g_file_new_for_uri (uri);

  proc = gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                   GIMP_FILE_PROCEDURE_GROUP_SAVE,
                                                   file, NULL);
  if (! proc)
    {
      proc = gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                       GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                                                       file, NULL);
      export = TRUE;
    }

  if (proc)
    {
      if (! g_file_query_exists (file, NULL) ||
          gimp_file_overwrite_dialog (NULL, file))
        {
          if (file_save (image->gimp,
                         image, NULL,
                         file, proc, GIMP_RUN_INTERACTIVE,
                         ! export, FALSE, export,
                         &error) == GIMP_PDB_SUCCESS)
            {
              gtk_selection_data_set (selection,
                                      gtk_selection_data_get_target (selection),
                                      8, (const guchar *) "S", 1);
            }
          else
            {
              gtk_selection_data_set (selection,
                                      gtk_selection_data_get_target (selection),
                                      8, (const guchar *) "E", 1);

              if (error)
                {
                  gimp_message (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                                _("Saving '%s' failed:\n\n%s"),
                                gimp_file_get_utf8_name (file),
                                error->message);
                  g_clear_error (&error);
                }
            }
        }
    }
  else
    {
      gtk_selection_data_set (selection,
                              gtk_selection_data_get_target (selection),
                              8, (const guchar *) "E", 1);

      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("The given filename does not have any known "
                              "file extension."));
    }

  g_object_unref (file);
  g_free (uri);
}


/*  private functions  */

static gboolean
gimp_file_overwrite_dialog (GtkWidget *parent,
                            GFile     *file)
{
  GtkWidget *dialog;
  gboolean   overwrite = FALSE;

  dialog = gimp_message_dialog_new (_("File Exists"),
                                    GIMP_ICON_DIALOG_WARNING,
                                    parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                    _("_Replace"), GTK_RESPONSE_OK,

                                    NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("A file named '%s' already exists."),
                                     gimp_file_get_utf8_name (file));

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Do you want to replace it with the image "
                               "you are saving?"));

  if (GTK_IS_DIALOG (parent))
    gtk_dialog_set_response_sensitive (GTK_DIALOG (parent),
                                       GTK_RESPONSE_CANCEL, FALSE);

  g_object_ref (dialog);

  overwrite = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);
  g_object_unref (dialog);

  if (GTK_IS_DIALOG (parent))
    gtk_dialog_set_response_sensitive (GTK_DIALOG (parent),
                                       GTK_RESPONSE_CANCEL, TRUE);

  return overwrite;
}
