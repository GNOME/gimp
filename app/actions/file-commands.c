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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptemplate.h"

#include "file/file-open.h"
#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "menus/menus.h"

#include "gui/file-open-dialog.h"
#include "gui/file-open-location-dialog.h"
#include "gui/file-save-dialog.h"

#include "actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


#define REVERT_DATA_KEY "revert-confirm-dialog"


/*  local function prototypes  */

static void   file_new_template_callback   (GtkWidget   *widget,
                                            const gchar *name,
                                            gpointer     data);
static void   file_revert_confirm_callback (GtkWidget   *widget,
                                            gboolean     revert,
                                            gpointer     data);


/*  public functions  */

void
file_open_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  file_open_dialog_show (gimp, NULL, NULL, global_menu_factory, widget);
}

void
file_open_from_image_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  file_open_dialog_show (gimp, action_data_get_image (data),
                         NULL, global_menu_factory, widget);
}

void
file_open_location_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  file_open_location_dialog_show (gimp, widget);
}

void
file_last_opened_cmd_callback (GtkAction *action,
                               gint       value,
                               gpointer   data)
{
  Gimp          *gimp;
  GimpImagefile *imagefile;
  gint           num_entries;
  return_if_no_gimp (gimp, data);

  num_entries = gimp_container_num_children (gimp->documents);

  if (value >= num_entries)
    return;

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_index (gimp->documents, value);

  if (imagefile)
    {
      GimpImage         *gimage;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      gimage = file_open_with_display (gimp, action_data_get_context (data),
                                       GIMP_OBJECT (imagefile)->name,
                                       &status, &error);

      if (! gimage && status != GIMP_PDB_CANCEL)
        {
          gchar *filename;

          filename =
            file_utils_uri_to_utf8_filename (GIMP_OBJECT (imagefile)->name);

          g_message (_("Opening '%s' failed:\n\n%s"),
                     filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}

void
file_save_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  g_return_if_fail (gimp_image_active_drawable (gdisp->gimage));

  /*  Only save if the gimage has been modified  */
  if (gdisp->gimage->dirty ||
      ! GIMP_GUI_CONFIG (gdisp->gimage->gimp->config)->trust_dirty_flag)
    {
      const gchar *uri;

      uri = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

      if (! uri)
        {
          file_save_as_cmd_callback (action, data);
        }
      else
        {
          GimpPDBStatusType  status;
          GError            *error = NULL;

          status = file_save (gdisp->gimage, action_data_get_context (data),
                              GIMP_RUN_WITH_LAST_VALS, &error);

          if (status != GIMP_PDB_SUCCESS &&
              status != GIMP_PDB_CANCEL)
            {
              gchar *filename;

              filename = file_utils_uri_to_utf8_filename (uri);

              g_message (_("Saving '%s' failed:\n\n%s"),
                         filename, error->message);
              g_clear_error (&error);

              g_free (filename);
            }
        }
    }
}

void
file_save_as_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  file_save_dialog_show (gdisp->gimage, global_menu_factory, gdisp->shell);
}

void
file_save_a_copy_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  file_save_a_copy_dialog_show (gdisp->gimage, global_menu_factory,
                                gdisp->shell);
}

void
file_save_template_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_string_box (_("Create New Template"),
                                gdisp->shell,
                                gimp_standard_help_func,
                                GIMP_HELP_FILE_SAVE_AS_TEMPLATE,
                                _("Enter a name for this template"),
                                NULL,
                                G_OBJECT (gdisp->gimage), "disconnect",
                                file_new_template_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
file_revert_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *query_box;
  const gchar *uri;
  return_if_no_display (gdisp, data);

  uri = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

  query_box = g_object_get_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY);

  if (! uri)
    {
      g_message (_("Revert failed. No file name associated with this image."));
    }
  else if (query_box)
    {
      gtk_window_present (GTK_WINDOW (query_box->window));
    }
  else
    {
      gchar *basename;
      gchar *text;

      basename = g_path_get_basename (uri);

      text = g_strdup_printf (_("Revert '%s' to\n"
                                "'%s'?\n\n"
                                "You will lose all your changes, "
                                "including all undo information."),
                              basename, uri);

      g_free (basename);

      query_box = gimp_query_boolean_box (_("Revert Image"),
                                          gdisp->shell,
                                          gimp_standard_help_func,
                                          GIMP_HELP_FILE_REVERT,
                                          GIMP_STOCK_QUESTION,
                                          text,
                                          GTK_STOCK_YES, GTK_STOCK_NO,
                                          G_OBJECT (gdisp->gimage),
                                          "disconnect",
                                          file_revert_confirm_callback,
                                          gdisp->gimage);
      
      g_free (text);

      g_object_set_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY,
                         query_box);
      
      gtk_window_set_transient_for (GTK_WINDOW (query_box),
                                    GTK_WINDOW (gdisp->shell));
      
      gtk_widget_show (query_box);
    }
}

void
file_quit_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  gimp_exit (gimp, FALSE);
}

void
file_file_open_dialog (Gimp        *gimp,
                       const gchar *uri,
                       GtkWidget   *parent)
{
  file_open_dialog_show (gimp, NULL, uri, global_menu_factory, parent);
}


/*  private functions  */

static void
file_new_template_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpTemplate *template;
  GimpImage    *gimage;

  gimage = (GimpImage *) data;

  if (! (name && strlen (name)))
    name = _("(Unnamed Template)");

  template = gimp_template_new (name);
  gimp_template_set_from_image (template, gimage);
  gimp_container_add (gimage->gimp->templates, GIMP_OBJECT (template));
  g_object_unref (template);
}

static void
file_revert_confirm_callback (GtkWidget *widget,
                              gboolean   revert,
                              gpointer   data)
{
  GimpImage *old_gimage = GIMP_IMAGE (data);

  g_object_set_data (G_OBJECT (old_gimage), REVERT_DATA_KEY, NULL);

  if (revert)
    {
      Gimp              *gimp;
      GimpImage         *new_gimage;
      const gchar       *uri;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      gimp = old_gimage->gimp;

      uri = gimp_object_get_name (GIMP_OBJECT (old_gimage));

      new_gimage = file_open_image (gimp, gimp_get_user_context (gimp),
                                    uri, uri, NULL,
                                    GIMP_RUN_INTERACTIVE,
                                    &status, NULL, &error);

      if (new_gimage)
        {
          GList *contexts = NULL;
          GList *list;

          /*  remember which contexts refer to old_gimage  */
          for (list = gimp->context_list; list; list = g_list_next (list))
            {
              GimpContext *context = list->data;

              if (gimp_context_get_image (context) == old_gimage)
                contexts = g_list_prepend (contexts, list->data);
            }

          gimp_displays_reconnect (gimp, old_gimage, new_gimage);
          gimp_image_flush (new_gimage);

          /*  set the new_gimage on the remembered contexts (in reverse
           *  order, since older contexts are usually the parents of
           *  newer ones)
           */
          g_list_foreach (contexts, (GFunc) gimp_context_set_image, new_gimage);
          g_list_free (contexts);

          /*  the displays own the image now  */
          g_object_unref (new_gimage);
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gchar *filename;

          filename = file_utils_uri_to_utf8_filename (uri);

          g_message (_("Reverting to '%s' failed:\n\n%s"),
                     filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}
