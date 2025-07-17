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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "file/file-open.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs/data-delete-dialog.h"

#include "actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
data_open_as_image_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context =
    gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data && gimp_data_get_file (data))
    {
      GFile             *file   = gimp_data_get_file (data);
      GtkWidget         *widget = GTK_WIDGET (view);
      GimpImage         *image;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      image = file_open_with_display (context->gimp, context, NULL,
                                      file, FALSE,
                                      G_OBJECT (gimp_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != GIMP_PDB_CANCEL)
        {
          gimp_message (context->gimp, G_OBJECT (view),
                        GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}

void
data_new_cmd_callback (GimpAction *action,
                       GVariant   *value,
                       gpointer    user_data)
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
          GtkWidget *edit_button;

          gimp_context_set_by_type (context,
                                    gimp_data_factory_view_get_child_type (view),
                                    GIMP_OBJECT (data));

          edit_button = gimp_data_factory_view_get_edit_button (view);
          if (edit_button && gtk_widget_get_visible (edit_button))
            gtk_button_clicked (GTK_BUTTON (gimp_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_duplicate_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data && gimp_data_factory_view_have (view, GIMP_OBJECT (data)))
    {
      GimpData *new_data;

      new_data = gimp_data_factory_data_duplicate (gimp_data_factory_view_get_data_factory (view), data);

      if (new_data)
        {
          GtkWidget *edit_button;

          gimp_context_set_by_type (context,
                                    gimp_data_factory_view_get_child_type (view),
                                    GIMP_OBJECT (new_data));

          edit_button = gimp_data_factory_view_get_edit_button (view);
          if (edit_button && gtk_widget_get_visible (edit_button))
            gtk_button_clicked (GTK_BUTTON (gimp_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_copy_location_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data)
    {
      GFile *file = gimp_data_get_file (data);

      if (file)
        {
          gchar *uri = g_file_get_uri (file);

          gimp_clipboard_set_text (context->gimp, uri);
          g_free (uri);
        }
    }
}

void
data_show_in_file_manager_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data)
    {
      GFile *file = gimp_data_get_file (data);

      if (file)
        {
          GError *error = NULL;

          if (! gimp_file_show_in_file_manager (file, &error))
            {
              gimp_message (context->gimp, G_OBJECT (view),
                            GIMP_MESSAGE_ERROR,
                            _("Can't show file in file manager: %s"),
                            error->message);
              g_clear_error (&error);
            }
        }
    }
}

void
data_delete_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context =
    gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data                          &&
      gimp_data_is_deletable (data) &&
      gimp_data_factory_view_have (view, GIMP_OBJECT (data)))
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
data_refresh_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    user_data)
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
data_edit_cmd_callback (GimpAction *action,
                        GVariant   *value,
                        gpointer    user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
                              gimp_data_factory_view_get_child_type (view));

  if (data && gimp_data_factory_view_have (view, GIMP_OBJECT (data)))
    {
      GdkMonitor *monitor = gimp_widget_get_monitor (GTK_WIDGET (view));
      GtkWidget  *dockable;
      GtkWidget  *editor  = NULL;

      dockable =
        gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                                   context->gimp,
                                                   gimp_dialog_factory_get_singleton (),
                                                   monitor,
                                                   g_variant_get_string (value,
                                                                         NULL));

      if (dockable)
        editor = gtk_bin_get_child (GTK_BIN (dockable));

      if (editor && GIMP_IS_DATA_EDITOR (editor))
        gimp_data_editor_set_data (GIMP_DATA_EDITOR (editor), data);
    }
}
