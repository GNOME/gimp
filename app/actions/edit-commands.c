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

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"

#include "gui/dialogs.h"

#include "actions.h"
#include "edit-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   cut_named_buffer_callback  (GtkWidget   *widget,
                                          const gchar *name,
                                          gpointer     data);
static void   copy_named_buffer_callback (GtkWidget   *widget,
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

static void
edit_undo_clear_callback (GtkWidget *widget,
                          gboolean   clear,
                          gpointer   data)
{
  if (clear)
    {
      GimpImage *gimage = data;

      gimp_image_undo_disable (gimage);
      gimp_image_undo_enable (gimage);
      gimp_image_flush (gimage);
    }
}

void
edit_undo_clear_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = gimp_query_boolean_box (_("Clear Undo History"), widget,
                                   gimp_standard_help_func,
                                   GIMP_HELP_EDIT_UNDO_CLEAR,
                                   GIMP_STOCK_QUESTION,
                                   _("Really clear image's undo history?"),
                                   GTK_STOCK_CLEAR, GTK_STOCK_CANCEL,
                                   G_OBJECT (gimage),
                                   "disconnect",
                                   edit_undo_clear_callback,
                                   gimage);
  gtk_widget_show (dialog);
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

static void
edit_paste (GimpDisplay *gdisp,
            gboolean     paste_into)
{
  if (gdisp->gimage->gimp->global_buffer)
    {
      GimpDisplayShell *shell;
      gint              x, y, width, height;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

      if (gimp_edit_paste (gdisp->gimage,
                           gimp_image_active_drawable (gdisp->gimage),
                           gdisp->gimage->gimp->global_buffer,
                           paste_into, x, y, width, height))
	{
          gimp_image_flush (gdisp->gimage);
	}
    }
}

void
edit_paste_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  edit_paste (gdisp, FALSE);
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
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  if (gimp->global_buffer)
    gimp_edit_paste_as_new (gimp, action_data_get_image (data),
                            gimp->global_buffer);
}

void
edit_named_cut_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *qbox;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  qbox = gimp_query_string_box (_("Cut Named"), widget,
                                gimp_standard_help_func,
                                GIMP_HELP_BUFFER_CUT,
                                _("Enter a name for this buffer"),
                                NULL,
                                G_OBJECT (gimage), "disconnect",
                                cut_named_buffer_callback, gimage);
  gtk_widget_show (qbox);
}

void
edit_named_copy_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *qbox;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  qbox = gimp_query_string_box (_("Copy Named"), widget,
                                gimp_standard_help_func,
                                GIMP_HELP_BUFFER_COPY,
                                _("Enter a name for this buffer"),
                                NULL,
                                G_OBJECT (gimage), "disconnect",
                                copy_named_buffer_callback, gimage);
  gtk_widget_show (qbox);
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
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  GimpImage        *gimage = GIMP_IMAGE (data);
  const GimpBuffer *cut_buffer;
  GimpDrawable     *drawable;

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    {
      g_message (_("There is no active layer or channel to cut from."));
      return;
    }

  cut_buffer = gimp_edit_cut (gimage, drawable,
                              gimp_get_user_context (gimage->gimp));

  if (cut_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (cut_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
      g_object_unref (new_buffer);

      gimp_image_flush (gimage);
    }
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
			    const gchar *name,
			    gpointer     data)
{
  GimpImage        *gimage = GIMP_IMAGE (data);
  const GimpBuffer *copy_buffer;
  GimpDrawable     *drawable;

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    {
      g_message (_("There is no active layer or channel to copy from."));
      return;
    }

  copy_buffer = gimp_edit_copy (gimage, drawable,
                                gimp_get_user_context (gimage->gimp));

  if (copy_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (copy_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
      g_object_unref (new_buffer);

      gimp_image_flush (gimage);
    }
}
