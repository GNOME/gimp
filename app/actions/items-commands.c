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

#include "libligmaconfig/ligmaconfig.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaitem.h"
#include "core/ligmaitemundo.h"

#include "dialogs/dialogs.h"
#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "actions.h"
#include "items-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   items_fill_callback   (GtkWidget         *dialog,
                                     LigmaItem          *item,
                                     GList             *drawables,
                                     LigmaContext       *context,
                                     LigmaFillOptions   *options,
                                     gpointer           user_data);
static void   items_stroke_callback (GtkWidget         *dialog,
                                     LigmaItem          *item,
                                     GList             *drawables,
                                     LigmaContext       *context,
                                     LigmaStrokeOptions *options,
                                     gpointer           user_data);


/*  public functions  */

void
items_visible_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            LigmaImage  *image,
                            LigmaItem   *item)
{
  gboolean visible = g_variant_get_boolean (value);

  if (visible != ligma_item_get_visible (item))
    {
      LigmaUndo *undo;
      gboolean  push_undo = TRUE;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_VISIBILITY);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      ligma_item_set_visible (item, visible, push_undo);
      ligma_image_flush (image);
    }
}

void
items_lock_content_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 LigmaImage  *image,
                                 LigmaItem   *item)
{
  gboolean locked = g_variant_get_boolean (value);

  if (locked != ligma_item_get_lock_content (item))
    {
      LigmaUndo *undo;
      gboolean  push_undo = TRUE;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_LOCK_CONTENT);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      ligma_item_set_lock_content (item, locked, push_undo);
      ligma_image_flush (image);
    }
}

void
items_lock_position_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  LigmaImage  *image,
                                  LigmaItem   *item)
{
  gboolean locked = g_variant_get_boolean (value);

  if (locked != ligma_item_get_lock_position (item))
    {
      LigmaUndo *undo;
      gboolean  push_undo = TRUE;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_LOCK_POSITION);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;


      ligma_item_set_lock_position (item, locked, push_undo);
      ligma_image_flush (image);
    }
}

void
items_color_tag_cmd_callback (LigmaAction   *action,
                              LigmaImage    *image,
                              LigmaItem     *item,
                              LigmaColorTag  color_tag)
{
  if (color_tag != ligma_item_get_color_tag (item))
    {
      LigmaUndo *undo;
      gboolean  push_undo = TRUE;

      undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_ITEM_UNDO,
                                           LIGMA_UNDO_ITEM_COLOR_TAG);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      ligma_item_set_color_tag (item, color_tag, push_undo);
      ligma_image_flush (image);
    }
}

void
items_fill_cmd_callback (LigmaAction  *action,
                         LigmaImage   *image,
                         LigmaItem    *item,
                         const gchar *dialog_key,
                         const gchar *dialog_title,
                         const gchar *dialog_icon_name,
                         const gchar *dialog_help_id,
                         gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to fill."));
      return;
    }

  dialog = dialogs_get_dialog (G_OBJECT (item), dialog_key);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = fill_dialog_new (item,
                                drawables,
                                action_data_get_context (data),
                                dialog_title,
                                dialog_icon_name,
                                dialog_help_id,
                                widget,
                                config->fill_options,
                                items_fill_callback,
                                NULL);

      dialogs_attach_dialog (G_OBJECT (item), dialog_key, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_fill_last_vals_cmd_callback (LigmaAction *action,
                                   LigmaImage  *image,
                                   LigmaItem   *item,
                                   gpointer    data)
{
  GList            *drawables;
  LigmaDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to fill."));
      return;
    }

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  if (! ligma_item_fill (item, drawables,
                        config->fill_options, TRUE, NULL, &error))
    {
      ligma_message_literal (image->ligma, G_OBJECT (widget),
                            LIGMA_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }
  else
    {
      ligma_image_flush (image);
    }
  g_list_free (drawables);
}

void
items_stroke_cmd_callback (LigmaAction  *action,
                           LigmaImage   *image,
                           LigmaItem    *item,
                           const gchar *dialog_key,
                           const gchar *dialog_title,
                           const gchar *dialog_icon_name,
                           const gchar *dialog_help_id,
                           gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }

  dialog = dialogs_get_dialog (G_OBJECT (item), dialog_key);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = stroke_dialog_new (item,
                                  drawables,
                                  action_data_get_context (data),
                                  dialog_title,
                                  dialog_icon_name,
                                  dialog_help_id,
                                  widget,
                                  config->stroke_options,
                                  items_stroke_callback,
                                  NULL);

      dialogs_attach_dialog (G_OBJECT (item), dialog_key, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_stroke_last_vals_cmd_callback (LigmaAction *action,
                                     LigmaImage  *image,
                                     LigmaItem   *item,
                                     gpointer    data)
{
  GList            *drawables;
  LigmaDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  if (! ligma_item_stroke (item, drawables,
                          action_data_get_context (data),
                          config->stroke_options, NULL,
                          TRUE, NULL, &error))
    {
      ligma_message_literal (image->ligma, G_OBJECT (widget),
                            LIGMA_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }
  else
    {
      ligma_image_flush (image);
    }

  g_list_free (drawables);
}


/*  private functions  */

static void
items_fill_callback (GtkWidget       *dialog,
                     LigmaItem        *item,
                     GList           *drawables,
                     LigmaContext     *context,
                     LigmaFillOptions *options,
                     gpointer         user_data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (context->ligma->config);
  LigmaImage        *image  = ligma_item_get_image (item);
  GError           *error  = NULL;

  ligma_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  if (! ligma_item_fill (item, drawables, options, TRUE, NULL, &error))
    {
      ligma_message_literal (context->ligma,
                            G_OBJECT (dialog),
                            LIGMA_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
items_stroke_callback (GtkWidget         *dialog,
                       LigmaItem          *item,
                       GList             *drawables,
                       LigmaContext       *context,
                       LigmaStrokeOptions *options,
                       gpointer           data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (context->ligma->config);
  LigmaImage        *image  = ligma_item_get_image (item);
  GError           *error  = NULL;

  ligma_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  if (! ligma_item_stroke (item, drawables, context, options, NULL,
                          TRUE, NULL, &error))
    {
      ligma_message_literal (context->ligma,
                            G_OBJECT (dialog),
                            LIGMA_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}
