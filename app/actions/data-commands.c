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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadata.h"
#include "core/ligmadatafactory.h"

#include "file/file-open.h"

#include "widgets/ligmaclipboard.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadataeditor.h"
#include "widgets/ligmadatafactoryview.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"
#include "widgets/ligmawidgets-utils.h"

#include "dialogs/data-delete-dialog.h"

#include "actions.h"
#include "data-commands.h"

#include "ligma-intl.h"


/*  public functions  */

void
data_open_as_image_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context =
    ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data && ligma_data_get_file (data))
    {
      GFile             *file   = ligma_data_get_file (data);
      GtkWidget         *widget = GTK_WIDGET (view);
      LigmaImage         *image;
      LigmaPDBStatusType  status;
      GError            *error = NULL;

      image = file_open_with_display (context->ligma, context, NULL,
                                      file, FALSE,
                                      G_OBJECT (ligma_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != LIGMA_PDB_CANCEL)
        {
          ligma_message (context->ligma, G_OBJECT (view),
                        LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}

void
data_new_cmd_callback (LigmaAction *action,
                       GVariant   *value,
                       gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);

  if (ligma_data_factory_view_has_data_new_func (view))
    {
      LigmaDataFactory *factory;
      LigmaContext     *context;
      LigmaData        *data;

      factory = ligma_data_factory_view_get_data_factory (view);

      context =
        ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

      data = ligma_data_factory_data_new (factory, context, _("Untitled"));

      if (data)
        {
          ligma_context_set_by_type (context,
                                    ligma_data_factory_view_get_children_type (view),
                                    LIGMA_OBJECT (data));

          gtk_button_clicked (GTK_BUTTON (ligma_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_duplicate_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context = ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data && ligma_data_factory_view_have (view, LIGMA_OBJECT (data)))
    {
      LigmaData *new_data;

      new_data = ligma_data_factory_data_duplicate (ligma_data_factory_view_get_data_factory (view), data);

      if (new_data)
        {
          ligma_context_set_by_type (context,
                                    ligma_data_factory_view_get_children_type (view),
                                    LIGMA_OBJECT (new_data));

          gtk_button_clicked (GTK_BUTTON (ligma_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_copy_location_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context = ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data)
    {
      GFile *file = ligma_data_get_file (data);

      if (file)
        {
          gchar *uri = g_file_get_uri (file);

          ligma_clipboard_set_text (context->ligma, uri);
          g_free (uri);
        }
    }
}

void
data_show_in_file_manager_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context = ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data)
    {
      GFile *file = ligma_data_get_file (data);

      if (file)
        {
          GError *error = NULL;

          if (! ligma_file_show_in_file_manager (file, &error))
            {
              ligma_message (context->ligma, G_OBJECT (view),
                            LIGMA_MESSAGE_ERROR,
                            _("Can't show file in file manager: %s"),
                            error->message);
              g_clear_error (&error);
            }
        }
    }
}

void
data_delete_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context =
    ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data                          &&
      ligma_data_is_deletable (data) &&
      ligma_data_factory_view_have (view, LIGMA_OBJECT (data)))
    {
      LigmaDataFactory *factory;
      GtkWidget       *dialog;

      factory = ligma_data_factory_view_get_data_factory (view);

      dialog = data_delete_dialog_new (factory, data, context,
                                       GTK_WIDGET (view));
      gtk_widget_show (dialog);
    }
}

void
data_refresh_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  Ligma                *ligma;
  return_if_no_ligma (ligma, user_data);

  ligma_set_busy (ligma);
  ligma_data_factory_data_refresh (ligma_data_factory_view_get_data_factory (view),
                                  action_data_get_context (user_data));
  ligma_unset_busy (ligma);
}

void
data_edit_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    user_data)
{
  LigmaDataFactoryView *view = LIGMA_DATA_FACTORY_VIEW (user_data);
  LigmaContext         *context;
  LigmaData            *data;

  context = ligma_container_view_get_context (LIGMA_CONTAINER_EDITOR (view)->view);

  data = (LigmaData *)
    ligma_context_get_by_type (context,
                              ligma_data_factory_view_get_children_type (view));

  if (data && ligma_data_factory_view_have (view, LIGMA_OBJECT (data)))
    {
      GdkMonitor *monitor = ligma_widget_get_monitor (GTK_WIDGET (view));
      GtkWidget  *dockable;

      dockable =
        ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
                                                   context->ligma,
                                                   ligma_dialog_factory_get_singleton (),
                                                   monitor,
                                                   g_variant_get_string (value,
                                                                         NULL));

      ligma_data_editor_set_data (LIGMA_DATA_EDITOR (gtk_bin_get_child (GTK_BIN (dockable))),
                                 data);
    }
}
