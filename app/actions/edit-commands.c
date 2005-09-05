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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "vectors/gimpvectors-import.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   edit_paste                         (GimpDisplay *gdisp,
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
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (gimp_image_undo (gimage))
    gimp_image_flush (gimage);
}

void
edit_redo_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (gimp_image_redo (gimage))
    gimp_image_flush (gimage);
}

void
edit_undo_clear_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  gchar     *size;
  gint64     memsize;
  gint64     guisize;
  return_if_no_image (gimage, data);
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

  g_signal_connect_object (gimage, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Really clear image's undo history?"));

  memsize = gimp_object_get_memsize (GIMP_OBJECT (gimage->undo_stack),
                                     &guisize);
  memsize += guisize;
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->redo_stack),
                                      &guisize);
  memsize += guisize;

  size = gimp_memsize_to_string (memsize);

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the undo history of this "
                               "image will gain %s of memory."), size);
  g_free (size);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gimp_image_undo_disable (gimage);
      gimp_image_undo_enable (gimage);
      gimp_image_flush (gimage);
    }

  gtk_widget_destroy (dialog);
}

void
edit_cut_cmd_callback (GtkAction *action,
                       gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  if (gimp_edit_cut (gimage, drawable, action_data_get_context (data)))
    gimp_image_flush (gimage);
}

void
edit_copy_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  if (gimp_edit_copy (gimage, drawable, action_data_get_context (data)))
    gimp_image_flush (gimage);
}

void
edit_copy_visible_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (gimp_edit_copy_visible (gimage, action_data_get_context (data)))
    gimp_image_flush (gimage);
}

void
edit_paste_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpDisplay *gdisp = action_data_get_display (data);

  if (gdisp)
    edit_paste (gdisp, FALSE);
  else
    edit_paste_as_new_cmd_callback (action, data);
}

void
edit_paste_into_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  edit_paste (gdisp, TRUE);
}

void
edit_paste_as_new_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp       *gimp;
  GimpBuffer *buffer;
  return_if_no_gimp (gimp, data);

  buffer = gimp_clipboard_get_buffer (gimp);

  if (buffer)
    {
      GimpImage *image;

      image = gimp_edit_paste_as_new (gimp, action_data_get_image (data),
                                      buffer);

      if (image)
        {
          gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0);
          g_object_unref (image);
        }

      g_object_unref (buffer);
    }
}

void
edit_named_cut_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Cut Named"), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_CUT,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (gimage), "disconnect",
                                  cut_named_buffer_callback, gimage);
  gtk_widget_show (dialog);
}

void
edit_named_copy_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Copy Named"), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (gimage), "disconnect",
                                  copy_named_buffer_callback, gimage);
  gtk_widget_show (dialog);
}

void
edit_named_copy_visible_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_string_box (_("Copy Visible Named "), widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (gimage), "disconnect",
                                  copy_named_visible_buffer_callback, gimage);
  gtk_widget_show (dialog);
}

void
edit_named_paste_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (widget),
                                    "gimp-buffer-list|gimp-buffer-grid", -1);
}

void
edit_clear_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  gimp_edit_clear (gimage, drawable, action_data_get_context (data));
  gimp_image_flush (gimage);
}

void
edit_fill_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  GimpFillType  fill_type;
  return_if_no_drawable (gimage, drawable, data);

  fill_type = (GimpFillType) value;

  gimp_edit_fill (gimage, drawable, action_data_get_context (data),
                  fill_type);
  gimp_image_flush (gimage);
}


/*  private functions  */

static void
edit_paste (GimpDisplay *gdisp,
            gboolean     paste_into)
{
  gchar *svg;
  gsize  svg_size;

  svg = gimp_clipboard_get_svg (gdisp->gimage->gimp, &svg_size);

  if (svg)
    {
      if (gimp_vectors_import_buffer (gdisp->gimage, svg, svg_size,
                                      TRUE, TRUE, -1, NULL))
        {
          gimp_image_flush (gdisp->gimage);
        }

      g_free (svg);
    }
  else
    {
      GimpBuffer *buffer;

      buffer = gimp_clipboard_get_buffer (gdisp->gimage->gimp);

      if (buffer)
        {
          GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);
          gint              x, y;
          gint              width, height;

          gimp_display_shell_untransform_viewport (shell,
                                                   &x, &y, &width, &height);

          if (gimp_edit_paste (gdisp->gimage,
                               gimp_image_active_drawable (gdisp->gimage),
                               buffer, paste_into, x, y, width, height))
            {
              gimp_image_flush (gdisp->gimage);
            }

          g_object_unref (buffer);
        }
    }
}

static void
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  GimpImage    *gimage = GIMP_IMAGE (data);
  GimpDrawable *drawable;

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    {
      g_message (_("There is no active layer or channel to cut from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_cut (gimage, name, drawable,
                           gimp_get_user_context (gimage->gimp)))
    {
      gimp_image_flush (gimage);
    }
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
			    const gchar *name,
			    gpointer     data)
{
  GimpImage    *gimage = GIMP_IMAGE (data);
  GimpDrawable *drawable;

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    {
      g_message (_("There is no active layer or channel to copy from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_copy (gimage, name, drawable,
                            gimp_get_user_context (gimage->gimp)))
    {
      gimp_image_flush (gimage);
    }
}

static void
copy_named_visible_buffer_callback (GtkWidget   *widget,
                                    const gchar *name,
                                    gpointer     data)
{
  GimpImage *gimage = GIMP_IMAGE (data);

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (gimp_edit_named_copy_visible (gimage, name,
                                    gimp_get_user_context (gimage->gimp)))
    {
      gimp_image_flush (gimage);
    }
}
