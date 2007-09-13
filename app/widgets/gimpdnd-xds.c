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

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "plug-in/gimppluginmanager.h"

#include "file/file-procedure.h"
#include "file/file-save.h"
#include "file/file-utils.h"

#include "gimpdnd-xds.h"
#include "gimpfiledialog.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"

#include "gimp-intl.h"


#ifdef DEBUG_DND
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


#define MAX_URI_LEN 4096


/*  local function prototypes  */

static gboolean   gimp_file_overwrite_dialog (GtkWidget   *parent,
                                              const gchar *uri);


/*  public functions  */

void
gimp_dnd_xds_source_set (GdkDragContext *context,
                         GimpImage      *image)
{
  GdkAtom  property;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  D (g_printerr ("\ngimp_dnd_xds_source_set\n"));

  property = gdk_atom_intern_static_string ("XdndDirectSave0");

  if (image)
    {
      GdkAtom  type     = gdk_atom_intern_static_string ("text/plain");
      gchar   *filename = gimp_image_get_filename (image);
      gchar   *basename;

      if (filename)
        {
          basename = g_path_get_basename (filename);
        }
      else
        {
          gchar *tmp = g_strconcat (_("Untitled"), ".xcf", NULL);
          basename = g_filename_from_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }


      gdk_property_change (context->source_window,
                           property, type, 8, GDK_PROP_MODE_REPLACE,
                           (const guchar *) basename,
                           basename ? strlen (basename) : 0);

      g_free (basename);
      g_free (filename);
    }
  else
    {
      gdk_property_delete (context->source_window, property);
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
  GError              *error = NULL;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  D (g_printerr ("\ngimp_dnd_xds_save_image\n"));

  property = gdk_atom_intern_static_string ("XdndDirectSave0");
  type     = gdk_atom_intern_static_string ("text/plain");

  if (! gdk_property_get (context->source_window, property, type,
                          0, MAX_URI_LEN, FALSE,
                          NULL, NULL, &length, &data))
    return;


  uri = g_strndup ((const gchar *) data, length);
  g_free (data);

  proc = file_procedure_find (image->gimp->plug_in_manager->save_procs, uri,
                              NULL);

  if (proc)
    {
      gchar *filename = file_utils_filename_from_uri (uri);

      /*  FIXME: shouldn't overwrite non-local files w/o confirmation  */

      if (! filename ||
          ! g_file_test (filename, G_FILE_TEST_EXISTS) ||
          gimp_file_overwrite_dialog (NULL, uri))
        {
          if (file_save (image->gimp, image,
                         gimp_get_user_context (image->gimp), NULL,
                         uri, proc, GIMP_RUN_INTERACTIVE, FALSE,
                         &error) == GIMP_PDB_SUCCESS)
            {
              gtk_selection_data_set (selection, selection->target, 8,
                                      (const guchar *) "S", 1);
            }
          else
            {
              gtk_selection_data_set (selection, selection->target, 8,
                                      (const guchar *) "E", 1);

              if (error)
                {
                  gchar *filename = file_utils_uri_display_name (uri);

                  gimp_message (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                                _("Saving '%s' failed:\n\n%s"),
                                filename, error->message);

                  g_free (filename);
                  g_error_free (error);
                }
            }
        }

      g_free (filename);
    }
  else
    {
      gtk_selection_data_set (selection, selection->target, 8,
                              (const guchar *) "E", 1);

      gimp_message (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                    _("The given filename does not have any known "
                      "file extension."));
    }

  g_free (uri);
}


/*  private functions  */

static gboolean
gimp_file_overwrite_dialog (GtkWidget   *parent,
                            const gchar *uri)
{
  GtkWidget *dialog;
  gchar     *filename;
  gboolean   overwrite = FALSE;

  dialog = gimp_message_dialog_new (_("File Exists"), GIMP_STOCK_WARNING,
                                    parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    _("_Replace"),    GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  filename = file_utils_uri_display_name (uri);
  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("A file named '%s' already exists."),
                                     filename);
  g_free (filename);

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
