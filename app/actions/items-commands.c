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
                                     GList             *items,
                                     GList             *drawables,
                                     GimpContext       *context,
                                     GimpFillOptions   *options,
                                     gpointer           user_data);
static void   items_stroke_callback (GtkWidget         *dialog,
                                     GList             *items,
                                     GList             *drawables,
                                     GimpContext       *context,
                                     GimpStrokeOptions *options,
                                     gpointer           user_data);


/*  public functions  */

void
items_visible_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            GimpImage  *image,
                            GList      *items)
{
  GimpUndo *undo;
  gboolean  push_undo = TRUE;
  GList    *start     = NULL;
  GList    *iter;
  gboolean  visible   = g_variant_get_boolean (value);
  gint      n_items   = 0;

  for (iter = items; iter; iter = iter->next)
    {
      if (visible && gimp_item_get_visible (iter->data))
        {
          /* If any of the items are already visible, we don't
           * toggle the selection visibility.
           */
          return;
        }
    }

  for (iter = items; iter; iter = iter->next)
    if (visible != gimp_item_get_visible (iter->data))
      {
        if (start == NULL)
          start = iter;
        n_items++;
      }

  if (n_items == 0)
    {
      return;
    }
  else if (n_items == 1)
    {
      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (start->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_VISIBILITY,
                                   "Item visibility");
    }

  for (iter = start; iter; iter = iter->next)
    if (visible != gimp_item_get_visible (iter->data))
      gimp_item_set_visible (iter->data, visible, push_undo);

  if (n_items != 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
items_lock_content_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 GimpImage  *image,
                                 GList      *items)
{
  GList    *locked_items = NULL;
  GList    *iter;
  gchar    *undo_label;
  gboolean  locked       = g_variant_get_boolean (value);

  for (iter = items; iter; iter = iter->next)
    if (gimp_item_can_lock_content (iter->data))
      {
        if (! locked && ! gimp_item_get_lock_content (iter->data))
          {
            /* When unlocking, we expect all selected items to be locked. */
            g_list_free (locked_items);
            return;
          }
        else if (locked != gimp_item_get_lock_content (iter->data))
          {
            locked_items = g_list_prepend (locked_items, iter->data);
          }
      }

  if (! locked_items)
    return;

  if (locked)
    undo_label = _("Lock content");
  else
    undo_label = _("Unlock content");

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                               undo_label);

  for (iter = locked_items; iter; iter = iter->next)
    gimp_item_set_lock_content (iter->data, locked, TRUE);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (locked_items);
}

void
items_lock_position_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  GimpImage  *image,
                                  GList      *items)
{
  GList    *iter;
  GList    *locked_items = NULL;
  gchar    *undo_label;
  gboolean  locked       = g_variant_get_boolean (value);

  for (iter = items; iter; iter = iter->next)
    if (gimp_item_can_lock_position (iter->data))
      {
        if (! locked && ! gimp_item_get_lock_position (iter->data))
          {
           /* When unlocking, we expect all selected items to be locked. */
            g_list_free (locked_items);
            return;
          }
        else if (locked != gimp_item_get_lock_position (iter->data))
          {
            locked_items = g_list_prepend (locked_items, iter->data);
          }
      }

  if (! locked_items)
    return;

  if (locked)
    undo_label = _("Lock position");
  else
    undo_label = _("Unlock position");

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_LOCK_POSITION,
                               undo_label);

  for (iter = locked_items; iter; iter = iter->next)
    gimp_item_set_lock_position (iter->data, locked, TRUE);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (locked_items);
}

void
items_color_tag_cmd_callback (GimpAction   *action,
                              GimpImage    *image,
                              GList        *items,
                              GimpColorTag  color_tag)
{
  GimpUndo *undo;
  gboolean  push_undo = TRUE;
  GList    *iter;

  if (g_list_length (items) == 1)
    {
      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_COLOR_TAG);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (items->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   "Item color tag");
    }

  for (iter = items; iter; iter = iter->next)
    if (color_tag != gimp_item_get_color_tag (iter->data))
      gimp_item_set_color_tag (iter->data, color_tag, push_undo);

  if (g_list_length (items) != 1)
    gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
items_fill_cmd_callback (GimpAction  *action,
                         GimpImage   *image,
                         GList       *items,
                         const gchar *dialog_title,
                         const gchar *dialog_icon_name,
                         const gchar *dialog_help_id,
                         gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There are no selected items to fill."));
      return;
    }

  dialog = fill_dialog_new (items,
                            drawables,
                            action_data_get_context (data),
                            dialog_title,
                            dialog_icon_name,
                            dialog_help_id,
                            widget,
                            GIMP_DIALOG_CONFIG (image->gimp->config)->fill_options,
                            items_fill_callback,
                            NULL);

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_fill_last_vals_cmd_callback (GimpAction *action,
                                   GimpImage  *image,
                                   GList      *items,
                                   gpointer    data)
{
  GList            *drawables;
  GimpDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There are no selected layers or channels to fill."));
      return;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Fill");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_fill (iter->data, drawables,
                          config->fill_options, TRUE, NULL, &error))
      {
        gimp_message_literal (image->gimp, G_OBJECT (widget),
                              GIMP_MESSAGE_WARNING, error->message);
        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  g_list_free (drawables);
}

void
items_stroke_cmd_callback (GimpAction  *action,
                           GimpImage   *image,
                           GList       *items,
                           const gchar *dialog_title,
                           const gchar *dialog_icon_name,
                           const gchar *dialog_help_id,
                           gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }


  dialog = stroke_dialog_new (items,
                              drawables,
                              action_data_get_context (data),
                              dialog_title,
                              dialog_icon_name,
                              dialog_help_id,
                              widget,
                              GIMP_DIALOG_CONFIG (image->gimp->config)->stroke_options,
                              items_stroke_callback,
                              NULL);

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_stroke_last_vals_cmd_callback (GimpAction *action,
                                     GimpImage  *image,
                                     GList      *items,
                                     gpointer    data)
{
  GList            *drawables;
  GimpDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Stroke");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_stroke (iter->data, drawables,
                            action_data_get_context (data),
                            config->stroke_options, NULL,
                            TRUE, NULL, &error))
      {
        gimp_message_literal (image->gimp, G_OBJECT (widget),
                              GIMP_MESSAGE_WARNING, error->message);
        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  g_list_free (drawables);
}


/*  private functions  */

static void
items_fill_callback (GtkWidget       *dialog,
                     GList           *items,
                     GList           *drawables,
                     GimpContext     *context,
                     GimpFillOptions *options,
                     gpointer         user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (items->data);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Fill");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_fill (iter->data, drawables, options, TRUE, NULL, &error))
      {
        gimp_message_literal (context->gimp,
                              G_OBJECT (dialog),
                              GIMP_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
items_stroke_callback (GtkWidget         *dialog,
                       GList             *items,
                       GList             *drawables,
                       GimpContext       *context,
                       GimpStrokeOptions *options,
                       gpointer           data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (items->data);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Stroke");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_stroke (iter->data, drawables, context, options, NULL,
                            TRUE, NULL, &error))
      {
        gimp_message_literal (context->gimp,
                              G_OBJECT (dialog),
                              GIMP_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}
