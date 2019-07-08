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

#include "libgimpconfig/gimpconfig.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem.h"
#include "core/gimpitemundo.h"

#include "dialogs/dialogs.h"
#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "actions.h"
#include "items-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   items_fill_callback   (GtkWidget         *dialog,
                                     GimpItem          *item,
                                     GimpDrawable      *drawable,
                                     GimpContext       *context,
                                     GimpFillOptions   *options,
                                     gpointer           user_data);
static void   items_stroke_callback (GtkWidget         *dialog,
                                     GimpItem          *item,
                                     GimpDrawable      *drawable,
                                     GimpContext       *context,
                                     GimpStrokeOptions *options,
                                     gpointer           user_data);


/*  public functions  */

void
items_visible_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            GimpImage  *image,
                            GimpItem   *item)
{
  gboolean visible = g_variant_get_boolean (value);

  if (visible != gimp_item_get_visible (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_visible (item, visible, push_undo);
      gimp_image_flush (image);
    }
}

void
items_linked_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           GimpImage  *image,
                           GimpItem   *item)
{
  gboolean linked = g_variant_get_boolean (value);

  if (linked != gimp_item_get_linked (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_linked (item, linked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_lock_content_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 GimpImage  *image,
                                 GimpItem   *item)
{
  gboolean locked = g_variant_get_boolean (value);

  if (locked != gimp_item_get_lock_content (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_lock_content (item, locked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_lock_position_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  GimpImage  *image,
                                  GimpItem   *item)
{
  gboolean locked = g_variant_get_boolean (value);

  if (locked != gimp_item_get_lock_position (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LOCK_POSITION);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;


      gimp_item_set_lock_position (item, locked, push_undo);
      gimp_image_flush (image);
    }
}

void
items_color_tag_cmd_callback (GimpAction   *action,
                              GimpImage    *image,
                              GimpItem     *item,
                              GimpColorTag  color_tag)
{
  if (color_tag != gimp_item_get_color_tag (item))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_COLOR_TAG);

      if (undo && GIMP_ITEM_UNDO (undo)->item == item)
        push_undo = FALSE;

      gimp_item_set_color_tag (item, color_tag, push_undo);
      gimp_image_flush (image);
    }
}

void
items_fill_cmd_callback (GimpAction  *action,
                         GimpImage   *image,
                         GimpItem    *item,
                         const gchar *dialog_key,
                         const gchar *dialog_title,
                         const gchar *dialog_icon_name,
                         const gchar *dialog_help_id,
                         gpointer     data)
{
  GimpDrawable *drawable;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to fill."));
      return;
    }

  dialog = dialogs_get_dialog (G_OBJECT (item), dialog_key);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = fill_dialog_new (item,
                                drawable,
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
}

void
items_fill_last_vals_cmd_callback (GimpAction *action,
                                   GimpImage  *image,
                                   GimpItem   *item,
                                   gpointer    data)
{
  GimpDrawable     *drawable;
  GimpDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to fill."));
      return;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  if (! gimp_item_fill (item, drawable,
                        config->fill_options, TRUE, NULL, &error))
    {
      gimp_message_literal (image->gimp, G_OBJECT (widget),
                            GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_image_flush (image);
    }
}

void
items_stroke_cmd_callback (GimpAction  *action,
                           GimpImage   *image,
                           GimpItem    *item,
                           const gchar *dialog_key,
                           const gchar *dialog_title,
                           const gchar *dialog_icon_name,
                           const gchar *dialog_help_id,
                           gpointer     data)
{
  GimpDrawable *drawable;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to stroke to."));
      return;
    }

  dialog = dialogs_get_dialog (G_OBJECT (item), dialog_key);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = stroke_dialog_new (item,
                                  drawable,
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
}

void
items_stroke_last_vals_cmd_callback (GimpAction *action,
                                     GimpImage  *image,
                                     GimpItem   *item,
                                     gpointer    data)
{
  GimpDrawable     *drawable;
  GimpDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to stroke to."));
      return;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  if (! gimp_item_stroke (item, drawable,
                          action_data_get_context (data),
                          config->stroke_options, NULL,
                          TRUE, NULL, &error))
    {
      gimp_message_literal (image->gimp, G_OBJECT (widget),
                            GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_image_flush (image);
    }
}


/*  private functions  */

static void
items_fill_callback (GtkWidget       *dialog,
                     GimpItem        *item,
                     GimpDrawable    *drawable,
                     GimpContext     *context,
                     GimpFillOptions *options,
                     gpointer         user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (item);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  if (! gimp_item_fill (item, drawable, options, TRUE, NULL, &error))
    {
      gimp_message_literal (context->gimp,
                            G_OBJECT (dialog),
                            GIMP_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
items_stroke_callback (GtkWidget         *dialog,
                       GimpItem          *item,
                       GimpDrawable      *drawable,
                       GimpContext       *context,
                       GimpStrokeOptions *options,
                       gpointer           data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (item);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  if (! gimp_item_stroke (item, drawable, context, options, NULL,
                          TRUE, NULL, &error))
    {
      gimp_message_literal (context->gimp,
                            G_OBJECT (dialog),
                            GIMP_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}
