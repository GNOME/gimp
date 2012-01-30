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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpwindowstrategy.h"

#include "dialogs/data-delete-dialog.h"

#include "actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
data_open_as_image_cmd_callback (GtkAction *action,
                                 gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context =
    gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_children_type (view));

  if (data && gimp_data_get_filename (data))
    {
      gchar *uri = g_filename_to_uri (gimp_data_get_filename (data), NULL, NULL);

      if (uri)
        {
          GimpImage         *image;
          GimpPDBStatusType  status;
          GError            *error = NULL;

          image = file_open_with_display (context->gimp, context, NULL,
                                          uri, FALSE,
                                          &status, &error);

          if (! image && status != GIMP_PDB_CANCEL)
            {
              gchar *filename = file_utils_uri_display_name (uri);

              gimp_message (context->gimp, G_OBJECT (view),
                            GIMP_MESSAGE_ERROR,
                            _("Opening '%s' failed:\n\n%s"),
                            filename, error->message);
              g_clear_error (&error);

              g_free (filename);
            }

          g_free (uri);
        }
    }
}

void
data_new_cmd_callback (GtkAction *action,
                       gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);

  if (gimp_data_factory_view_has_data_new_func (view))
    {
      GimpDataFactory *factory;
      GimpContext     *context;
      GimpData        *data;

      factory = gimp_data_factory_view_get_data_factory (view);

      context =
        gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

      data = gimp_data_factory_data_new (factory, context, _("Untitled"));

      if (data)
        {
          gimp_context_set_by_type (context,
                                    gimp_data_factory_view_get_children_type (view),
                                    GIMP_OBJECT (data));

          gtk_button_clicked (GTK_BUTTON (gimp_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_duplicate_cmd_callback (GtkAction *action,
                             gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_children_type (view));

  if (data && gimp_data_factory_view_have (view,
                                           GIMP_OBJECT (data)))
    {
      GimpData *new_data;

      new_data = gimp_data_factory_data_duplicate (gimp_data_factory_view_get_data_factory (view), data);

      if (new_data)
        {
          gimp_context_set_by_type (context,
                                    gimp_data_factory_view_get_children_type (view),
                                    GIMP_OBJECT (new_data));

          gtk_button_clicked (GTK_BUTTON (gimp_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_copy_location_cmd_callback (GtkAction *action,
                                 gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_children_type (view));

  if (data)
    {
      const gchar *filename = gimp_data_get_filename (data);

      if (filename && *filename)
        {
          gchar *uri = g_filename_to_uri (filename, NULL, NULL);

          if (uri)
            {
              gimp_clipboard_set_text (context->gimp, uri);
              g_free (uri);
            }
        }
    }
}

void
data_delete_cmd_callback (GtkAction *action,
                          gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context =
    gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_children_type (view));

  if (data                          &&
      gimp_data_is_deletable (data) &&
      gimp_data_factory_view_have (view,
                                   GIMP_OBJECT (data)))
    {
      GimpDataFactory *factory;
      GtkWidget       *dialog;

      factory = gimp_data_factory_view_get_data_factory (view);

      dialog = data_delete_dialog_new (factory, data, context,
                                       GTK_WIDGET (view));
      gtk_widget_show (dialog);
    }
}

void
data_refresh_cmd_callback (GtkAction *action,
                           gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  Gimp                *gimp;
  return_if_no_gimp (gimp, user_data);

  gimp_set_busy (gimp);
  gimp_data_factory_data_refresh (gimp_data_factory_view_get_data_factory (view),
                                  action_data_get_context (user_data));
  gimp_unset_busy (gimp);
}

void
data_edit_cmd_callback (GtkAction   *action,
                        const gchar *value,
                        gpointer     user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_children_type (view));

  if (data && gimp_data_factory_view_have (view,
                                           GIMP_OBJECT (data)))
    {
      GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (view));
      GtkWidget *dockable;

      dockable =
        gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                                   context->gimp,
                                                   gimp_dialog_factory_get_singleton (),
                                                   screen,
                                                   value);

      gimp_data_editor_set_data (GIMP_DATA_EDITOR (gtk_bin_get_child (GTK_BIN (dockable))),
                                 data);
    }
}
