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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimplayer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimage-undo.h"

#include "vectors/gimpvectors-import.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "tools/tool_manager.h"

#include "dialogs/fade-dialog.h"

#include "actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   edit_paste                         (GimpDisplay *display,
                                                  gboolean     paste_into);
static void   cut_named_buffer_callback          (GtkWidget   *widget,
                                                  const gchar *name,
                                                  gpointer     data);
static void   copy_named_buffer_callback         (GtkWidget   *widget,
                                                  const gchar *name,
                                                  gpointer     data);
static void   copy_named_visible_buffer_callback (GtkWidget   *widget,
                                                  const gchar *name,
                                                  gpointer     data);


/*  public functions  */

void
edit_undo_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage   *image;
  GimpDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_undo_active (image->gimp, display) ||
      gimp_image_undo (image))
    {
      gimp_image_flush (image);
    }
}

void
edit_redo_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage   *image;
  GimpDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_redo_active (image->gimp, display) ||
      gimp_image_redo (image))
    {
      gimp_image_flush (image);
    }
}

void
edit_strong_undo_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  if (gimp_image_strong_undo (image))
    gimp_image_flush (image);
}

void
edit_strong_redo_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  if (gimp_image_strong_redo (image))
    gimp_image_flush (image);
}

void
edit_undo_clear_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage     *image;
  GimpUndoStack *undo_stack;
  GimpUndoStack *redo_stack;
  GtkWidget     *widget;
  GtkWidget     *dialog;
  gchar         *size;
  gint64         memsize;
  gint64         guisize;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = gimp_message_dialog_new (_("Clear Undo History"), GIMP_STOCK_WARNING,
                                    widget,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func,
                                    GIMP_HELP_EDIT_UNDO_CLEAR,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_CLEAR,  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (widget), "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Really clear image's undo history?"));

  undo_stack = gimp_image_get_undo_stack (image);
  redo_stack = gimp_image_get_redo_stack (image);

  memsize =  gimp_object_get_memsize (GIMP_OBJECT (undo_stack), &guisize);
  memsize += guisize;
  memsize += gimp_object_get_memsize (GIMP_OBJECT (redo_stack), &guisize);
  memsize += guisize;

  size = g_format_size (memsize);

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the undo history of this "
                               "image will gain %s of memory."), size);
  g_free (size);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gimp_image_undo_disable (image);
      gimp_image_undo_enable (image);
      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

void
edit_cut_cmd_callback (GtkAction *action,
                       gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GError       *error = NULL;
  return_if_no_drawable (image, drawable, data);

  if (gimp_edit_cut (image, drawable, action_data_get_context (data), &error))
    {
      GimpDisplay *display = action_data_get_display (data);

      if (display)
        gimp_message_literal (image->gimp,
                              G_OBJECT (display), GIMP_MESSAGE_INFO,
                              _("Cut pixels to the clipboard"));

      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (action_data_get_display (data)),
                            GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
edit_copy_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GError       *error = NULL;
  return_if_no_drawable (image, drawable, data);

  if (gimp_edit_copy (image, drawable, action_data_get_context (data), &error))
    {
      GimpDisplay *display = action_data_get_display (data);

      if (display)
        gimp_message_literal (image->gimp,
                              G_OBJECT (display), GIMP_MESSAGE_INFO,
                              _("Copied pixels to the clipboard"));

      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (action_data_get_display (data)),
                            GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
edit_copy_visible_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *image;
  GError    *error = NULL;
  return_if_no_image (image, data);

  if (gimp_edit_copy_visible (image, action_data_get_context (data), &error))
    {
      GimpDisplay *display = action_data_get_display (data);

      if (display)
        gimp_message_literal (image->gimp,
                              G_OBJECT (display), GIMP_MESSAGE_INFO,
                              _("Copied pixels to the clipboard"));

      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (action_data_get_display (data)),
                            GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
edit_paste_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpDisplay *display = action_data_get_display (data);

  if (display && gimp_display_get_image (display))
    edit_paste (display, FALSE);
  else
    edit_paste_as_new_cmd_callback (action, data);
}

void
edit_paste_into_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  edit_paste (display, TRUE);
}

void
edit_paste_as_new_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp       *gimp;
  GimpBuffer *buffer;
  GtkWidget  *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  buffer = gimp_clipboard_get_buffer (gimp);

  if (buffer)
    {
      GimpImage *image;

      image = gimp_image_new_from_buffer (gimp, action_data_get_image (data),
                                          buffer);
      g_object_unref (buffer);

      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0,
                           G_OBJECT (gtk_widget_get_screen (widget)),
                           gimp_widget_get_monitor (widget));
      g_object_unref (image);
    }
  else
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING,
                            _("There is no image data in the clipboard to paste."));
    }
}

void
edit_paste_as_new_layer_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  Gimp       *gimp;
  GimpImage  *image;
  GimpBuffer *buffer;
  return_if_no_gimp (gimp, data);
  return_if_no_image (image, data);

  buffer = gimp_clipboard_get_buffer (gimp);

  if (buffer)
    {
      GimpLayer *layer;

      layer = gimp_layer_new_from_buffer (gimp_buffer_get_buffer (buffer),
                                          image,
                                          gimp_image_get_layer_format (image,
                                                                       TRUE),
                                          _("Clipboard"),
                                          GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
      g_object_unref (buffer);

      gimp_image_add_layer (image, layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING,
                            _("There is no image data in the clipboard to paste."));
    }
}

void
edit_named_cut_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Cut Named"), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_CUT,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  cut_named_buffer_callback, image);
  gtk_widget_show (dialog);
}

void
edit_fade_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = fade_dialog_new (image, widget);

  if (dialog)
    {
      g_signal_connect_object (image, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);
      gtk_widget_show (dialog);
    }
}

void
edit_named_copy_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Copy Named"), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_buffer_callback, image);
  gtk_widget_show (dialog);
}

void
edit_named_copy_visible_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Copy Visible Named "), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_visible_buffer_callback, image);
  gtk_widget_show (dialog);
}

void
edit_named_paste_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
                                             gimp,
                                             gimp_dialog_factory_get_singleton (),
                                             gtk_widget_get_screen (widget),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-buffer-list|gimp-buffer-grid");
}

void
edit_clear_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  return_if_no_drawable (image, drawable, data);

  gimp_edit_clear (image, drawable, action_data_get_context (data));
  gimp_image_flush (image);
}

void
edit_fill_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  GimpFillType  fill_type;
  return_if_no_drawable (image, drawable, data);

  fill_type = (GimpFillType) value;

  gimp_edit_fill (image, drawable, action_data_get_context (data),
                  fill_type, GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
  gimp_image_flush (image);
}


/*  private functions  */

static void
edit_paste (GimpDisplay *display,
            gboolean     paste_into)
{
  GimpImage *image = gimp_display_get_image (display);
  gchar     *svg;
  gsize      svg_size;

  svg = gimp_clipboard_get_svg (display->gimp, &svg_size);

  if (svg)
    {
      if (gimp_vectors_import_buffer (image, svg, svg_size,
                                      TRUE, FALSE,
                                      GIMP_IMAGE_ACTIVE_PARENT, -1,
                                      NULL, NULL))
        {
          gimp_image_flush (image);
        }

      g_free (svg);
    }
  else
    {
      GimpBuffer *buffer;

      buffer = gimp_clipboard_get_buffer (display->gimp);

      if (buffer)
        {
          GimpDisplayShell *shell = gimp_display_get_shell (display);
          gint              x, y;
          gint              width, height;

          gimp_display_shell_untransform_viewport (shell,
                                                   &x, &y, &width, &height);

          if (gimp_edit_paste (image,
                               gimp_image_get_active_drawable (image),
                               buffer, paste_into, x, y, width, height))
            {
              gimp_image_flush (image);
            }

          g_object_unref (buffer);
        }
      else
        {
          gimp_message_literal (display->gimp, G_OBJECT (display),
                                GIMP_MESSAGE_WARNING,
                                _("There is no image data in the clipboard to paste."));
        }
    }
}

static void
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  GimpImage    *image    = GIMP_IMAGE (data);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  GError       *error    = NULL;

  if (! drawable)
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to cut from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_cut (image, name, drawable,
                           gimp_get_user_context (image->gimp), &error))
    {
      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpImage    *image    = GIMP_IMAGE (data);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  GError       *error    = NULL;

  if (! drawable)
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                            _("There is no active layer or channel to copy from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_copy (image, name, drawable,
                            gimp_get_user_context (image->gimp), &error))
    {
      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

static void
copy_named_visible_buffer_callback (GtkWidget   *widget,
                                    const gchar *name,
                                    gpointer     data)
{
  GimpImage *image = GIMP_IMAGE (data);
  GError    *error = NULL;

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_copy_visible (image, name,
                                    gimp_get_user_context (image->gimp),
                                    &error))
    {
      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}
