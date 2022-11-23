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
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligma-edit.h"
#include "core/ligmabuffer.h"
#include "core/ligmacontainer.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-edit.h"
#include "core/ligmafilloptions.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"
#include "core/ligmalayermask.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"

#include "vectors/ligmavectors-import.h"

#include "widgets/ligmaclipboard.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-transform.h"

#include "tools/ligmatools-utils.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "edit-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static gboolean   check_drawable_alpha               (LigmaDrawable  *drawable,
                                                      gpointer       data);
static void       edit_paste                         (LigmaDisplay   *display,
                                                      LigmaPasteType  paste_type,
                                                      gboolean       merged,
                                                      gboolean       try_svg);
static void       cut_named_buffer_callback          (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);
static void       copy_named_buffer_callback         (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);
static void       copy_named_visible_buffer_callback (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);


/*  public functions  */

void
edit_undo_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_undo_active (image->ligma, display) ||
      ligma_image_undo (image))
    {
      ligma_image_flush (image);
    }
}

void
edit_redo_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_redo_active (image->ligma, display) ||
      ligma_image_redo (image))
    {
      ligma_image_flush (image);
    }
}

void
edit_strong_undo_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  if (ligma_image_strong_undo (image))
    ligma_image_flush (image);
}

void
edit_strong_redo_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  if (ligma_image_strong_redo (image))
    ligma_image_flush (image);
}

void
edit_undo_clear_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage     *image;
  LigmaUndoStack *undo_stack;
  LigmaUndoStack *redo_stack;
  GtkWidget     *widget;
  GtkWidget     *dialog;
  gchar         *size;
  gint64         memsize;
  gint64         guisize;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = ligma_message_dialog_new (_("Clear Undo History"),
                                    LIGMA_ICON_DIALOG_WARNING,
                                    widget,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    ligma_standard_help_func,
                                    LIGMA_HELP_EDIT_UNDO_CLEAR,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (widget), "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("Really clear image's undo history?"));

  undo_stack = ligma_image_get_undo_stack (image);
  redo_stack = ligma_image_get_redo_stack (image);

  memsize =  ligma_object_get_memsize (LIGMA_OBJECT (undo_stack), &guisize);
  memsize += guisize;
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (redo_stack), &guisize);
  memsize += guisize;

  size = g_format_size (memsize);

  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the undo history of this "
                               "image will gain %s of memory."), size);
  g_free (size);

  if (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      ligma_image_undo_disable (image);
      ligma_image_undo_enable (image);
      ligma_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

void
edit_cut_cmd_callback (LigmaAction *action,
                       GVariant   *value,
                       gpointer    data)
{
  LigmaImage    *image;
  GList        *drawables;
  GList        *iter;
  LigmaObject   *cut;
  GError       *error = NULL;
  return_if_no_drawables (image, drawables, data);

  for (iter = drawables; iter; iter = iter->next)
    if (! check_drawable_alpha (iter->data, data))
      {
        g_list_free (drawables);
        return;
      }

  cut = ligma_edit_cut (image, drawables, action_data_get_context (data), &error);

  if (cut)
    {
      LigmaDisplay *display = action_data_get_display (data);

      if (display)
        {
          gchar *msg;

          if (LIGMA_IS_IMAGE (cut))
            msg = g_strdup_printf (ngettext ("Cut layer to the clipboard.",
                                             "Cut %d layers to the clipboard.",
                                             g_list_length (drawables)),
                                   g_list_length (drawables));
          else
            msg = g_strdup (_("Cut pixels to the clipboard."));

          ligma_message_literal (image->ligma,
                                G_OBJECT (display), LIGMA_MESSAGE_INFO,
                                msg);
          g_free (msg);
        }

      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (action_data_get_display (data)),
                            LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

void
edit_copy_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  LigmaImage    *image;
  GList        *drawables;
  LigmaObject   *copy;
  GError       *error = NULL;
  return_if_no_drawables (image, drawables, data);

  copy = ligma_edit_copy (image, drawables, action_data_get_context (data),
                         &error);

  if (copy)
    {
      LigmaDisplay *display = action_data_get_display (data);

      if (display)
        ligma_message_literal (image->ligma,
                              G_OBJECT (display), LIGMA_MESSAGE_INFO,
                              LIGMA_IS_IMAGE (copy) ?
                              _("Copied layer to the clipboard.") :
                              _("Copied pixels to the clipboard."));

      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (action_data_get_display (data)),
                            LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }

  g_list_free (drawables);
}

void
edit_copy_visible_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage *image;
  GError    *error = NULL;
  return_if_no_image (image, data);

  if (ligma_edit_copy_visible (image, action_data_get_context (data), &error))
    {
      LigmaDisplay *display = action_data_get_display (data);

      if (display)
        ligma_message_literal (image->ligma,
                              G_OBJECT (display), LIGMA_MESSAGE_INFO,
                              _("Copied pixels to the clipboard."));

      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (action_data_get_display (data)),
                            LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
edit_paste_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaImage     *image;
  LigmaDisplay   *display    = action_data_get_display (data);
  LigmaPasteType  paste_type = (LigmaPasteType) g_variant_get_int32 (value);
  LigmaPasteType  converted_type;
  GList         *drawables;
  gboolean       merged     = FALSE;

  return_if_no_image (image, data);

  if (paste_type == LIGMA_PASTE_TYPE_FLOATING)
    {
      if (! display || ! ligma_display_get_image (display))
        {
          edit_paste_as_new_image_cmd_callback (action, value, data);
          return;
        }
    }

  if (! display)
    return;

  switch (paste_type)
    {
    case LIGMA_PASTE_TYPE_FLOATING:
    case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
    case LIGMA_PASTE_TYPE_FLOATING_INTO:
    case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
      edit_paste (display, paste_type, merged, TRUE);
      break;

    case LIGMA_PASTE_TYPE_NEW_LAYER:
    case LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE:
      edit_paste (display, paste_type, merged, FALSE);
      break;

    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      merged = TRUE;
    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
      drawables = ligma_image_get_selected_drawables (image);

      if (drawables &&
         (g_list_length (drawables) == 1) &&
          LIGMA_IS_LAYER_MASK (drawables->data))
        {
          converted_type = (paste_type == LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING ||
                            paste_type == LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING) ?
                            LIGMA_PASTE_TYPE_FLOATING :
                            LIGMA_PASTE_TYPE_FLOATING_IN_PLACE;

          edit_paste (display, converted_type, merged, TRUE);
        }
      else
        {
          converted_type = (paste_type == LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING ||
                            paste_type == LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING) ?
                            LIGMA_PASTE_TYPE_NEW_LAYER :
                            LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE;

          edit_paste (display, converted_type, merged, FALSE);
        }
      g_list_free (drawables);

      break;
    }
}

void
edit_paste_as_new_image_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  Ligma       *ligma;
  GtkWidget  *widget;
  LigmaObject *paste;
  LigmaImage  *image = NULL;
  return_if_no_ligma (ligma, data);
  return_if_no_widget (widget, data);

  paste = ligma_clipboard_get_object (ligma);

  if (paste)
    {
      image = ligma_edit_paste_as_new_image (ligma, paste);
      g_object_unref (paste);
    }

  if (image)
    {
      ligma_create_display (ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (image);
    }
  else
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_WARNING,
                            _("There is no image data in the clipboard "
                              "to paste."));
    }
}

void
edit_named_cut_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = ligma_query_string_box (_("Cut Named"), widget,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_BUFFER_CUT,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  cut_named_buffer_callback,
                                  image, NULL);
  gtk_widget_show (dialog);
}

void
edit_named_copy_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = ligma_query_string_box (_("Copy Named"), widget,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_buffer_callback,
                                  image, NULL);
  gtk_widget_show (dialog);
}

void
edit_named_copy_visible_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = ligma_query_string_box (_("Copy Visible Named"), widget,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_visible_buffer_callback,
                                  image, NULL);
  gtk_widget_show (dialog);
}

void
edit_named_paste_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  Ligma      *ligma;
  GtkWidget *widget;
  return_if_no_ligma (ligma, data);
  return_if_no_widget (widget, data);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
                                             ligma,
                                             ligma_dialog_factory_get_singleton (),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-buffer-list|ligma-buffer-grid");
}

void
edit_clear_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaImage *image;
  GList     *drawables;
  GList     *iter;

  return_if_no_drawables (image, drawables, data);

  for (iter = drawables; iter; iter = iter->next)
    /* Return if any has a locked alpha. */
    if (! check_drawable_alpha (iter->data, data))
      {
        g_list_free (drawables);
        return;
      }

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT,
                               _("Clear"));

  for (iter = drawables; iter; iter = iter->next)
    if (! ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)) &&
        ! ligma_item_is_content_locked (LIGMA_ITEM (iter->data), NULL))
      ligma_drawable_edit_clear (iter->data, action_data_get_context (data));

  ligma_image_undo_group_end (image);
  ligma_image_flush (image);
  g_list_free (drawables);
}

void
edit_fill_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  LigmaImage       *image;
  GList           *drawables;
  GList           *iter;
  LigmaFillType     fill_type;
  LigmaFillOptions *options;
  GError          *error = NULL;
  return_if_no_drawables (image, drawables, data);

  fill_type = (LigmaFillType) g_variant_get_int32 (value);

  options = ligma_fill_options_new (action_data_get_ligma (data), NULL, FALSE);

  if (ligma_fill_options_set_by_fill_type (options,
                                          action_data_get_context (data),
                                          fill_type, &error))
    {
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT,
                                   ligma_fill_options_get_undo_desc (options));

      for (iter = drawables; iter; iter = iter->next)
        ligma_drawable_edit_fill (iter->data, options, NULL);

      ligma_image_undo_group_end (image);
      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }

  g_list_free (drawables);
  g_object_unref (options);
}


/*  private functions  */

static gboolean
check_drawable_alpha (LigmaDrawable *drawable,
                      gpointer      data)
{
  LigmaLayer *locked_layer = NULL;

  if (ligma_drawable_has_alpha (drawable) &&
      LIGMA_IS_LAYER (drawable)           &&
      ligma_layer_is_alpha_locked (LIGMA_LAYER (drawable), &locked_layer))
    {
      Ligma        *ligma    = action_data_get_ligma    (data);
      LigmaDisplay *display = action_data_get_display (data);

      if (ligma && display)
        {
          ligma_message_literal (
            ligma, G_OBJECT (display), LIGMA_MESSAGE_WARNING,
            _("A selected layer's alpha channel is locked."));

          ligma_tools_blink_lock_box (ligma, LIGMA_ITEM (locked_layer));
        }

      return FALSE;
    }

  return TRUE;
}

static void
edit_paste (LigmaDisplay   *display,
            LigmaPasteType  paste_type,
            gboolean       merged,
            gboolean       try_svg)
{
  LigmaImage  *image = ligma_display_get_image (display);
  LigmaObject *paste;

  g_return_if_fail (paste_type != LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING          &&
                    paste_type != LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE &&
                    paste_type != LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING   &&
                    paste_type != LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE);

  if (try_svg)
    {
      gchar *svg;
      gsize  svg_size;

      svg = ligma_clipboard_get_svg (display->ligma, &svg_size);

      if (svg)
        {
          if (ligma_vectors_import_buffer (image, svg, svg_size,
                                          TRUE, FALSE,
                                          LIGMA_IMAGE_ACTIVE_PARENT, -1,
                                          NULL, NULL))
            {
              ligma_image_flush (image);
            }

          g_free (svg);

          return;
        }
    }

  paste = ligma_clipboard_get_object (display->ligma);

  if (paste)
    {
      LigmaDisplayShell *shell     = ligma_display_get_shell (display);
      GList            *drawables = ligma_image_get_selected_drawables (image);
      GList            *pasted_layers;
      gint              x, y;
      gint              width, height;

      if (g_list_length (drawables) != 1 ||
          (paste_type != LIGMA_PASTE_TYPE_NEW_LAYER &&
           paste_type != LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE))
        {
          if (g_list_length (drawables) != 1)
            {
              ligma_message_literal (display->ligma, G_OBJECT (display),
                                    LIGMA_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target is not a single layer or layer mask."));
            }
          else if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawables->data)))
            {
              ligma_message_literal (display->ligma, G_OBJECT (display),
                                    LIGMA_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target is a layer group."));
            }
          else if (ligma_item_is_content_locked (LIGMA_ITEM (drawables->data), NULL))
            {
              ligma_message_literal (display->ligma, G_OBJECT (display),
                                    LIGMA_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target's pixels are locked."));
            }

          /* the actual paste-type conversion happens in ligma_edit_paste() */
        }

      ligma_display_shell_untransform_viewport (
        shell,
        ! ligma_display_shell_get_infinite_canvas (shell),
        &x, &y, &width, &height);

      if ((pasted_layers = ligma_edit_paste (image, drawables, paste, paste_type,
                                            ligma_get_user_context (display->ligma),
                                            merged, x, y, width, height)))
        {
          ligma_image_set_selected_layers (image, pasted_layers);
          g_list_free (pasted_layers);
          ligma_image_flush (image);
        }

      g_list_free (drawables);
      g_object_unref (paste);
    }
  else
    {
      ligma_message_literal (display->ligma, G_OBJECT (display),
                            LIGMA_MESSAGE_WARNING,
                            _("There is no image data in the clipboard "
                              "to paste."));
    }
}

static void
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  LigmaImage *image     = LIGMA_IMAGE (data);
  GList     *drawables = ligma_image_get_selected_drawables (image);
  GError    *error     = NULL;

  if (! drawables)
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to cut from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (ligma_edit_named_cut (image, name, drawables,
                           ligma_get_user_context (image->ligma), &error))
    {
      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  LigmaImage *image     = LIGMA_IMAGE (data);
  GList     *drawables = ligma_image_get_selected_drawables (image);
  GError    *error     = NULL;

  if (! drawables)
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            _("There are no selected layers or channels to copy from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (ligma_edit_named_copy (image, name, drawables,
                            ligma_get_user_context (image->ligma), &error))
    {
      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

static void
copy_named_visible_buffer_callback (GtkWidget   *widget,
                                    const gchar *name,
                                    gpointer     data)
{
  LigmaImage *image = LIGMA_IMAGE (data);
  GError    *error = NULL;

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (ligma_edit_named_copy_visible (image, name,
                                    ligma_get_user_context (image->ligma),
                                    &error))
    {
      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}
